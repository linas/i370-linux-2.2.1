/****************************************************************/
/* "raw" driver for 3210; I/O does not go through the tty layer */
/****************************************************************/

#include <asm/3270.h>
#include <asm/delay.h>
#include <asm/ebcdic.h>
#include <asm/iorb.h>
#include <asm/spinlock.h>
#include <asm/string.h>
#include <asm/uaccess.h>
#include <asm/unitblk.h>

#include <linux/fs.h>
#include <linux/mm.h>

/* Mapping of minor devnos to units. */
extern unitblk_t *unt_raw[NRAWTERM];

/* FIXME. The IOCCW needs to stay unclobbered until the I/O operation
 * has completed, i.e. until after we get the secondary status interrupt.
 * Until then, we should be careful not to clobber this!
 * In other words, the current design is basically broken, until we solve this.
 *
 * Use two different ioccws, to distinguish reader and writer interupts.
 * Each CCW is 8 bytes long; Each chain is two of them. The irq returns
 * when successful, return the address just past the end of the chain,
 * i.e. the below plus 0x10
 */
static int align_ccw[5];
ccw_t *wr_ioccw;

static int blign_ccw[5];
ccw_t *rd_ioccw;

int i370_raw3210_open (struct inode *inode, struct file *filp)
{
	printk ("raw3210_open of %s %x\n", filp->f_dentry->d_iname, inode->i_rdev);

	/* private_data should be unused, unless someone cut-n-pasted
	 * this code into a tty implementation of terminals. Ooops! */
	if (NULL != filp->private_data)
		return -ENODEV;

	int minor = inode->i_rdev & 0xff;
	if ((minor < RAWMINOR) || (RAWMINOR+NRAWTERM <= minor))
		return -ENODEV;

	filp->private_data = unt_raw[minor - RAWMINOR];

	/* double-word align the ccw array */
	wr_ioccw = (ccw_t *) (((((unsigned long) &align_ccw[0]) + 7) >>3) << 3);
	rd_ioccw = (ccw_t *) (((((unsigned long) &blign_ccw[0]) + 7) >>3) << 3);
	return 0;
}

static void do_write_one_line(char *ebcstr, size_t len, unitblk_t* unit)
{
	long rc;
	orb_t orb;

	/* FIXME/TODO:
	 * In principle, we should verify that we got a device-end signal,
	 * (secondary status) and are ready to perform more writing.
	 * For now, just assume everything is OK.
	 */

	/*
	 *  Build the CCW for the 3210 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */
	wr_ioccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
	wr_ioccw[0].cmd     = CMDCON_WRI;     /* CCW command is write */
	wr_ioccw[0].count   = len;
	wr_ioccw[0].dataptr = ebcstr;      /* address of 3210 buffer */
	wr_ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	wr_ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	wr_ioccw[1].dataptr = NULL;           /* buffer = 0 */
	wr_ioccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (int) unit;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;			/* Logical Path Mask */
	orb.ptrccw = wr_ioccw;		/* ccw addr to orb */

	rc = _ssch(unit->unitsid, &orb); /* issue Start Subchannel */

#if 0
	/* XXX Do this at least once!? Huh? Note sure why; if not done,
	 * the first byte written becomes blank. Seems buggy. I dunno.
	 * fcntl=4 means 'start function'
	 * activity=20 means 'start pending'
	 * status=1 means 'status pending'
	 * Bug: if we don't delay at least a bit, the first byte to
	 * be printed is lost. Beats me why. Altnerately, the printk
	 * provides enough delay.
	 */
	irb_t irb;
	rc = _tsch(unit->unitsid, &irb);
	udelay (100);   /* spin 100 microseconds */
	// printk("raw3210_write irb FCN=%x activity=%x status=%x\n",
	//       irb.scsw.fcntl, irb.scsw.actvty, irb.scsw.status);
#endif

#if 0
	int cnt = 0;
	while (1) {
		rc = _tsch(unit->unitsid, &irb);
		if (!(irb.scsw.status & 0x1)) {
			udelay (100);   /* spin 100 microseconds */
			cnt++;
			continue;
		}
		else {
			break; /* assume it worked */
		}
	}
	printk("write irb status=%x after %d loops\n", irb.scsw.status, cnt);
#endif
}

