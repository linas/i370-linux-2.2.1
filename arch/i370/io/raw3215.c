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
#define LINELEN 120

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
	return 0;
}

/* ---------------------------------------------------------------- */

/* Can't do stuff, until the previous one has finished.
	This is almost identical to interruptible_sleep_on()
	except that it is meant to be used with interrupts disabled
	so that the status falg doesn't get trashed by an I/O interrupt.
	Must enter with irq's disabled, and we return with them still
   disabled.
 */
static void wait_for_status(unitblk_t* ucb, unsigned long PENDING,
                            struct wait_queue **que, long *irqf)
{
	struct wait_queue wait;
	unsigned long timeout = MAX_SCHEDULE_TIMEOUT;

	/* Sigh. Due to console hackology, we are allowing multiple
	   processes to write to the console.(!) That means that wait
	   queues don't cut it; different processes stomp on each-others
	   status flags. We could fack it for writing by using a semaphore
	   not a queue, but we also have readers sharing wand we don't
	   know which task to route a read to. That's unfixable, except
	   by forcing each new process to open it's own /dev/3215/rawNN
	   and disallow sharing of console for I/O. But that's for some
	   future date. (It doesn't help that con3270.c is doing _tsch
	   and is resetting our channel status.) For now,  we'll brute
	   for this, and just wait and cross our fingers.
	   Two millisecs should be enough.  */
	/* Timeouts are in jiffies, which happen at HZ. One jiffy should
	 * be about 10 to 50 millisecs on current settings. */
	if (PENDING == WRITE_PENDING)
		timeout = 4;

	/* Whelp, this is still broken, and I can't figure out how to hac
	 * around it. We can either _tsch like in con3270.c or just disable
	 * more than one user using a console. I dunno. At any rate, busybox
	 * is broken because of this feat of mis-engineering.
	 */
	if (ucb->unitflg1 & PENDING) {
		current->state = TASK_INTERRUPTIBLE;
		wait.task = current;
		add_wait_queue(que, &wait);

		spin_unlock_irqrestore(NULL, *irqf);
		/* schedule(); */
		schedule_timeout(timeout);
		spin_lock_irqsave(NULL, *irqf);

		remove_wait_queue(que, &wait);

// #define BRUTE_FORCE
#ifdef BRUTE_FORCE
		if ((PENDING == WRITE_PENDING) && (ucb->unitflg1 & PENDING)) {
			irb_t irb;
			while (1) {
				_tsch(ucb->unitsid, &irb);
				if (irb.scsw.status & 0x1) break;
				udelay (100);  /* spin 100 microseconds */
			}
		}
#endif
	}
}

/* ---------------------------------------------------------------- */

static void do_write_one_line(char *ebcstr, size_t len, unitblk_t* ucb)
{
	long  flags;
	long rc;
	orb_t orb;

	spin_lock_irqsave(NULL,flags);

	/* Can't start a new write, until the previous one has finished. */
	wait_for_status(ucb, WRITE_PENDING, &(ucb->unitoutq), &flags);

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

	/* Hang out here, until the write completes.  Why? If we return
	   before the write completes, the ebcstr buffer, which lives on
	   stack, will disappear with the stack, and the write will
		print whatever garbage is found on the stack. Ooops!  */
	wait_for_status(ucb, WRITE_PENDING, &(ucb->unitoutq), &flags);

	spin_unlock_irqrestore(NULL,flags);
}

