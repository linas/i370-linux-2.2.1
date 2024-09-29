/*
 * Definitions for ESA/390 I/O Request Blocks, Channel Command Words, etc.
 */

#ifndef I370_IORB_H_
#define I370_IORB_H_

#define  _PACK __attribute__ ((packed))

/*
 *	Channel Command Word Command Byte bits
 */

#define CCW_CD		0x80	/* Use Data (Next CCW)              */
#define CCW_CC		0x40	/* Command Chaining                 */
#define CCW_SLI 	0x20	/* Suppress Length Incorrect        */
#define CCW_SKIP	0x10	/* Suppress Transfer of Information */
#define CCW_PCI 	0x08	/* PCI Intermediate Interruption    */
#define CCW_IDAW	0x04	/* CCW specifies location of IDAW   */
#define CCW_SUSPEND	0x02	/* Suspend before execution         */

#define CCW_CMD_RDC 	0x64	/* Read Device Characteristics */
#define CCW_CMD_DEXT	0x63	/* Define Extent */
#define CCW_CMD_LOCR	0x47	/* Locate Record */
#define CCW_CMD_RDDATA	0x06	/* Read Data (DASD) */
#define CCW_CMD_NOP     0x03	/* NO-OP CCW */
#define CCW_CMD_SID	0xe4	/* Sense ID */

#ifndef __ASSEMBLY__

/*
 *       Define the Format-1 Channel Command Word
 */

typedef struct _ccw {             /* CCW structure                    */
	char            cmd;      /* CCW command field                */
	char            flags;    /* CCW flags                        */
	short int       count;    /* count of bytes to transfer       */
	void            *dataptr; /* Data address                     */
} ccw_t;                          /* End of CCW structure             */

/*
 *       Define the Operation Request Block
 */

typedef struct _orb {             /* ORB structure                    */
	int             intparm;  /* interrupt parameter              */
	char            key;      /* channel key                      */
	char            fpiau;    /* channel program format           */
	char            lpm;      /* logical path mask                */
	char            zeros;    /*                                  */
	ccw_t           *ptrccw;  /* Channel program address          */
} orb_t;                          /* End of ORB structure             */

/*
 *       Define the DE  (Define Extent for DASD)
 */

typedef struct _de {              /* DE  structure                    */
	char            mask;     /* mask byte                        */
	char            globa;    /* global attributes                */
	unsigned short  recsize;  /* record size                      */
	unsigned short  id;       /* ID                               */
	unsigned char   gloa;     /* Global Attributes 		      */
	unsigned char   gloex; 	  /* Global Attr extended 	      */
	unsigned long   deext;    /* Start of Extent CCCCHHHH         */
	unsigned long   deend;	  /* End of Extend CCCCHHHH	      */
} de_t;                           /* End of D.E. structure            */

/*
 *       Define the LOCR (Locate record for DASD)
 */

typedef struct _locr {            /* LOCR structure                   */
	char            orop;     /* orientation and operation        */
	char            rsv;      /* reserved byte                    */
	char            aux;      /* auxilary byte                    */
	char            count;    /* record or track count            */
	long            seek;     /* seek cylinder - CCHH             */
	long            search;   /* search cylinder CCHH             */
	char            record;   /* search record                    */
	char            sector;   /* sector number                    */
	short           zeros;    /* must be zeros                    */
} locr_t;                         /* End of L.R. structure            */

/*
 * Define the SCSW SubChannel Status Word
 */

