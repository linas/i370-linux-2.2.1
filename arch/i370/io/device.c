/*
 * quick & dirty I/O setup; i/o info placed in low memory during boot.
 * uses spinloops to poll device status ... because interrupts are
 * not yet initialized ...
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tasks.h>
#include <linux/string.h>

#include <asm/delay.h>
#include <asm/iorb.h>
#include <asm/unitblk.h>

/*
 *	Issue a Read Device Characteristics to a specific subchannel
 */

int     i370_getrdc(int sid, schib_t *schib, devchar_t *rdc) 
{
	int     rc;
	ccw_t	ioccw[2];
	orb_t	orb;
	irb_t	irb;

	/* 
	 *	Build CCWS for Read Device Characteristics.
	 */

	ioccw[0].flags = CCW_CC | CCW_SLI;  /* Write chained to NOOP + SLI */
	ioccw[0].cmd =   CCW_CMD_RDC;	    /* ccw command is read */
	ioccw[0].count = sizeof(devchar_t); /* length of read bfr */
	ioccw[0].dataptr = rdc;	/* address of read buffer */
	ioccw[1].cmd =   CCW_CMD_NOP;	/* ccw is NOOP */
	ioccw[1].flags = CCW_SLI;	/* Suppress Length Incorrect */
	ioccw[1].dataptr = 0;	/* buffer = 0 */
	ioccw[1].count =   1;


	/* 
	 *	Construct an ORB for the SSCH.
	 */

	memset(&orb,0x00,sizeof(orb_t));	/* clear the ORB */
	orb.intparm = schib->interrupt_parm;
	orb.fpiau = 0x80;	/* format 1 ORB */
	orb.lpm = 0xff;	/* Logical Path Mask */
	orb.ptrccw = &ioccw[0];

	rc = _ssch(sid,&orb);	/* issue Start Subchannel */

	if (rc) {
		return(rc); /* we lost the subchannel - tell caller */
	}

	/* 
	 *	This really is bad form, but we can't handle an I/O 
	 *	interrupt yet.
	 */

	while(1) {
		rc = _stsch(sid,schib);

		if (rc) {
			break; /* lost subchannel - tell caller */
		}

		if (!(schib->scsw.status & 0x1)) {
			udelay (100);	/* busy spin for 100 microsecs */
			continue;
		}

		rc = _tsch(sid,&irb);

		if (rc) {
			break; /* lost subchannel - tell caller */
		}

		if (irb.scsw.status & 0x7) {

			if ((irb.scsw.devstat == 0x0c) &&
				 (irb.scsw.schstat == 0)) {
				rc = 0;
				break;
			}
			else {
			 	rc = 1000;
				break;
			}
		}
		else {
			rc = 1000;
			break;
		}

	}

        return rc;

}

/*
 *	Issue a Read for the Volume label
 */

