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

/* Each ccw is 8 bytes (2 ints) and we want to chain two of them,
   and we want to use different ones for the read and the write.
   And we must align on a doubleword boundary. So 4+4+1=9. And,
	well, we will pad with one more, for paranoia in the irb block. */
static int align_ccw[17];
ccw_t *rdccw;
ccw_t *wrccw;

// #define SHOW_IRB_STATUS
#ifdef SHOW_IRB_STATUS
	#define DBGPRT(...) printk(__VA_ARGS__)
#else
	#define DBGPRT(...)
#endif

/* I/O status  flags */
#define WRITE_PENDING 0x1
#define READ_PENDING 0x2
#define READ_WANTED 0x4

/* The reality of line devices */
#define LINELEN 124

/* ---------------------------------------------------------------- */

int i370_raw3215_open (struct inode *inode, struct file *filp)
{
	ccw_t *ioccw;
	unitblk_t *ucb;
	printk ("raw3215_open of /dev/%s (c %d %d)\n",
	        filp->f_dentry->d_iname,
	        inode->i_rdev >> 8, inode->i_rdev & 0xff);

	/* private_data should be unused, unless someone cut-n-pasted
	 * this code into a tty implementation of terminals. Ooops! */
	if (NULL != filp->private_data)
		return -ENODEV;

	int minor = inode->i_rdev & 0xff;
	if ((RAWMINOR <= minor) && (minor < RAWMINOR+NRAWTERM))
		ucb = unt_raw[minor - RAWMINOR];
	else if (1 == minor)
		ucb = unt_con3215;
	else
		return -ENODEV;

	filp->private_data = ucb;

	ucb->unitflg1 = 0;

	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) &align_ccw[0]) + 7) >>3) << 3);
	rdccw = &ioccw[0];
	wrccw = &ioccw[4];

	return 0;
}

int i370_raw3215_close (struct inode *inode, struct file *filp)
{
	printk ("raw3215_close of /dev/%s\n",
	        filp->f_dentry->d_iname);
}

/* ---------------------------------------------------------------- */

static void do_write_one_line(char *ebcstr, size_t len, unitblk_t* ucb)
{
	long  flags;
	long rc;
	orb_t orb;

	/* Can't start a new write, until the previous one has finished. */
	if (ucb->unitflg1 & WRITE_PENDING)
		interruptible_sleep_on(&ucb->unitoutq);

	spin_lock_irqsave(NULL,flags);
	/*
	 *  Build the CCW for the 3215 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */
	wrccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
	wrccw[0].cmd     = CMDCON_WRI;     /* CCW command is write */
	wrccw[0].count   = len;
	wrccw[0].dataptr = ebcstr;      /* address of 3215 buffer */
	wrccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	wrccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	wrccw[1].dataptr = NULL;           /* buffer = 0 */
	wrccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (int) ucb;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;		/* Logical Path Mask */
	orb.ptrccw = wrccw;		/* ccw addr to orb */

	ucb->unitflg1 |= WRITE_PENDING;
	rc = _ssch(ucb->unitsid, &orb); /* issue Start Subchannel */
	spin_unlock_irqrestore(NULL,flags);
}

ssize_t i370_raw3215_write (struct file *filp, const char *str,
                            size_t len, loff_t *ignore)
{
	long  rc;
	int  i;
	unitblk_t* ucb = (unitblk_t*) filp->private_data;
	char kstr[LINELEN];

	/* Sorry. One line at a time. */
	if (LINELEN <= len)
		return -E2BIG;

	rc = copy_from_user(kstr, str, len);
	if (rc)
		return -EIO;

	for (i=0; i<len; i++)
		kstr[i] = ascii_to_ebcdic[(unsigned char)kstr[i]];

	do_write_one_line(kstr, len, ucb);
	return len;
}

/* ---------------------------------------------------------------- */
/* XXX FIXME this is shared by all and gets clobbered.
 * we want a per-unit read buffer.
 */
#define RDBUFSZ 120
char rdbuf[RDBUFSZ];
char rascii[RDBUFSZ];

/* Return the number of characters read.
 * Max possible is 120.
 */
static int do_read(unitblk_t* ucb)
{
	int flags;
	int i;
	long rc;
	orb_t orb;
	irb_t irb;

	/* Can't start a new read, until the previous one has finished. */
	if (ucb->unitflg1 & READ_PENDING)
		interruptible_sleep_on(&ucb->unitinq);

	memset(rdbuf, 0, RDBUFSZ);
	spin_lock_irqsave(NULL,flags);

	ucb->unitflg1 &= ~READ_WANTED;
	ucb->unitflg1 |= READ_PENDING;
	/*
	 *  Build the CCW for the 3215 Console I/O
	 *  CCW = READ chained to NOP.
	 */
	rdccw[0].flags   = CCW_CC;         /* Read chained to NOOP */
	rdccw[0].cmd     = CMDCON_RD;      /* CCW command is read */
	rdccw[0].count   = RDBUFSZ;
	rdccw[0].dataptr = rdbuf;          /* address of 3215 buffer */
	rdccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
	rdccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
	rdccw[1].dataptr = NULL;           /* buffer = 0 */
	rdccw[1].count   = 1;              /* ?? */
	/*
	 *  Clear and format the ORB
	 */
	memset(&orb, 0x00, sizeof(orb_t));
	orb.intparm = (unsigned long) ucb;
	orb.fpiau  = 0x80;		/* format 1 ORB */
	orb.lpm    = 0xff;		/* Logical Path Mask */
	orb.ptrccw = rdccw;		/* ccw addr to orb */

	rc = _ssch(ucb->unitsid, &orb); /* issue Start Subchannel */

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

ssize_t i370_raw3215_read (struct file *filp, char *str,
                           size_t len, loff_t *ignore)
{
	int nread;
	int i;
	unitblk_t* ucb = (unitblk_t*) filp->private_data;

	if ((0 == (ucb->unitflg1 & READ_WANTED)) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	/* Ensure reaction to signals */
	if (signal_pending(current))
		return -ERESTARTSYS;

	/* Wait for unsolicited keyboard interrupt. */
	interruptible_sleep_on(&ucb->unitexpq);
	nread = do_read(ucb);

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
	DBGPRT("unit flags = %x\n", ucb->unitflg1);

	/* Empty ccw is an unsolicited interrupt. Basically, user
	 * hit "enter" at the keyboard.  */
	if (irb->scsw.ccw == 0) {
		ucb->unitflg1 |= READ_WANTED;
		wake_up_interruptible(&ucb->unitexpq);
	}
	else if (irb->scsw.ccw == &rdccw[1]) {
		ucb->unitflg1 &= ~READ_PENDING;
		wake_up_interruptible(&ucb->unitinq);
	}
	else if (irb->scsw.ccw == &wrccw[2]) {
		ucb->unitflg1 &= ~WRITE_PENDING;
		wake_up_interruptible(&ucb->unitoutq);
	}
	else {
		printk("Unhandled 3215 interrupt status ccw=%lx\n",
		       (unsigned long) irb->scsw.ccw);
		i370_halt();
	}
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
