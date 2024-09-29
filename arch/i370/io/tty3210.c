/************************************************************/
/* Generic driver for 3210                                  */
/************************************************************/

#include <asm/3270.h>
#include <asm/delay.h>
#include <asm/ebcdic.h>
#include <asm/iorb.h>
#include <asm/spinlock.h>
#include <asm/string.h>
#include <asm/unitblk.h>

#include <linux/fs.h>

extern unitblk_t *dev_tty3210;

int i370_tty3210_open (struct inode *tty, struct file *filp)
{
	printk ("i370_cons_open\n");
	return 0;
}

ssize_t i370_tty3210_write (struct file *filp, const char *str,
                            size_t len, loff_t *noidea)
{
	long  pbufnext = 0;
	char  txtBuffer[1920];
	long  rc;
	long  i;
	long  flags;
	int   lign_ccw[5];
	ccw_t *ioccw;
	orb_t orb;
	irb_t irb;

	printk ("i370_cons_write: len=%ld %s<<<\n", len, str);
	if (NULL == dev_tty3210) {
		printk("Error: missing unit block for tty3210\n");
		return -ENODEV;
	}

	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) &lign_ccw[0]) + 7) >>3) << 3);

//	flags = cli();	/* reset interrupts */
	spin_lock_irqsave(NULL,flags);

	/*
	 *  Build the CCW for the 3210 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */

	for (i=0; i<len; i++) {
		txtBuffer[pbufnext] = ascii_to_ebcdic[(unsigned char)str[i]];
		/* kill the EBCDIC line-feed */
		if (txtBuffer[pbufnext] == 0x25)  txtBuffer[pbufnext] = 0x0;
		if ((txtBuffer[pbufnext] == 0x00) ||
		    (pbufnext >= sizeof(txtBuffer)))
		{
			ioccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
			ioccw[0].cmd     = CMDCON_WRI;     /* CCW command is write */
			ioccw[0].count   = pbufnext;
			ioccw[0].dataptr = txtBuffer;      /* address of 3210 buffer */
			ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
			ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
			ioccw[1].dataptr = NULL;           /* buffer = 0 */
			ioccw[1].count   = 1;              /* ?? */
			pbufnext         = 0;
			/*
			 *  Clear and format the ORB
			*/

			memset(&orb, 0x00, sizeof(orb_t));
			orb.intparm = (int) dev_tty3210;
			orb.fpiau  = 0x80;		/* format 1 ORB */
			orb.lpm    = 0xff;			/* Logical Path Mask */
			orb.ptrccw = &ioccw[0];		/* ccw addr to orb */

			rc = _tsch(dev_tty3210->unitsid, &irb); /* hack for unsolicited DE */
			rc = _ssch(dev_tty3210->unitsid, &orb); /* issue Start Subchannel */

/* XXX FIXME hack alert. We're just going to poll, for now But this is wrong. */
			while (1) {
				rc = _tsch(dev_tty3210->unitsid, &irb);
				if (!(irb.scsw.status & 0x1)) {
					udelay (100);	/* spin 100 microseconds */
					continue;
				}
				else {
					break; /* assume it worked */
				}
			}
		}
		else
			pbufnext++;
	}

	spin_unlock_irqrestore(NULL,flags);
//	sti();
	return 0;
}

void
i370_tty3210_driver(void)
{
}

void
i370_tty3210_flih(int irq, void *dev_id, struct pt_regs *regs)
{
}

/*===================== End of Mainline ====================*/

struct file_operations i370_fop_tty3210 =
{
   NULL,		 /* lseek - default */
   NULL,		 /* read - general block-dev read */
   i370_tty3210_write,	 /* write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   i370_tty3210_open,	 /* open */
   NULL,		 /* flush */
   NULL,		 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*===================== End of Function ====================*/
