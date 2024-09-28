/*
 * Structures defining ESA/390 I/O Devices
 */

#ifndef I370_UNITB_H_
#define I370_UNITB_H_

#include <linux/fs.h>
#include <linux/major.h>
#include <asm/iorb.h>
#define  _PACK __attribute__ ((packed))

/*
 *	Define Unit Control Block
 */

typedef struct _unitblk {
	long		unitlock;	/* Structure Lock word */
	void		*unitque;	/* last queuing element */
	long		unitsid;	/* Subchannel Identifier */
	irb_t		unitirb;	/* Interrupt Request Block */
	void		*unitaction;	/* Pointer to irq action element */
	struct wait_queue *unitinq;     /* Input wait queue */
	struct wait_queue *unitoutq;    /* Output wait queue */
	struct wait_queue *unitexpq;    /* Exception wait queue */
	struct fasync_struct *unitasyq; /* Asynchronous reader queue */
	unsigned  long  unitnread;      /* Number of readers */
	unsigned  long  unitnwrit;      /* Number of writers */
	unsigned  int   unitmajor;	/* Major device number */
	unsigned  int   unitminor;	/* Minor device number */
	int             unitisc;	/* Device interrupt subclass */
	struct file_operations *unitfops;
	void     	(*unitirqh)(int,    /* Interupt handler       */
				void *, struct pt_regs *);
	unsigned char   unitname[8];	/* Device name */
	unsigned char   unitflg1;	/* flag byte */
	unsigned char   unitflg2;	/* Flag byte */
	unsigned char   unitid;		/* Unit ID */
	unsigned char   unitstat;	/* Device Status */
	unsigned short  unitdtyp;	/* Device type from RDC */
	unsigned short  unitdev;	/* Device Number */
	unsigned char   unitmodl;	/* Device Model from RDC */
	unsigned char   unitclas;	/* Device Class code from RDC */
	unsigned char   unittycd;	/* Device Type code from RDC */
	unsigned char   unitcuid;	/* CUID from RDC */
	unsigned char   unitvol[6];	/* volume Id */
	unsigned char   unittype;	/* 1=charddev; 2=blockdev */
	unsigned char   fill[5];	/* fill to next 16 byte boundary */
	struct _unitblk *unitnxt;       /* Next Unit Block */
} unitblk_t;

#define	UNIT_READY	0x80	/* Subchannel is enabled */

/*
 *	Sense ID Structure
 */

typedef	struct _PACK _idchar
{
	unsigned char	idctrl;		/* Function control byte */
	unsigned short	idcuid;		/* Control unit ID */
	unsigned char	idcumdl;	/* Control unit model */
	unsigned short	iddevid;	/* Device ID */
	unsigned char	iddevmdl;	/* Device Model */
	unsigned char	idxxxx;		/* Unused */
	unsigned char	idextid;	/* Extended ID entry type */
	unsigned char	idrcdc;		/* Read Config Data command code */
	unsigned short	idrcd;		/* Bytes of data returned by RCD */
	unsigned char	idrsv[244];	/* Reserved */

	/*---------------------------------------------------------*/
	/* Device Type Information...                              */
	/*---------------------------------------------------------*/

} idchar_t;

/*
 *	Read Device Characteristics Structure
 */

typedef	struct _PACK _devchar {
	unsigned short	devcutyp;	/* Control Unit Type */
	unsigned char	devcumod;	/* Control Unit Model */
	unsigned short	devtype;	/* Device type */
	unsigned char	devmodel;	/* Device Model */
	unsigned char	devfeat[3];	/* Reserved */
	unsigned char	devsubfe;	/* Subsystem Features */
	unsigned char	devclcd;	/* Device Class code */
	unsigned char	devtycod;	/* Device Type Code */
	short		devcyl;		/* number of primary cylinders */
	short 		devtrk;		/* number of tracks/cylinder */
	unsigned char	devsect;        /* number of sectors */
	unsigned char	devtrkln[3];	/* Total usable track length */
	unsigned short	devhar0;	/* Length for HA and R0 */
	unsigned char	devmode;	/* Track Capacity Mode */
	unsigned char	devmodr;	/* Track Capacity Mode changed */
	short		devnkey;	/* Non-Keyed Record Overhead */
	short		devkey;		/* Keyed Record Overhead */
	short		devaltc;	/* Address of first Alternate cylinder */
	short		devaltr;	/* Number of Alternate track */
	short		devdiagc;	/* Address of diagnostic Cylinder */
	short		devdiagt;	/* Number of diagnostic Tracks */
	short		devdvcyl;	/* Address of first Device Cylinder */
	short		devdvsup;	/* Number of Device Support Tracks */
	unsigned char	devmdr;		/* MDR Record id */
	unsigned char	devobr;		/* OBR Record id */
	unsigned char	devcuid;	/* Control Unit Id */
	unsigned char	fill[21];	/* Rest of RDC block */
	unsigned char	devvol[6];      /* add volume id */
	unsigned char	devvtype[4];    /* Volume type */
	unsigned char	fill1[2];       /* fill to boundary */
} devchar_t;

/*------------------------------------------------------------*/
/* Maps the device info. returned by Sense ID to a major no.  */
/*------------------------------------------------------------*/
typedef struct {
	unsigned short int cuid;        /* Control unit ID (idcuid)     */
	unsigned char	model;		/* Control unit model		*/
	unsigned short	dev;		/* Device ID (iddevid)          */
	int      	i_s390dev;	/* Index into s390dev table	*/
} S390map_t;

/*------------------------------------------------------------*/
/* Maps the major number to device driver information         */
/*------------------------------------------------------------*/
typedef struct {
	unsigned int 	major;		/* Major number           */
	int		curMinor;	/* Current minor number   */
	int		maxMinor;	/* Maximum no. of devices */
	int		drvType;	/* Device driver type     */
#   define CHRDEV 1
#   define BLKDEV 2
	char		devName[8];	/* Device name            */
	struct file_operations *fops;
	unsigned int 	isc;    	/* Interrupt sub-class    */
	void     	(*irqh)(int,    /* Interupt handler       */
				void *, struct pt_regs *);
} S390dev_t;

/* System/390 Device ID */
#define T3990 0x3990
#define T3880 0x3880
#define TFBLK 0x3370
#define T3274 0x3274
#define T3210 0x3210
#define T3215 0x3215
#define T3480 0x3480
#define T3590 0x3590
#define T3172 0x3088

/* System/390 CU Model */
#define MOSAD 0xe0
#define MCTCA 0x66

/* Linux Major device number */
#define MJ3990 60
#define MJ3880 61
#define MJFBLK 62
#define MJ3274 63
#define MJCONS TTYAUX_MAJOR  /* Map system console to /dev/console */
#define MJ3210 TTY_MAJOR     /* Map other attached 3210 to /dev/ttyN */
#define MJ3480 120
#define MJ3590 121
#define MJ3172 NBD_MAJOR
#define MJCTCA 122

/* Linux device names in /dev/ */
#define D3990 "eckd"
#define D3880 "ckd"
#define DFBLK "fba"
#define D3274 "graf"
#define DCONS "console"
#define D3210 "tty"
#define D3480 "tape"
#define D3590 "tss"
#define D3172 "nbd"
#define DCTCA "ctca"

#undef  _PACK
#endif /* I370_UNITB_H_ */
