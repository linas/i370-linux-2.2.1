/*
 * Implements 3270 console
 * Currently output only, used to dump kernel printk's
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/console.h>

#include <asm/3270.h>
#include <asm/atomic.h>
#include <asm/delay.h>
#include <asm/iorb.h>
#include <asm/unitblk.h>

/* ===================================================== */

/* ASCII to EBCDIC conversion table.  */
unsigned char ascebc[256] =
{
 /*00  NL    SH    SX    EX    ET    NQ    AK    BL */
     0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F,
 /*08  BS    HT    LF    VT    FF    CR    SO    SI */
     0x16, 0x05, 0x00, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
 /*10  DL    D1    D2    D3    D4    NK    SN    EB */
     0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,
 /*18  CN    EM    SB    EC    FS    GS    RS    US */
     0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
 /*20  SP     !     "     #     $     %     &     ' */
     0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,
 /*28   (     )     *     +     ,     -    .      / */
     0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
 /*30   0     1     2     3     4     5     6     7 */
     0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
 /*38   8     9     :     ;     <     =     >     ? */
     0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
 /*40   @     A     B     C     D     E     F     G */
     0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
 /*48   H     I     J     K     L     M     N     O */
     0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
 /*50   P     Q     R     S     T     U     V     W */
     0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
 /*58   X     Y     Z     [     \     ]     ^     _ */
     0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
 /*60   `     a     b     c     d     e     f     g */
     0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
 /*68   h     i     j     k     l     m     n     o */
     0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
 /*70   p     q     r     s     t     u     v     w */
     0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
 /*78   x     y     z     {     |     }     ~    DL */
     0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0xA1, 0x07,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xFF
};

