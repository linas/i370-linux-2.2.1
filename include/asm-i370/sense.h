/*
 * Structures defining ESA/390 I/O Devices.
 * The IOCCW data field should point at one of these structures,
 * then a sequence of SSCH, STSCH and TSCH will result in these being
 * filled out.
 */

#ifndef I370_SENSE_H_
#define I370_SENSE_H_

#include <asm/iorb.h>
#define  _PACK __attribute__ ((packed))

/*
 *	Sense ID Structure
 */

typedef	struct _PACK _idchar
{
	unsigned char	idctrl; 	/* Function control byte */
	unsigned short	idcuid; 	/* Control unit ID */
	unsigned char	idcumdl;	/* Control unit model */
	unsigned short	iddevid;	/* Device ID */
	unsigned char	iddevmdl;	/* Device Model */
	unsigned char	idxxxx;		/* Unused */
	unsigned char	idextid;		/* Extended ID entry type */
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
	unsigned short	devtype; 	/* Device type */
	unsigned char	devmodel;	/* Device Model */
	unsigned char	devfeat[3];	/* Reserved */
	unsigned char	devsubfe;	/* Subsystem Features */
	unsigned char	devclcd; 	/* Device Class code */
	unsigned char	devtycod;	/* Device Type Code */
	short		devcyl;		/* number of primary cylinders */
	short 	devtrk;		/* number of tracks/cylinder */
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

/* System/390 CU ID */
#define T3990 0x3990
#define T3880 0x3880
#define TFBLK 0x3370
#define T3274 0x3274
#define T3210 0x3210
#define T3215 0x3215
#define T3480 0x3480
#define T3590 0x3590
#define T3172 0x3088   /* CTCA for hercules to network but why lie? */

/* System/390 CU Model */
#define MOSAD 0xe0
#define MCTCA 0x66
#define MHERC 0x08 /* Hercules report model 08 for networking CTCA */

#undef  _PACK
#endif /* I370_SENSE_H_ */
