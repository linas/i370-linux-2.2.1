/*
 * Mapping of ESA/390 I/O Devices to Linux devices.
 */

#ifndef I370_UNITB_H_
#define I370_UNITB_H_

#include <asm/sense.h>

#include <linux/fs.h>
#include <linux/major.h>

/*
 *	Unit Control Block. Contains a mixture of ESA/390 device fields
 * and Linux device driver fields. Very little in this structure
 * is currently used; I think its a place-holder for "future development"
 * that hasn't happened yet.
 */

#define DEVNAMELEN 10

typedef struct _unitblk {
	long		unitlock;	/* Structure Lock word */
	void		*unitque;	/* last queuing element */
	long		unitsid;	/* Subchannel Identifier */
	irb_t		unitirb;	/* Interrupt Request Block */
	void		*unitaction;	/* Pointer to irq action element */

	/* Linux kernel stuff */
	struct wait_queue *unitinq;     /* Input wait queue */
	struct wait_queue *unitoutq;    /* Output wait queue */
	struct wait_queue *unitexpq;    /* Exception wait queue */
	struct fasync_struct *unitasyq; /* Asynchronous reader queue */
	unsigned  long  unitnread;      /* Number of readers */
	unsigned  long  unitnwrit;      /* Number of writers */
	unsigned  int   unitmajor;	/* Major device number */
	unsigned  int   unitminor;	/* Minor device number */
	unsigned  int   unitisc;	/* Device interrupt subclass */
	struct file_operations *unitfops;
	void     	(*unitirqh)(int,    /* Interupt handler       */
				void *, struct pt_regs *);
	unsigned char   unitname[DEVNAMELEN];	/* Device name */
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
	unsigned char   fill[3];	/* fill to next 16 byte boundary */
	struct _unitblk *unitnxt;       /* Next Unit Block */
} unitblk_t;

#define	UNIT_READY	0x80	/* Subchannel is enabled */

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
	int		drvType; 	/* Device driver type     */
#   define CHRDEV 1
#   define BLKDEV 2
	char		devName[8];	/* Device name            */
	struct file_operations *fops;
	unsigned int 	isc;    	/* Interrupt sub-class    */
	void     	(*irqh)(int,    /* Interupt handler       */
				void *, struct pt_regs *);
} S390dev_t;

#define CON3270_MAJOR 227

/* See
 * https://lxr.linux.no/#linux+v2.6.31/Documentation/devices.txt
 */
/* Linux Major device number */
#define MJ3990 60   /* Local/experimental */
#define MJ3880 61
#define MJFBLK 62
#define MJ3274 63
#define MJCONS CON3270_MAJOR  /* Map system console to /dev/console */
#define MJ3215 CON3270_MAJOR  /* Map other attached 3215 to /dev/3270/rawN */
#define MJ3480 120  /* Local/experimental */
#define MJ3590 121
#define MJ3172 NBD_MAJOR /* ??? Network block device? I doubt it. */
#define MJCTCA 122

/* Linux device names in /dev/ */
#define D3990 "eckd"
#define D3880 "ckd"
#define DFBLK "fba"
#define D3274 "graf"
#define DCONS "console"
#define D3215 "3270/raw"
#define D3480 "tape"
#define D3590 "tss"
#define D3172 "nbd"
#define DCTCA "ctca"

/* Max number of raw terminals */
#define NRAWTERM 32
#define RAWMINOR 128

#endif /* I370_UNITB_H_ */