static long do_write (char* kstr, const char *str, size_t len, unitblk_t* unit)
{
	long  flags;
	long  rc;
	long  i, j;
#define EBCLEN 120
	char  ebcstr[EBCLEN];

	if (PAGE_SIZE <= len)
	{
		printk("Error: unexpected long raw3210 input; FIXME!\n");
		return -ENAMETOOLONG;
	}

	rc = strncpy_from_user(kstr, str, len);
	if (rc < 0)
		return rc;
	if (0 == rc)
		return -ENOENT;

	spin_lock_irqsave(NULL,flags);
	j = 0;
	for (i=0; i<len; i++) {
		ebcstr[j] = ascii_to_ebcdic[(unsigned char)kstr[i]];

		/* kill the EBCDIC line-feed */
		if (ebcstr[j] == 0x25) ebcstr[j] = 0x0;
		if ((ebcstr[j] == 0x0) || (j >= EBCLEN-2))
		{
			if (0 < j)
				do_write_one_line(ebcstr, j, unit);
			j = 0;
		}
		else
			j++;
	}
	spin_unlock_irqrestore(NULL,flags);

	return len;
}

ssize_t i370_raw3210_write (struct file *filp, const char *str,
                            size_t len, loff_t *ignore)
{
	long rc;
	char * kstr;

	unitblk_t* unit = (unitblk_t*) filp->private_data;

	kstr = (char *) __get_free_page(GFP_KERNEL);
	if (!kstr)
		return -ENOMEM;

	rc = do_write(kstr, str, len, unit);

	free_page((unsigned long)kstr);
	return rc;
}

/* XXX FIXME this is shared by all and gets clobbered.
 * we want a per-unit read bufer.
 */
#define RDBUFSZ 120
char rdbuf[RDBUFSZ];

static void show_rdbuf(void)
{
	int i;
	if (0 == rdbuf[0]) return;

	for (i=0; i<16; i++) {
		if (0 == rdbuf[i]) break;
		printk("got %d %c\n", i, ebcdic_to_ascii[(unsigned char)rdbuf[i]]);
	}
}

static void do_read(unitblk_t* unit)
{
	long rc;
	orb_t orb;

	memset(rdbuf, 0, RDBUFSZ);

	/*
	 *  Build the CCW for the 3210 Console I/O
	 *  CCW = READ chained to NOP.
	 */
	rd_ioccw[0].flags   = CCW_CC+CCW_SLI; /* Read chained to NOOP + SLI */
	rd_ioccw[0].cmd     = CMDCON_RD;      /* CCW command is read */
	rd_ioccw[0].count   = RDBUFSZ;
	rd_ioccw[0].dataptr = rdbuf;          /* address of 3210 buffer */
	rd_ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	rd_ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	rd_ioccw[1].dataptr = NULL;           /* buffer = 0 */
	rd_ioccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (int) unit;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;			/* Logical Path Mask */
	orb.ptrccw = rd_ioccw;		/* ccw addr to orb */

	rc = _ssch(unit->unitsid, &orb); /* issue Start Subchannel */
}

ssize_t i370_raw3210_read (struct file *filp, char *str,
                           size_t len, loff_t *ignore)
{
	unitblk_t* unit = (unitblk_t*) filp->private_data;
	do_read(unit);

	char* kstr = "yes";
	size_t readlen = strlen(kstr)+1;
	__copy_to_user(str, kstr, readlen);
	return readlen;
}

void
i370_raw3210_flih(int irq, void *dev_id, struct pt_regs *regs)
{
	int rc;
	irb_t irb;
	unitblk_t* unit = (unitblk_t*) dev_id;

	memset(&irb, 0x00, sizeof(irb_t));
	rc = _tsch(unit->unitsid, &irb);

	/* Quick hack to see what we are taking interrupt for */
	unsigned long ewr_ioccw = wr_ioccw;
	ewr_ioccw += 0x10;
	unsigned long erd_ioccw = rd_ioccw;
	erd_ioccw += 0x10;

	/* Ignore interupts announcing end-of-write. For now. */
	if (ewr_ioccw == irb.scsw.ccw) {
		// printk("got writer done interupt\n");
		return;
	}

	/* Ah hah: read is done! */
	if (erd_ioccw == irb.scsw.ccw) {
		printk("got reader done interupt\n");
	}

	printk("raw3210_flihw irb FCN=%x activity=%x status=%x\n",
		irb.scsw.fcntl, irb.scsw.actvty, irb.scsw.status);

	printk("devstat=%x schstat=%x residual=%x\n", irb.scsw.devstat,
		irb.scsw.schstat, irb.scsw.residual);
	show_rdbuf();
}

/*===================== End of Mainline ====================*/

struct file_operations i370_fop_raw3210 =
{
   NULL,		 /* lseek - default */
   i370_raw3210_read,	 /* read - general block-dev read */
   i370_raw3210_write,	 /* write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   i370_raw3210_open,	 /* open */
   NULL,		 /* flush */
   NULL,		 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*===================== End of Function ====================*/