typedef struct _scsw {		/* SUBCHANNEL Status Word Structure     */
	unsigned key     :4;    /* Word 0  bits 0-3 Subchannel Key      */
	unsigned suspend :1;    /*         bit 4 Suspend control        */
	unsigned eswl    :1;    /*         bit 5 ESW logout             */
	unsigned cc      :2;    /*         bits 6-7 Condition Code      */
	unsigned format  :1;    /*         bit 8 Format control         */
	unsigned prfetch :1;    /*         bit 9 prefetch control       */
	unsigned intstat :1;    /*         bit 10 Initial Status        */
	unsigned adrlmt  :1;    /*         bit 11 Address-Limit         */
	unsigned supprs  :1;    /*         bit 12 Suppress-suspended ctl */
	unsigned zerocc  :1;    /*         bit 13 Zero cc               */
	unsigned eci     :1;    /* 	   bit 14 ECI stored in ECW     */
	unsigned pnom    :1;    /* 	   bit 15 Path not oper.        */
	unsigned         :1;    /* 	   bit 16 Unassigned            */
	unsigned fcntl   :3;    /* 	   bits 17-19 FCN control       */
	unsigned actvty  :7;    /* 	   bits 20-26 Activity Control  */
	unsigned status  :5;    /* 	   bits 27-31 Status Control    */
	char	  *ccw;     	/* Word 1  bits 0-31 CCW Address        */
	unsigned char devstat; 	/* Word 2  bits 0-7 Device Status       */
	unsigned char schstat;  /*         bits 8-15 Subchannel Status  */
	unsigned short residual; /*         bits 16-31 Residual Count   */
} scsw_t;     		/* End of SCSW structure */

/*
 *       Define the (IRB) Interruption Response Block
 */
	
typedef struct _irb {			/* IRB structure                     */
	scsw_t	 	scsw;     	/* Subchannel Status Word(s)         */
	unsigned		:1;     /* ESW word 0                        */
	unsigned 	esf	:7;     /*     bits 1-7 Extended Status Flag */
	unsigned char 	lpum;     	/*     bits 8-15 Last Path Used Mask */
	unsigned		:1;     /*     bit 16 unassigned             */
	unsigned 	fvf	:5;     /*     bits 17-21 FVF                */
	unsigned 	sa	:2;     /*     bits 22-23 Storage Access     */
	unsigned 	tc	:2;     /*     bits 24-25 Termination Code   */
	unsigned 	dsc	:1;     /*     bit 26 Device Status Check    */
	unsigned 	scnd	:1;     /*     bit 27 Secondary Error        */
	unsigned 	alert	:1;     /*     bit 28 I/O Error Alert        */
	unsigned 	seqcode :3;     /*     bit 29-31 Sequence Code       */
	unsigned int	esw[4];        /* Rest of ESW */
	unsigned int	ecw[8];        /* Extended Control Word */
	unsigned int	emw[8];        /* Extended Measurement Word */
} irb_t;     		/* End of IRB structure */
	
/*
 *       Define the (SCHIB) Subchannel Information Block
 */
	
typedef struct _schib {	         	/* SCHIB structure                   */
	unsigned int	interrupt_parm; /* Word 0: interruption parameter    */
	unsigned 		:2;    	/* Word 1: bits 0-1 unused           */
	unsigned        isc	:3;    	/*     bits 2-4 Interrupt Subclass   */
	unsigned 		:3;    	/*     bits 5-7 unused               */
	unsigned 	enabl 	:1;    	/*     bit 8 Subchannel Enablement   */
	unsigned 	lm	:2;    	/*     bits 9-10 Limit Mode          */
	unsigned 	mm	:2;    	/*     bits 11-12 Measurement Mode   */
	unsigned 	mpath 	:1;    	/*     bit 13 Multi-Path Mode        */
	unsigned 	tfac 	:1;    	/*     bit 14 Timing Faclty. Avail   */
	unsigned 	dval  	:1;    	/*     bit 15 Device Valid Bit       */
	unsigned short 	devno;  	/*     bits 16-31 Device Number      */
	unsigned 	lpm	:8;    	/* Word 2  bits 0-7 Logical Path Mask*/
	unsigned 	pnom	:8;    	/*     bits 8-15 Path Not Oper. Mask */
	unsigned 	lpum	:8;    	/*     bits 16-23 Last Path Used Mask*/
	unsigned 	pim 	:8;    	/*     bits 24-31 Path Installed Mask*/
	unsigned short 	mbi;    	/* Word 3  bits 0-15 Measure Blk ix  */
	unsigned 	pom   	:8; 	/*     bits 16-23 Path Oper. Mask    */
	unsigned 	pam    	:8; 	/*     bits 24-31 Path Avail. Mask   */
	unsigned char	chpids[8];      /* Words 4 and 5 - Subchannel CHPIDS */
	unsigned int	schfill;        /* Word 6  all zeros                 */
	scsw_t      	scsw;     	/* Words 7-9 SCSW                    */
	unsigned int	schfill2[3];    /* Word 10 Model Dependent           */
} schib_t;     		/* End of SCHIB structure             */
	