int     i370_getvol(int sid, schib_t *schib, devchar_t *rdc) 
{
	int     rc;

	ccw_t	ioccw[3];	/* DE, LOCR, READ DATA */
	orb_t	orb;
	irb_t	irb;
	de_t	debuf;		/* Define Extent Structure */
	locr_t	locbuf;		/* Locate Record Structure */
	char 	label[12];

	/* 
	 *	Build CCWS to Read DASD Volume Label. Start with 
	 *	Parameters to Define Extent.
	 */

	debuf.mask = 0x00;
	debuf.globa = 0xc0;
	debuf.recsize = 0x1000;
	debuf.id = 0x0000;
	debuf.gloa = 0x00;
	debuf.gloex = 0x40;
	debuf.deext = 0x00000000;	/* start of extent is cyl 0 hd 0 */
	debuf.deend = 0x00010000;  	/* end of extent is cyl 1 head 0 */

	locbuf.orop = 0x06;		/* Read Data */
	locbuf.rsv = 0x00;		/* Reserved */
	locbuf.aux = 0x00;		/* Aux Byte */
	locbuf.count = 0x01;		/* Read Count is 1 */
	locbuf.seek = 0x00000000;	/* Seek to CCHH 00 00 */
	locbuf.search = 0x00000000;	/* Search CCHH 00 00 */
	locbuf.record = 0x03;		/* Record 3 */
	locbuf.sector = 0xff;		/* Set Sector 0 */
	locbuf.zeros  = 0x0000;		/* Set Must be zeros */


	ioccw[0].flags = CCW_CC;	/* Command is Define Extent */
	ioccw[0].cmd = CCW_CMD_DEXT;	/* ccw command is DE */
	ioccw[0].count = sizeof(de_t);  /* length of DE PARM */
	ioccw[0].dataptr = &debuf;	/* address of read buffer */

	ioccw[1].flags = CCW_CC;	/* Suppress Length Incorrect */
	ioccw[1].cmd = CCW_CMD_LOCR;    /* ccw command is LOCR */
	ioccw[1].dataptr = &locbuf;	/* address of LOCR PARM */
	ioccw[1].count = sizeof(locr_t);

	ioccw[2].flags = CCW_SLI;	/* Suppress LI */
	ioccw[2].cmd = CCW_CMD_RDDATA;	/* ccw command is READ DATA */
	ioccw[2].count = sizeof(label); /* length of label */
	ioccw[2].dataptr = &label[0];	/* address of read buffer */

	/* 
	 *	Construct an ORB for the SSCH.
	 */

	memset(&orb,0x00,sizeof(orb_t));	/* clear the ORB */
	orb.intparm = schib->interrupt_parm;
	orb.fpiau = 0x80;	/* format 1 ORB */
	orb.lpm = 0xff;	/* Logical Path Mask */
	orb.ptrccw = &ioccw[0];

	rc = _ssch(sid,&orb);	/* issue Start Subchannel */

	if (rc) {
		return(rc); /* we lost the subchannel - tell caller */
	}

	/* 
	 *	This really is bad form, but we can't handle an I/O 
	 *	interrupt yet.
	 */

	while(1) {
		memset(&rdc->devvol[0],0xff,6);

		rc = _stsch(sid,schib);
		if (rc) {
			break; /* lost subchannel - tell caller */
		}

		if (!(schib->scsw.status & 0x1)) {
			udelay (100);	/* busy spin for 100 microsecs */
			continue;
		}

		rc = _tsch(sid,&irb);
		if (rc) {
			break; /* lost subchannel - tell caller */
		}

		if (irb.scsw.status & 0x7) {
			if ((irb.scsw.devstat == 0x0c) &&
				 (irb.scsw.schstat == 0)) {
				memcpy(&rdc->devvol[0],&label[4],6);
				rc = 0;
				break;
			}
			else {
			 	rc = 1000;
				break;
			}
		}
		else {
			rc = 1000;
			break;
		}

	}

        return rc;

}


#define	ON	1
#define	OFF	0

unitblk_t *unit_base;
unitblk_t *dev_cons;
long	  cons_sid = 0x0;   
long	sid_count = 0;


void	
i370_find_devices(unsigned long *memory_start, unsigned long memory_end) 
{

	long	sid = 0x00010000;
	long	rc;
	schib_t	schib;
	unitblk_t *dev_cons;
	unitblk_t *devices;
	devchar_t rdc;


	/* 
	 *	Set Unit Block base address to memory start
	 *	and calculate the number of subchannels in the 
	 *	configuration set with device valid.
	 */

	unit_base = (unitblk_t *)*memory_start; 

	while(1) {
		rc = _stsch(sid,&schib);	
		if (rc == 0) {
			if (schib.dval) {
				sid_count++;
			}
			sid++;
		}
		else {
			break;
		}
	}	/* end of while(1) */

	/*
	 *	Bump memory_start by count of valid subchannels.
	 *	Build Unit Blocks for all subchannels with 	
	 *	device valid.
	 */
				
	sid = 0x00010000;	/* back to subch 0 */

	devices = unit_base;

	while(1) {

		rc = _stsch(sid,&schib);	/* store a schib */

		if (rc != 0) {	
			break;		/* drop out of loop on CC3 */
		}
	
		else {

			if (schib.dval) {
				memset((void *)devices,0x00,sizeof(unitblk_t));
				devices->unitsid = sid;
				devices->unitdev = schib.devno;

				schib.enabl = ON;
				schib.interrupt_parm = (int)devices;
				rc = _msch(sid,&schib);
					
				rc = i370_getrdc(sid,&schib,&rdc);	

				/* 
				 * Hard code Console to unit address 3e0.
				 * This needs to change!
	 			 */
		
				if (schib.devno == 0x3e0) {
					dev_cons = devices;
					cons_sid = sid;
				}

				if (!rc) {
				   devices->unitmodl = rdc.devcumod;
				   devices->unitclas = rdc.devclcd;
				   devices->unittycd = rdc.devtycod;
				   memcpy(devices->unitdtyp,&rdc.devtype[0],sizeof(devices->unitdtyp));
				   if (rdc.devcutyp == 0x3990) {
				  	 devices->unitstat = UNIT_READY;
					 rc = i370_getvol(sid,&schib,&rdc);
					 memcpy(&devices->unitvol,rdc.devvol,6);
				   }	
				}
				devices++;
			}
		}

		sid++;
	}

	*memory_start = devices; /* return new memory_start */

	return;
}

/* ============================== END OF FILE ==================== */
