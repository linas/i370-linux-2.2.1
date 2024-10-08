/*
 * Implements early-initialization 3270 console
 * Output only, used to dump kernel printk's from the very earliest
 * moments of booting. This is a polling driver, because interrupts
 * are not available in early boot. This is NOT a general-purpose
 * 3210/3215/3270 driver!
 *
 * See raw3215.c for a generic driver.
 */
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/console.h>

#include <asm/3270.h>
#include <asm/atomic.h>
#include <asm/delay.h>
#include <asm/ebcdic.h>
#include <asm/iorb.h>
#include <asm/unitblk.h>

/* ===================================================== */

static void console_write_3270(struct console *, const char *, unsigned);
static void console_write_3215(struct console *, const char *, unsigned);
static int console_setup_3270(struct console *, char *);
static int console_setup_3215(struct console *, char *);
static kdev_t console_device_3270(struct console *);
static kdev_t console_device_3215(struct console *);

static struct console cons3270 = {
        "3270",
        console_write_3270,
        NULL,
        console_device_3270,
        NULL,
        NULL,
        console_setup_3270,
        CON_PRINTBUFFER,
        1,
        0,
        NULL
};

static struct console cons3215 = {
        "3215",
        console_write_3215,
        NULL,
        console_device_3215,
        NULL,
        NULL,
        console_setup_3215,
        CON_PRINTBUFFER,
        1,
        0,
        NULL
};

long		pbufnext;	/* index of next line in prtlines */
char		conBuffer[1920];
pbuffer_t	prtlines[MAX_PRINT_LINES];	/* PRINTK buffer array */
extern	char	scrn3270[1];	/* define 3270 screen */
extern	char	scrln1[1];	/* define 3270 screen */
extern	char	screnda[1];     /* define 3270 screen end */
prt_lne_t	*dbgline = (prt_lne_t *)scrln1;
prt_lne_t	*dbglend = (prt_lne_t *)screnda;
extern  unitblk_t *unt_cons,
                  *unt_con3215,
                  *unt_con3270;
long		cons_init =0;

/* ===================================================== */

long	
console_3270_init(long mstart, long mend)
{
	cons_init = 1;
	if (unt_cons == unt_con3270)
		register_console(&cons3270);
	else
		register_console(&cons3215);
	return(mstart);
}

/* ===================================================== */

static void
console_write_3270(struct console *c, const char *s,
				unsigned count)
{
	long	rc;
	long	i;
	long	flags;
	int     lign_ccw[5];
	ccw_t   *ioccw;
	orb_t   orb;
	irb_t   irb;
	prt_lne_t	*lastline;

	if (!(cons_init)) {
		return;
	}

/* XXXX - Until 3270 rewritten properly */
printk("Hack alert: 3270 writes go to 3215 driver\n");
console_write_3215(c, s, count);
return;

	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) lign_ccw) + 7) >>3) << 3);

	spin_lock_irqsave(NULL,flags);

	if (count > 79) {
		count = 79;
	}

	/*
	 * If we are at the last output line
	 * clear the screen and wrap back to first line.
	 */

	if (dbgline >= dbglend) {
		dbgline = (prt_lne_t *)scrln1;
		for (i=0; i<MAXLINES; i++) {
			dbgline->attribute = ATTRSKIP;
			memset((void *)&dbgline->scrnln[0],0x00,79);
			dbgline++;
		}
		dbgline = (prt_lne_t *)scrln1;
	}

	/*
	 * Clear the Output Line and Set HI-INTENSITY in attribute
	 */

	memset((void *)&dbgline->scrnln[0],0x00,79);
	dbgline->attribute = ATTRPRHI;
	lastline = dbgline;   /* copy for attribute reset */

	for (i=0; i<count; i++) {
		dbgline->scrnln[i] = ascii_to_ebcdic[(unsigned char)(dbgline->scrnln[i])];
	}

	dbgline++;

	/*
	 *  Build the CCW for the 3270 Screen I/O
	 *  CCW = WRITE/ERASE chained to NOP.
	 */

	if (count > 0) {
		ioccw[0].flags = CCW_CC+CCW_SLI;   /* Write chained to NOOP + SLI */
		ioccw[0].cmd = CMDTRM_WRI; /* ccw command is write */
		ioccw[0].count = 0x74f;
		ioccw[0].dataptr = scrn3270; /* address of 3270 buffer */
		ioccw[1].cmd = CCW_CMD_NOP;    /* ccw is NOOP */
		ioccw[1].flags = CCW_SLI;      /* Suppress Length Incorrect */
		ioccw[1].dataptr = NULL;   /* buffer = 0 */
		ioccw[1].count = 1;        /* ?? */
	}
	else {
		return;
	}

	/*
	 *  Clear and format the ORB
	 */

	memset(&orb,0x00,sizeof(orb_t));
	orb.intparm = (int) unt_con3270;
	orb.fpiau = 0x80;		/* format 1 ORB */
	orb.lpm = 0xff;			/* Logical Path Mask */
	orb.ptrccw = &ioccw[0];		/* ccw addr to orb */

	rc = _tsch(unt_con3270->unitsid, &irb);     /* hack for unsolicited DE */
	rc = _ssch(unt_con3270->unitsid, &orb);     /* issue Start Subchannel */

	while (1) {
		rc = _tsch(unt_con3270->unitsid,&irb);
		if (!(irb.scsw.status & 0x1)) {
			udelay (100);	/* spin 100 microseconds */
			continue;
		}
		else {
			break; /* assume it worked */
		}
	}

	/*
	 * Reset the 3270 Attribute for the Last Output line to Normal
	 * Intensity.
	 */

	lastline->attribute = ATTRSKIP;
	spin_unlock_irqrestore(NULL,flags);
}