/*
 *       Define the (IORB) I/O Request Block
 */
	
typedef struct _PACK _iorb {
	char            eye[8];         /* Eye catcher                      */
	char            status;         /* device avail: OFFLINE/ONLINE=1   */
	char            reason;         /*                                  */
	char            opcode;         /* Last operation performed         */
	char            opstat;         /* PEND, COMP, AVAIL(0)             */
	char            filend;         /* file end indicator               */
	int             devnum;         /* device number                    */
	int             sid;            /* subchannel id                    */
	int             recsize;        /* record size                      */
	int             currec;         /* Current record number            */
	int             maxrec;         /* highest record number possible   */
	ccw_t           ccw[24];        /* channel program for this device  */
	orb_t           orb;            /* Operation Request Block          */
	char            chpid[8];       /* schib chpids                     */
	char            csect[8];       /* csect detecting error            */
	unsigned char   esw01;          /* ERP info block - first byte      */
	unsigned char   eswfl;          /* flag byte                        */
	unsigned char   eswvl;          /* validity indicators              */
	unsigned char   eswts;          /* termination sequence             */
	de_t            de;             /* Define extent struct -16 bytes   */
	irb_t           irb;            /* Intrp Resp block                 */
} iorb_t;
	
/* ---------------------------------------------------------------- */
/* I/O utilities */

/* *	Issue SSCH 	*/

extern inline int
_ssch (short sid, orb_t *orb)
{
	int     rc;
	long	scid = 0x10000 | sid;

        __asm__ __volatile__
        ("l     r1,%1;		\n"
         "ssch %2;		\n"
         "ipm   r1;		\n"
	 "srl   r1,28;		\n"
         "la    %0,0(0,r1);	\n"
        : "=r" (rc)
        : "m" (scid), "m"(*orb)
        :"r1","memory");

        return (rc);
}

/* *	Issue MSCH 	*/

extern inline int
_msch (short sid, schib_t *schib)
{
	int     rc;
	long	scid = 0x10000 | sid;

        __asm__ __volatile__
        ("l     r1,%1;		\n"
         "msch %2;		\n"
         "ipm   r1;		\n"
	 "srl   r1,28;		\n"
         "la    %0,0(0,r1);	\n"
        : "=r" (rc)
        : "m" (scid), "m"(*schib)
        :"r1","memory");

        return(rc);
}

/* *	Issue TSCH 	*/

extern inline int
_tsch (short sid, irb_t *irb)
{
	int     rc;
	long	scid = 0x10000 | sid;

        __asm__ __volatile__
        ("l     r1,%1;		\n"
         "tsch %2;		\n"
         "ipm   r1;		\n"
	 "srl   r1,28;		\n"
         "la    %0,0(0,r1);	\n"
        : "=r" (rc)
        : "m" (scid), "m"(*irb)
        :"r1","memory");

        return(rc);
}

/* *	Issue STSCH 	*/

extern inline int
_stsch (short sid, schib_t *schib)
{
	int     rc;
	long	scid = 0x10000 | sid;

        __asm__ __volatile__
        ("l     r1,%1;		\n"
         "stsch %2;		\n"
         "ipm   r1;		\n"
	 "srl   r1,28;		\n"
         "la    %0,0(0,r1);	\n"
        : "=r" (rc)
        : "m" (scid), "m"(*schib)
        :"r1","memory");

        return(rc);
}

extern inline int
_tpi (long *scid)
{
	int     rc;

	__asm__ __volatile__
		("	TPI   %1	\n"
		"       IPM   r1	\n"
		"       SRL   r1,28     \n"
		"       ST    r1,%0     \n"
		: "=m" (rc)
		: "m" (*scid)
		:"r1","memory");

	return(rc);
}

extern inline int
_csch(long scid)
{
	int     rc;

	__asm__ __volatile__
		("	L     r1,%1 	\n"
		"       CSCH		\n"
		"       IPM   r1	\n"
		"       SRL   r1,28     \n"
		"       ST    r1,%0     \n"
		: "=m" (rc)
		: "m" (scid)
		:"r1","memory");

	return(rc);
}

#endif /* __ASSEMBLY__ */


#undef _PACK
#endif /* I370_IORB_H_ */
