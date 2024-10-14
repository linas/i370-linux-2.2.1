/****************************************************************/
/* "raw" driver for 3215; I/O does not go through the tty layer */
/* Of course, 3215 can't ever go through a tty layer; that's    */
/* not how ESA/390 I/O works. Clashes a bit with conventional   */
/* unix, but so it goes.                                        */
/*                                                              */
/* Documentation in detail available on page 30 of              */
/* http://wwww.bitsavers.org/pdf/ibm/370/funcChar/GA22-6942-1_370-155_funcChar_Jan71.pdf */
/* In particular, the subchannel status words & etc are         */
/* explained in full detail. The explanation is for a 3210.     */
/* Notable is that using 0x1 instead of 0x9 for writes will     */
/* inhibit the automatic carriage return, and maybe we want     */
/* that.                                                        */
/*
 * Known but unfixed bugs:
 *
 * If you hit enter, with no chars beforehand, you get
 * HHCTE006A Enter input for console device 0009
 * If I wait long enough that the shell attempts another read,
 * I think the pending flag below remains set, these console
 * input requests pile up, and the input is lost. The only way to
 * clear this is to type, hit enter, fast, over and over, until
 * everything pending gets thrown out. So this is some kind of
 * 3215 behavior that the code below somehow trips over.
 *
 * Here's another: if I enter more than 80 chars, e.g. by cut and
 * pasting multiple lines into the telnet session, Hercules closes
 * the session. Reopening, its stuck in the
 * HHCTE006A Enter input for console device 0009
 * mode again.
 *
 * So overall this is delicate.
 ****************************************************************/

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
#include <linux/wait.h>

/* Mapping of minor devnos to units. */
extern unitblk_t *unt_raw[NRAWTERM];
extern unitblk_t *unt_con3215;

/* FIXME. The IOCCW needs to stay unclobbered until the I/O operation
 * has completed, i.e. until after we get the secondary status interrupt.
 * Until then, we should be careful not to clobber this!
 * The current design is to just spin.
 */
static int align_ccw[5];
ccw_t *ioccw;

#define SHOW_IRB_STATUS
#ifdef SHOW_IRB_STATUS
	#define DBGPRT(...) printk(__VA_ARGS__)
#else
	#define DBGPRT(...)
#endif

int i370_raw3215_open (struct inode *inode, struct file *filp)
{
	printk ("raw3215_open of /dev/%s (c %d %d)\n",
	        filp->f_dentry->d_iname,
	        inode->i_rdev >> 8, inode->i_rdev & 0xff);

	/* private_data should be unused, unless someone cut-n-pasted
	 * this code into a tty implementation of terminals. Ooops! */
	if (NULL != filp->private_data)
		return -ENODEV;

	int minor = inode->i_rdev & 0xff;
	if ((RAWMINOR <= minor) && (minor < RAWMINOR+NRAWTERM))
		filp->private_data = unt_raw[minor - RAWMINOR];
	else if (1 == minor)
		filp->private_data = unt_con3215;
	else
		return -ENODEV;

	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) &align_ccw[0]) + 7) >>3) << 3);
	return 0;
}

int i370_raw3215_close (struct inode *inode, struct file *filp)
{
	printk ("raw3215_close of /dev/%s\n",
	        filp->f_dentry->d_iname);
}

/* ---------------------------------------------------------------- */

static void do_write_one_line(char *ebcstr, size_t len, unitblk_t* unit)
{
	long  flags;
	long rc;
	orb_t orb;

	spin_lock_irqsave(NULL,flags);

	/* Can't start a new write, until the previous one has finished. */

	/*
	 *  Build the CCW for the 3215 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */
	ioccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
	ioccw[0].cmd     = CMDCON_WRI;     /* CCW command is write */
	ioccw[0].count   = len;
	ioccw[0].dataptr = ebcstr;      /* address of 3215 buffer */
	ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	ioccw[1].dataptr = NULL;           /* buffer = 0 */
	ioccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (int) unit;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;		/* Logical Path Mask */
	orb.ptrccw = ioccw;		/* ccw addr to orb */

	rc = _ssch(unit->unitsid, &orb); /* issue Start Subchannel */
	spin_unlock_irqrestore(NULL,flags);
}

static long do_write (char* kstr, const char *str, size_t len, unitblk_t* unit)
{
	long  rc;
	long  i, j;
#define EBCLEN 120
	char  ebcstr[EBCLEN];

	rc = copy_from_user(kstr, str, len);
	if (rc)
		return -EIO;

	j = 0;
	for (i=0; i<len; i++) {
		ebcstr[j] = ascii_to_ebcdic[(unsigned char)kstr[i]];

		if ((ebcstr[j] == 0x0) || (j >= EBCLEN-2))
		{
			if (0 < j)
				do_write_one_line(ebcstr, j, unit);
			j = 0;
		}
		else
			j++;
	}

	/* Anything left? */
	if (0 < j)
		do_write_one_line(ebcstr, j, unit);

	return len;
}

