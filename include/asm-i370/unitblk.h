/*     
 */
        
#ifndef I370_UNITB_H_
#define I370_UNITB_H_
                                                                                
                                                                                
/*                                                                              
 *	Define Unit Control Block
 */                                                                             
                                                                                
typedef struct _unitblk {                                                       
	long		unitlock;	/* Structure Lock word */
	void		*unitque;	/* last queuing element */
	long		unitsid;	/* Subchannel Identifier */
	long		unitparm; 	/* Interruption Parameter */
	unsigned char   unitflg1;	/* flag byte */
	unsigned char   unitflg2;	/* Flag byte */
	unsigned char   unitid;		/* Unit ID */
	unsigned char   unitstat;	/* Device Status */
	unsigned char	unitdtyp[2];	/* Device type from RDC */
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
 *	Read Device Characteristics Structure
 */                                                                             

typedef	struct _devchar {
	unsigned short	devcutyp;	/* Control Unit Type */
	unsigned char   devcumod;	/* Control Unit Model */
	unsigned char	devtype[2];	/* Device type */
	unsigned char	devmodel;	/* Device Model */
	unsigned char	devfeat[3];	/* Reserved */
	unsigned char   devsubfe;	/* Subsystem Features */
	unsigned char   devclcd;	/* Device Class code */
	unsigned char   devtycod;	/* Device Type Code */
	short		devcyl;		/* number of primary cylinders */
	short 		devtrk;		/* number of tracks/cylinder */
	unsigned char	devsect;        /* number of sectors */
	unsigned char	devtrkln[3];    /* Total usable track length */
	unsigned char	devhar0[2];     /* Length for HA and R0 */
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


#endif /* I370_UNITB_H_ */