ssize_t i370_raw3215_write (struct file *filp, const char *str,
                            size_t len, loff_t *ignore)
{
	long  rc;
	int i;
	const char *p;
	size_t nc, nleft;
	unitblk_t* ucb = (unitblk_t*) filp->private_data;
	char kstr[LINELEN];

#define MAXWRI (10*PAGE_SIZE)
	/* Don't spam the console. */
	if (MAXWRI <= len)
		return -E2BIG;

	/* Don't go past end of memory. STACK_TOP == 0x80000000 */
	if (STACK_TOP - MAXWRI <= (unsigned long) str)
		return -E2BIG;

	/* Loop. One line at a time. */
	/* Why? Because uClibc can't deal with it, if we return -E2BIG for
	 * writes longer than 120 chars. So we approximate a normal tty. */
	p = str;
	nleft = len;
	while (p < str+len) {
		nc = nleft;
		if (LINELEN < nc)
			nc = LINELEN;

		rc = copy_from_user(kstr, p, nc);
		if (rc)
			return -EIO;

		for (i=0; i<nc; i++)
			kstr[i] = ascii_to_ebcdic[(unsigned char)kstr[i]];
		do_write_one_line(kstr, nc, ucb);

		p += nc;
		nleft -= nc;
	}
	return len;
}

/* ---------------------------------------------------------------- */

/* Perform the read.
 * Return the number of characters read.
 * Max possible is 120.
 */
static int do_read(unitblk_t* ucb, char* rdbuf)
{
	long flags;
	long rc;
	orb_t orb;
	irb_t *irb;

	spin_lock_irqsave(NULL,flags);

	/* Can't start a new read, until the previous one has finished. */
	wait_for_status(ucb, READ_PENDING, &(ucb->unitinq), &flags);

	ucb->unitflg1 &= ~READ_WANTED;
	memset(rdbuf, 0, LINELEN);

	/*
	 *  Build the CCW for the 3215 Console I/O
	 *  CCW = READ chained to NOP.
	 */
	rdccw[0].flags   = CCW_CC;         /* Read chained to NOOP */
	rdccw[0].cmd     = CMDCON_RD;      /* CCW command is read */
	rdccw[0].count   = 120;            /* More than 120 is hated */
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

	ucb->unitflg1 |= READ_PENDING;
	rc = _ssch(ucb->unitsid, &orb); /* issue Start Subchannel */
	DBGPRT("raw3215_read ssch=%d\n", rc);

	/* Wait for the read-completed interrupt */
	wait_for_status(ucb, READ_PENDING, &(ucb->unitinq), &flags);

	spin_unlock_irqrestore(NULL,flags);

	irb = &ucb->unitirb;

	DBGPRT("raw3215_read irb FCN=%x activity=%x status=%x\n",
		irb->scsw.fcntl, irb->scsw.actvty, irb->scsw.status);
	DBGPRT("devstat=0x%x schstat=0x%x residual=%d\n",
		irb->scsw.devstat, irb->scsw.schstat, irb->scsw.residual);

	/* The "residual" is how much room is still left in a 120-character
	 * line. So the number of characters read is 120 minus residual.  */
	return 120 - irb->scsw.residual;
}

/* ---------------------------------------------------------------- */

ssize_t i370_raw3215_read (struct file *filp, char *str,
                           size_t len, loff_t *ignore)
{
	int nread;
	int i;
	unitblk_t* ucb = (unitblk_t*) filp->private_data;
	char rdbuf[LINELEN+2];

	if ((0 == (ucb->unitflg1 & READ_WANTED)) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	/* Ensure reaction to signals */
	if (signal_pending(current))
		return -ERESTARTSYS;

	/* Wait for unsolicited keyboard interrupt. */
	interruptible_sleep_on(&ucb->unitexpq);
	nread = do_read(ucb, rdbuf);
	if (0 == nread) return -EAGAIN;

	for (i=0; i<nread; i++)
		rdbuf[i] = ebcdic_to_ascii[(unsigned char)rdbuf[i]];

	/* Whelp. Pretened to be a tty, and just stick a newline at the end.
	 * Conventional C libs like uClic have no idea we are not a tty, and
	 * so we are going to fake it. */
	rdbuf[i] = '\n';
	i++;
	rdbuf[i] = 0;
	__copy_to_user(str, rdbuf, i);
	return i;
}

/* ---------------------------------------------------------------- */

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