ssize_t i370_raw3215_write (struct file *filp, const char *str,
                            size_t len, loff_t *ignore)
{
	long rc;
	char * kstr;

	unitblk_t* unit = (unitblk_t*) filp->private_data;

	/* Avoid spew to console */
	if (PAGE_SIZE <= len)
		return -E2BIG;

	kstr = (char *) __get_free_page(GFP_KERNEL);
	if (!kstr)
		return -ENOMEM;

	rc = do_write(kstr, str, len, unit);

	free_page((unsigned long)kstr);
	return rc;
}

/* XXX FIXME this is shared by all and gets clobbered.
 * we want a per-unit read buffer.
 */
#define RDBUFSZ 120
char rdbuf[RDBUFSZ];
char rascii[RDBUFSZ];

/* Return the number of characters read.
 * Max possible is 120.
 */
static int do_read(unitblk_t* unit)
{
	int flags;
	int i;
	long rc;
	orb_t orb;
	irb_t irb;

	memset(rdbuf, 0, RDBUFSZ);
	spin_lock_irqsave(NULL,flags);

#ifdef INEFFECTIVE
	/* Well, the idea here was to play it safe, and not clobber the ioccw
	 * until the device is in a happy state. But even when the device is
	 * happy, behavior is flakey. So I don't get it. The TSCH loop at
	 * the bottom fixes things. See similar block of code above, in
	 * do_write()
	 */
#define NLOOP 1000
	for (i=0; i<NLOOP; i++) {
		rc = _tsch(unit->unitsid, &irb);
		if ((irb.scsw.devstat & 0xfd) == 0xc) break;
		udelay (25);   /* spin 25 microseconds */
	}
	if (NLOOP == i) {
		// return -EIO;
		printk("No! Watch out! Mr. Bill!\n");
	}
#endif

	/*
	 *  Build the CCW for the 3215 Console I/O
	 *  CCW = READ chained to NOP.
	 */
	ioccw[0].flags   = CCW_CC;         /* Read chained to NOOP */
	ioccw[0].cmd     = CMDCON_RD;      /* CCW command is read */
	ioccw[0].count   = RDBUFSZ;
	ioccw[0].dataptr = rdbuf;          /* address of 3215 buffer */
	ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	ioccw[1].dataptr = NULL;           /* buffer = 0 */
	ioccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (int) unit;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;		/* Logical Path Mask */
	orb.ptrccw = ioccw;		/* ccw addr to orb */

	rc = _ssch(unit->unitsid, &orb); /* issue Start Subchannel */

	for (i=0; i<1000; i++) {
		udelay (25);   /* spin 25 microseconds */
		rc = _tsch(unit->unitsid, &irb);
		if (irb.scsw.status & 0x1) break;
	}

	DBGPRT("raw3215_read spin for %d\n", i);
	DBGPRT("raw3215_read irb FCN=%x activity=%x status=%x\n",
		irb.scsw.fcntl, irb.scsw.actvty, irb.scsw.status);
	DBGPRT("devstat=0x%x schstat=0x%x residual=%d\n", irb.scsw.devstat,
		irb.scsw.schstat, irb.scsw.residual);

	spin_unlock_irqrestore(NULL,flags);

	/* The "residual" is how much room is still left in a 120-character
	 * line. So the number of characters read is 120 minus residual */
	return 120 - irb.scsw.residual;
}

/* ---------------------------------------------------------------- */

/* XXX fixme, this should be in the unit block, for multiple devices */
static int i370_pending=0;

ssize_t i370_raw3215_read (struct file *filp, char *str,
                           size_t len, loff_t *ignore)
{
	int nread;
	int i;
	unitblk_t* unit = (unitblk_t*) filp->private_data;

	if ((0 == i370_pending) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	/* Ensure reaction to signals */
	if (signal_pending(current))
		return -ERESTARTSYS;

	interruptible_sleep_on(&unit->unitinq);
	i370_pending = 0;
	nread = do_read(unit);

	/* Seems like nread == i always, on exit of loop. */
	for (i=0; i<RDBUFSZ; i++) {
		rascii[i] = ebcdic_to_ascii[(unsigned char)rdbuf[i]];
		if (0 == rdbuf[i]) break;
	}
	if (0 == i) return -EAGAIN;

	i++;
	__copy_to_user(str, rascii, i);
	return i;
}

void
i370_raw3215_flih(int irq, void *dev_id, struct pt_regs *regs)
{
	unitblk_t* ucb = (unitblk_t*) dev_id;
	irb_t* irb = &ucb->unitirb;

	DBGPRT("raw3215_flihw irb FCN=%x activity=%x status=%x\n",
	       irb->scsw.fcntl, irb->scsw.actvty, irb->scsw.status);
	DBGPRT("devstat=%x schstat=%x residual=%x\n",
	       irb->scsw.devstat, irb->scsw.schstat, irb->scsw.residual);

	// In the current implementation, we only get interrupts for reads.
	i370_pending = 1;
	wake_up_interruptible(&ucb->unitinq);
}

/*===================== End of Mainline ====================*/

struct file_operations i370_fop_raw3215 =
{
   NULL,		 /* lseek - default */
   i370_raw3215_read,	 /* read - general block-dev read */
   i370_raw3215_write,	 /* write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   i370_raw3215_open,	 /* open */
   NULL,		 /* flush */
   i370_raw3215_close,	 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*===================== End of Function ====================*/
