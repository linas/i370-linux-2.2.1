/*
 * structures defining ESA/390 I/O Devices
 */

#ifndef I370_UNITB_H_
#define I370_UNITB_H_

#include <linux/fs.h>
#include <linux/major.h>

/*
 *	Define Unit Control Block
 */

typedef struct _unitblk {
	long		unitlock;	/* Structure Lock word */
	void		*unitque;	/* last queuing element */
	long		unitsid;	/* Subchannel Identifier */
	irb_t		unitirb;	/* Interrupt Request Block */
	void           *unitaction;	/* Pointer to irq action element */
	unsigned  int   unitmajor;	/* Major device number */
	unsigned  int   unitminor;	/* Minor device number */
	int             unitisc;	/* Device interrupt subclass */
	void           *unitirqh;	/* Interrupt handler     */
	unsigned  char  unitname[8];	/* Device name */

	unsigned char   unitflg1;	/* flag byte */
	unsigned char   unitflg2;	/* Flag byte */
	unsigned char   unitid;		/* Unit ID */
	unsigned char   unitstat;	/* Device Status */
	unsigned 	unitdtyp:16;	/* Device type from RDC */
	unsigned short  unitdev;	/* Device Number */
	unsigned char   unitmodl;	/* Device Model from RDC */
	unsigned char   unitclas;	/* Device Class code from RDC */
	unsigned char   unittycd;	/* Device Type code from RDC */
	unsigned char   unitcuid;	/* CUID from RDC */
	unsigned char   unitvol[6];	/* volume Id */
	unsigned char   fill[10];       /* fill to next 16 byte boundary */
	struct _unitblk *unitnxt;       /* Next Unit Block */
} unitblk_t;

#define	UNIT_READY	0x80	/* Subchannel is enabled */

/*
 *	Sense ID Structure
 */

typedef	struct _idchar
{
	unsigned char	idctrl;	/* Function control byte */
	unsigned 	idcuid:16;	/* Control unit ID */
	unsigned char	idcumdl;	/* Control unit model */
	unsigned 	iddevid:16;	/* Device ID */
	unsigned char	iddevmdl;	/* Device Model */
	unsigned char	idxxxx;		/* Unused */
	unsigned char	idextid;	/* Extended ID entry type */
	unsigned char	idrcdc;		/* Read Config Data command code */
	unsigned short	idrcd;		/*  Bytes of data returned by RCD */
	unsigned char	idrsv[244];	/* Reserved */

	/*---------------------------------------------------------*/
	/* Device Type Information...                              */
	/*---------------------------------------------------------*/

#define t3270 0x04
#define t3210 0x00

} idchar_t;

/*
 *	Read Device Characteristics Structure
 */

typedef	struct _devchar {
	unsigned short	devcutyp;	/* Control Unit Type */
	unsigned char   devcumod;	/* Control Unit Model */
	unsigned 	devtype:16;	/* Device type */
	unsigned char	devmodel;	/* Device Model */
	unsigned char	devfeat[3];	/* Reserved */
	unsigned char   devsubfe;	/* Subsystem Features */
	unsigned char   devclcd;	/* Device Class code */
	unsigned char   devtycod;	/* Device Type Code */
	short		devcyl;		/* number of primary cylinders */
	short 		devtrk;		/* number of tracks/cylinder */
	unsigned char	devsect;        /* number of sectors */
	unsigned char	devtrkln[3];	/* Total usable track length */
	unsigned 	devhar0:16;	/* Length for HA and R0 */
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
	unsigned char   devvol[6];      /* add volume id */
	unsigned char   fill1[2];       /* fill to boundary */
} devchar_t;

/*------------------------------------------------------------*/
/* Maps the device info. returned by Sense ID to a major no.  */
/*------------------------------------------------------------*/
typedef struct {
	unsigned short 	cuid;		/* Control unit ID (idcuid)	*/
	unsigned char	model;		/* Control unit model		*/
	unsigned 	dev:16;		/* Device ID (iddevid)		*/
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
#   define CHRDEV 0
#   define BLKDEV 1
	char		devName[8];	/* Device name            */
	struct file_operations *fops;
	unsigned int 	isc;    	/* Interrupt sub-class    */
	void     	(*irqh)(int,    /* Interupt handler       */
				void *, struct pt_regs *);
} S390dev_t __attribute__ ((packed));

#define T3990 0x3990
#define T3880 0x3880
#define TFBLK 0x3370
#define T3274 0x3274
#define T3210 0x3210
#define T3480 0x3480
#define T3590 0x3590
#define T3172 0x3088

#define MJ3990 60
#define MJ3880 61
#define MJFBLK 62
#define MJ3274 63
#define MJ3210 TTYAUX_MAJOR
#define MJ3480 120
#define MJ3590 121
#define MJ3172 NBD_MAJOR
#define MJCTCA 122

#define D3990 "eckd"
#define D3880 "ckd"
#define DFBLK "fba"
#define D3274 "graf"
#define D3210 "console"
#define D3480 "tape"
#define D3590 "tss"
#define D3172 "nbd"
#define DCTCA "ctca"

#define MOSAD 0xe0
#define MCTCA 0x66


#endif /* I370_UNITB_H_ */
