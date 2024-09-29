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

extern unitblk_t *dev_raw3210;

int i370_raw3210_open (struct inode *inode, struct file *filp)
{
	printk ("i370_raw3210_open of %s\n", filp->f_dentry->d_iname);
	printk("duuude its %x\n", inode->i_rdev);
	return 0;
}

static void do_write_one_line(char *ebcstr, size_t len)
{
	long rc;
	int   lign_ccw[5];
	ccw_t *ioccw;
	orb_t orb;
	irb_t irb;

	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) &lign_ccw[0]) + 7) >>3) << 3);

	/*
	 *  Build the CCW for the 3210 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */
	ioccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
	ioccw[0].cmd     = CMDCON_WRI;     /* CCW command is write */
	ioccw[0].count   = len;
	ioccw[0].dataptr = ebcstr;      /* address of 3210 buffer */
	ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	ioccw[1].dataptr = NULL;           /* buffer = 0 */
	ioccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (int) dev_raw3210;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;			/* Logical Path Mask */
	orb.ptrccw = &ioccw[0];		/* ccw addr to orb */

	rc = _tsch(dev_raw3210->unitsid, &irb); /* hack for unsolicited DE */
	rc = _ssch(dev_raw3210->unitsid, &orb); /* issue Start Subchannel */

/* XXX FIXME hack alert. We're just going to poll, for now But this is wrong. */
	while (1) {
		rc = _tsch(dev_raw3210->unitsid, &irb);
		if (!(irb.scsw.status & 0x1)) {
			udelay (100);	/* spin 100 microseconds */
			continue;
		}
		else {
			break; /* assume it worked */
		}
	}
}

static long do_write (char* kstr, const char *str, size_t len)
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

kstr[len-2]=0;
printk("print to raw3270 >>%s<< %ld\n", kstr, len);

	spin_lock_irqsave(NULL,flags);
	j = 0;
	for (i=0; i<len; i++) {
		ebcstr[j] = ascii_to_ebcdic[(unsigned char)kstr[i]];

		/* kill the EBCDIC line-feed */
		if (ebcstr[j] == 0x25) ebcstr[j] = 0x0;
		if ((ebcstr[j] == 0x0) || (j >= EBCLEN-2))
		{
			if (0 < j)
				do_write_one_line(ebcstr, j);
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

	if (NULL == dev_raw3210) {
		printk("Error: missing unit block for raw3210\n");
		return -ENODEV;
	}

	kstr = (char *) __get_free_page(GFP_KERNEL);
	if (!kstr)
		return -ENOMEM;

	rc = do_write(kstr, str, len);

	free_page((unsigned long)kstr);
	return rc;
}

void
i370_raw3210_driver(void)
{
}

void
i370_raw3210_flih(int irq, void *dev_id, struct pt_regs *regs)
{
}

/*===================== End of Mainline ====================*/

struct file_operations i370_fop_raw3210 =
{
   NULL,		 /* lseek - default */
   NULL,		 /* read - general block-dev read */
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