/* EBCDIC to ASCII conversion table.  */
unsigned char ebcasc[256] =
{
 /*00  NU    SH    SX    EX    PF    HT    LC    DL */
     0x00, 0x01, 0x02, 0x03, 0x00, 0x09, 0x00, 0x7F,
 /*08              SM    VT    FF    CR    SO    SI */
     0x00, 0x00, 0x00, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
 /*10  DE    D1    D2    TM    RS    NL    BS    IL */
     0x10, 0x11, 0x12, 0x13, 0x14, 0x0A, 0x08, 0x00,
 /*18  CN    EM    CC    C1    FS    GS    RS    US */
     0x18, 0x19, 0x00, 0x00, 0x1C, 0x1D, 0x1E, 0x1F,
 /*20  DS    SS    FS          BP    LF    EB    EC */
     0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x17, 0x1B,
 /*28              SM    C2    EQ    AK    BL       */
     0x00, 0x00, 0x00, 0x00, 0x05, 0x06, 0x07, 0x00,
 /*30              SY          PN    RS    UC    ET */
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
 /*38                    C3    D4    NK          SU */
     0x00, 0x00, 0x00, 0x00, 0x14, 0x15, 0x00, 0x1A,
 /*40  SP                                           */
     0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*48                     .     <     (     +     | */
     0x00, 0x00, 0x00, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
 /*50   &                                           */
     0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*58               !     $     *     )     ;     ^ */
     0x00, 0x00, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
 /*60   -     /                                     */
     0x2D, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*68                     ,     %     _     >     ? */
     0x00, 0x00, 0x00, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
 /*70                                               */
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*78         `     :     #     @     '     =     " */
     0x00, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
 /*80         a     b     c     d     e     f     g */
     0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 /*88   h     i           {                         */
     0x68, 0x69, 0x00, 0x7B, 0x00, 0x00, 0x00, 0x00,
 /*90         j     k     l     m     n     o     p */
     0x00, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
 /*98   q     r           }                         */
     0x71, 0x72, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x00,
 /*A0         ~     s     t     u     v     w     x */
     0x00, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
 /*A8   y     z                       [             */
     0x79, 0x7A, 0x00, 0x00, 0x00, 0x5B, 0x00, 0x00,
 /*B0                                               */
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*B8                                 ]             */
     0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x00,
 /*C0   {     A     B     C     D     E     F     G */
     0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
 /*C8   H     I                                     */
     0x48, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*D0   }     J     K     L     M     N     O     P */
     0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
 /*D8   Q     R                                     */
     0x51, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*E0   \           S     T     U     V     W     X */
     0x5C, 0x00, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
 /*E8   Y     Z                                     */
     0x59, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*F0   0     1     2     3     4     5     6     7 */
     0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 /*F8   8     9                                     */
     0x38, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
};


/* ===================================================== */

static void console_write_3270(struct console *, const char *, unsigned);
static void console_write_3210(struct console *, const char *, unsigned);
static int console_setup_3270(struct console *, char *);
static int console_setup_3210(struct console *, char *);
static kdev_t console_device_3270(struct console *);
static kdev_t console_device_3210(struct console *);

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

static struct console cons3210 = {
        "3210",
        console_write_3210,
        NULL,
        console_device_3210,
        NULL,
        NULL,
        console_setup_3210,
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
extern  unitblk_t       *dev_cons; 
long		cons_init =0;

/* ===================================================== */

long	
console_3270_init(long mstart, long mend) 
{
	cons_init = 1;
	if (dev_cons->unitmajor != TTYAUX_MAJOR)
		register_console(&cons3270);
	else
		register_console(&cons3210);
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
	ccw_t   ioccw[2];
	orb_t   orb;
	irb_t   irb;
	prt_lne_t	*lastline;

	if (!(cons_init)) {
		return;
	}

//	flags = cli();	/* reset interrupts */
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
		dbgline->scrnln[i] = ascebc[(dbgline->scrnln[i])];
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
	orb.intparm = (int) dev_cons;
	orb.fpiau = 0x80;		/* format 1 ORB */
	orb.lpm = 0xff;			/* Logical Path Mask */
	orb.ptrccw = &ioccw[0];		/* ccw addr to orb */

	rc = _tsch(dev_cons->unitsid,&irb);     /* hack for unsolicited DE */
	rc = _ssch(dev_cons->unitsid,&orb);     /* issue Start Subchannel */

	while (1) {
		rc = _tsch(dev_cons->unitsid,&irb); 
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
//	sti();
}  

/* ===================================================== */

static void
console_write_3210(struct console *c, const char *s,
				unsigned count)
{
	long	rc;
	long	i;
	long	flags;
	ccw_t	ioccw[2];
	orb_t	orb;
	irb_t	irb;
 
	if (!(cons_init)) {
		return;
	}
 
//	flags = cli();	/* reset interrupts */
	spin_lock_irqsave(NULL,flags);
 
	/*
	 *  Build the CCW for the 3210 Console I/O
	 *  CCW = WRITE chained to NOP.
	 */
 
	for (i=0; i<count; i++) {
		conBuffer[pbufnext] = ascebc[(s[i])];
		if ((conBuffer[pbufnext] == 0x00) ||
		    (pbufnext >= sizeof(conBuffer))) {
			
			ioccw[0].flags   = CCW_CC+CCW_SLI; /* Write chained to NOOP + SLI */
			ioccw[0].cmd     = CMDCON_WRI;     /* CCW command is write */
			ioccw[0].count   = pbufnext;
			ioccw[0].dataptr = conBuffer;      /* address of 3210 buffer */
			ioccw[1].cmd     = CCW_CMD_NOP;    /* ccw is NOOP */
			ioccw[1].flags   = CCW_SLI;        /* Suppress Length Incorrect */
			ioccw[1].dataptr = NULL;           /* buffer = 0 */
			ioccw[1].count   = 1;              /* ?? */
			pbufnext         = 0;
			/*
			 *  Clear and format the ORB
			*/
 
			memset(&orb,0x00,sizeof(orb_t));
			orb.intparm = (int) dev_cons;
			orb.fpiau  = 0x80;		/* format 1 ORB */
			orb.lpm    = 0xff;			/* Logical Path Mask */
			orb.ptrccw = &ioccw[0];		/* ccw addr to orb */
 
			rc = _tsch(dev_cons->unitsid,&irb);  /* hack for unsolicited DE */
			rc = _ssch(dev_cons->unitsid,&orb);  /* issue Start Subchannel */

			while (1) {
				rc = _tsch(dev_cons->unitsid,&irb);
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
}
 
/* ===================================================== */
 
static kdev_t 
console_device_3270(struct console *c)
{
        return MKDEV(TTYAUX_MAJOR, 64 + c->index);
}

/* ===================================================== */

static kdev_t
console_device_3210(struct console *c)
{
        return MKDEV(TTYAUX_MAJOR, 64 + c->index);
}
 
/* ===================================================== */
 
__initfunc(static int 
console_setup_3270(struct console *co, char *options))
{
	long	i;
	
	pbufnext = 0x0;		/* next line = zero */
	for (i=0; i<MAX_PRINT_LINES; i++) {
		memset(&prtlines[i].prtbuf[0],0x0,MAX_LINE_SIZE);
	}
	return(0);
}
 
/* ===================================================== */
 
__initfunc(static int
console_setup_3210(struct console *co, char *options))
{
	
	pbufnext = 0x0;		/* next line = zero */
	return(0);
}

/* ===================================================== */