/* ===================================================== */

static void
console_write_3215(struct console *c, const char *s, unsigned count)
{
	long  rc;
	long  i;
	long  flags;
	int   lign_ccw[5];
	ccw_t *ioccw;
	orb_t orb;
	irb_t irb;

	if (!(cons_init)) {
		return;
	}

	/* XXX FIXME! Major fail! The ioccw is being stored on stack, where
	 * it is guaranteed to be clobbered. The only reason this works for
	 * now is that we spin, polling the TSCH until the status is OK.
	 * Once the status is OK, the IOCCW can be clobbered.
	 */
	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) &lign_ccw[0]) + 7) >>3) << 3);

	spin_lock_irqsave(NULL,flags);

	/*
	 *  Build the CCW for the 3215 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */

	for (i=0; i<count; i++) {
		conBuffer[pbufnext] = ascii_to_ebcdic[(unsigned char)(s[i])];
		/* Kill the EBCDIC line-feed. Auto-carrier-return puts it back. */
		if (conBuffer[pbufnext] == 0x25)  conBuffer[pbufnext] = 0x0;
		if ((conBuffer[pbufnext] == 0x00) ||
		    (pbufnext >= sizeof(conBuffer)))
		{
			ioccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
			ioccw[0].cmd     = CMDCON_WRICR;   /* Write w/auto carrier return */
			ioccw[0].count   = pbufnext;
			ioccw[0].dataptr = conBuffer;      /* address of 3215 buffer */
			ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
			ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
			ioccw[1].dataptr = NULL;           /* buffer = 0 */
			ioccw[1].count   = 1;              /* ?? */
			pbufnext         = 0;
			/*
			 *  Clear and format the ORB
			*/

			memset(&orb,0x00,sizeof(orb_t));
			orb.intparm = (int) unt_con3215;
			orb.fpiau  = 0x80;		/* format 1 ORB */
			orb.lpm    = 0xff;			/* Logical Path Mask */
			orb.ptrccw = &ioccw[0];		/* ccw addr to orb */

			rc = _tsch(unt_con3215->unitsid,&irb); /* hack for unsolicited DE */
			rc = _ssch(unt_con3215->unitsid,&orb); /* issue Start Subchannel */

			while (1) {
				rc = _tsch(unt_con3215->unitsid, &irb);
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
}

/* ===================================================== */

static kdev_t
console_device_3270(struct console *c)
{
	return MKDEV(CON3270_MAJOR, 1);
}

/* ===================================================== */

static kdev_t
console_device_3215(struct console *c)
{
	return MKDEV(CON3270_MAJOR, 1);
}

/* ===================================================== */

__initfunc(static int
console_setup_3270(struct console *co, char *options))
{
	long	i;
	
	pbufnext = 0x0;		/* next line = zero */
	for (i=0; i<MAX_PRINT_LINES; i++) {
		memset(&prtlines[i].prtbuf[0], 0x0, MAX_LINE_SIZE);
	}
	return(0);
}

/* ===================================================== */

__initfunc(static int
console_setup_3215(struct console *co, char *options))
{
	pbufnext = 0x0;		/* next line = zero */
	return(0);
}

/* ===================================================== */
