/************************************************************/
/*                                                          */
/* Function   - Discover and configure S/390 I/O devices    */
/*                                                          */
/* Name       - Neale Ferguson.                             */
/* Date       - August, 1999.                               */
/*                                                          */
/************************************************************/

#define ON     1
#define OFF    0

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tasks.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/fs.h>

#include <asm/delay.h>
#include <asm/iorb.h>
#include <asm/irq.h>
#include <asm/sense.h>
#include <asm/unitblk.h>

static int i370_doio(int, schib_t *, ccw_t *);
static int i370_getsid(int, schib_t *, idchar_t *);
static int i370_getrdc(int, schib_t *, devchar_t *);
static int i370_getvol_eckd(int, schib_t *, devchar_t *);
static void i370_configure_device(long, schib_t *, unitblk_t *, idchar_t *);

unitblk_t *unit_base;
unitblk_t *dev_cons    = NULL,
          *dev_con3210 = NULL,
          *dev_con3270 = NULL,
          *dev_tty3210 = NULL;

long    sid_count = 0;
extern unsigned char* CPUID;

extern struct file_operations i370_fop_eckd;
extern struct file_operations i370_fop_ckd;
extern struct file_operations i370_fop_fba;
extern struct file_operations i370_fop_graf;
extern struct file_operations i370_fop_tty3210;
extern struct file_operations i370_fop_tape;
extern struct file_operations i370_fop_tss;
extern struct file_operations i370_fop_osa;
extern struct file_operations i370_fop_ctca;

extern void i370_eckd_flih (int, void *, struct pt_regs *regs);
extern void i370_ckd_flih  (int, void *, struct pt_regs *regs);
extern void i370_fba_flih  (int, void *, struct pt_regs *regs);
extern void i370_graf_flih (int, void *, struct pt_regs *regs);
extern void i370_tty3210_flih (int, void *, struct pt_regs *regs);
extern void i370_tape_flih (int, void *, struct pt_regs *regs);
extern void i370_tss_flih  (int, void *, struct pt_regs *regs);
extern void i370_osa_flih  (int, void *, struct pt_regs *regs);
extern void i370_ctca_flih (int, void *, struct pt_regs *regs);

S390map_t s390_map[10] = {
	{T3990, 0,     0,  0},
	{T3880, 0,     0,  1},
	{TFBLK, 0,     0,  2},
	{T3274, 0,     0,  3},
	/* {Txxxx, 0,     0,  4}, hard-coded as device 0009 system console */
	{T3480, 0,     0,  5},
	{T3590, 0,     0,  6},
	{T3172, MOSAD, 0,  7},
	{T3172, MCTCA, 0,  8},
	{T3215, 0,     0,  9}, /* non-system console 3210's */
	{0,     0,     0, -1}
};

#define I_CONS 4 /* Index into s390_devices (below) for system console */

S390dev_t s390_devices[11] = {
	{MJ3990, 0, 255, BLKDEV, D3990, &i370_fop_eckd,  7, i370_eckd_flih, NULL},
	{MJ3880, 0, 255, BLKDEV, D3880, &i370_fop_ckd,   6, i370_ckd_flih, NULL},
	{MJFBLK, 0, 255, BLKDEV, DFBLK, &i370_fop_fba,   6, i370_fba_flih, NULL},
	{MJ3274, 0, 255, CHRDEV, D3274, &i370_fop_graf,  1, i370_graf_flih, NULL},
	{MJCONS, 1, 2,   CHRDEV, DCONS, &i370_fop_tty3210, 1, i370_tty3210_flih, &dev_tty3210},
	{MJ3480, 0, 255, CHRDEV, D3480, &i370_fop_tape,  2, i370_tape_flih, NULL},
	{MJ3590, 0, 255, BLKDEV, D3590, &i370_fop_tss,   5, i370_tss_flih, NULL},
	{MJ3172, 0, 255, BLKDEV, D3172, &i370_fop_osa,   4, i370_osa_flih, NULL},
	{MJCTCA, 0, 255, BLKDEV, DCTCA, &i370_fop_ctca,  3, i370_ctca_flih, NULL},
	{MJ3210, 0, 255, CHRDEV, D3210, &i370_fop_tty3210,  1, i370_tty3210_flih, &dev_tty3210},
	{-1,    -1,  -1,     -1, {0, NULL},   NULL,            0, NULL}
};

/************************************************************/
/*                                                          */
/*                     M A I N L I N E                      */
/*                     ---------------                      */
/*                                                          */
/************************************************************/

__initfunc(void
i370_find_devices(unsigned long *memory_start, unsigned long memory_end))
{

	long    sid = 0x00010000;
	long    ipl_sid = *((long *) PFX_SUBSYS_ID);
	long    rc;
	schib_t schib;
	unitblk_t *devices;
	idchar_t dev_id;
	irb_t irb;

	/*------------------------------------------------------*/
	/* The IPL device may still be have status pending,     */
	/* thus clear any pending status and set it offline.    */
	/* We'll re-enable during our main initialization loop. */
	/*------------------------------------------------------*/
	_tsch(ipl_sid, &irb);
	_stsch(ipl_sid, &schib);
	schib.enabl = OFF;
	_msch(ipl_sid, &schib);

	/*------------------------------------------------------*/
	/* Set Unit Block base address to memory start	        */
	/* and calculate the number of subchannels in the       */
	/* configuration set with device valid.                 */
	/*------------------------------------------------------*/

	unit_base = (unitblk_t *) *memory_start;

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
	}

	/*------------------------------------------------------*/
	/* Bump memory_start by count of valid subchannels.     */
	/* Build Unit Blocks for all subchannels with	        */
	/* device valid.                                        */
	/*------------------------------------------------------*/

	/* sid = Sense ID */
	sid = 0x00010000;       /* back to subch 0 */

	devices = unit_base;

	while(1) {

		rc = _stsch(sid,&schib);	/* store a schib */

		if (rc != 0)
			break;	  /* drop out of loop on CC3 */

		if (schib.dval) {
			memset((void *)devices,0x00,sizeof(unitblk_t));
			devices->unitsid = sid;
			devices->unitdev = schib.devno;

			schib.enabl	  = ON;
			schib.interrupt_parm = (int)devices;
			_msch(sid, &schib);

			/*----------------------------------------------------*/
			/* If we can find a device '0009' under VM, then map  */
			/* it to /dev/console, Major, minor (5, 1)            */
			/* Other 3210's will map to /dev/ttyN (4, N+1)        */
			/*                                                    */
			/* VM has (CPUID[0] == 0xff)                          */
			/* and Hercules has (CPUID[0] != 0xff) in general     */
			/*----------------------------------------------------*/
			if ((schib.devno == 0x0009) && (CPUID[0] != 0x0)) {
				dev_cons	      = devices;
				dev_con3210	   = devices;
				schib.isc	   = devices->unitisc = s390_devices[I_CONS].isc;
				devices->unittype  = s390_devices[I_CONS].drvType;
				devices->unitmajor = s390_devices[I_CONS].major;
				devices->unitminor = s390_devices[I_CONS].curMinor;
				devices->unitirqh  = s390_devices[I_CONS].irqh;
				strncpy(devices->unitname,
					s390_devices[I_CONS].devName, DEVNAMELEN-1);
				devices->unitname[DEVNAMELEN] = 0x0;
				devices->unitstat  = UNIT_READY;

				devices->unitfops = s390_devices[I_CONS].fops;
				devices->unitirqh = s390_devices[I_CONS].irqh;

				rc = i370_getsid(sid, &schib, &dev_id);
				printk ("Device 0009 CU ID %04X  Model %02X  %sready\n",
					dev_id.idcuid, dev_id.idcumdl, rc ? "not ":"");
				printk ("Device 0009 mapped to unix /dev/%s (%d, %d)\n",
					devices->unitname, devices->unitmajor, devices->unitminor);
			}
			else {
				rc = i370_getsid(sid, &schib, &dev_id);
				printk("Device %04X found CU ID %04X  Model %02X  %sready\n",
					schib.devno, dev_id.idcuid, dev_id.idcumdl, rc ? "not ":"");

				if (!rc) i370_configure_device(sid, &schib, devices, &dev_id);
			}
			devices++;
		}
		sid++;
	}
	devices += 1;
	*memory_start = (unsigned long) devices; /* return new memory_start */

	return;
}

/* Loop over the devices found previously, early during setup_arch,
 * and register drivers for each. */
void
i370_setup_devices(void)
{
	int i, rc;
	unitblk_t *devices = unit_base;

	for (i=0; i< sid_count; i++) {

		if (UNIT_READY != devices->unitstat) {
			devices++;
			continue;
		}

		printk ("i370 register /dev/%s (%c %d %d)\n",
			devices->unitname, devices->unittype == CHRDEV? 'c':'b',
			devices->unitmajor, devices->unitminor);
		if (devices->unittype == CHRDEV)
		{
			rc = register_chrdev(devices->unitmajor,
					     devices->unitname,
					     devices->unitfops);
		}
		else if (devices->unittype == BLKDEV)
		{
			rc = register_blkdev(devices->unitmajor,
					     devices->unitname,
					     devices->unitfops);
		}
		else
		{
			printk("Bad device type for /dev/%s\n", devices->unitname);
		}
		printk("i370 register rc=%d\n", rc);

#if CANT_DO_THIS_HERE
		// Too late for this; _msch is expecting real address, but we
		// are now running with page translation turned on.
		if (rc < 0) {
			schib->enabl = OFF;
			printk("Unable to register device %08X. Rc: %dl\n",
			       schib->devno, rc);
			_msch(sid,schib);
		}
#endif
		devices++;
	}
}

/*===================== End of Mainline ====================*/

/************************************************************/
/*                                                          */
/* Name       - i370_doio.                                  */
/*                                                          */
/* Function   - Construct an ORB for the SSCH, issue and    */
/*              wait for completion.                        */
/*                                                          */
/************************************************************/

__initfunc(static int
i370_doio(int sid, schib_t *schib, ccw_t *ioccw))
{
	int   rc;
	orb_t orb;
	irb_t irb;

	memset(&orb,0x00,sizeof(orb_t));	/* clear the ORB */
	orb.intparm = schib->interrupt_parm;
	orb.fpiau = 0x80;	/* format 1 ORB */
	orb.lpm = 0xff;	/* Logical Path Mask */
	orb.ptrccw = &ioccw[0];

	rc = _ssch(sid,&orb);	/* issue Start Subchannel */

	if (rc)
		return rc; /* we lost the subchannel - tell caller */

	/*
	 *	This really is bad form, but we can't handle an I/O
	 *	interrupt yet.
	 */

	while(1) {
		rc = _stsch(sid,schib);

		if (rc)
			return rc; /* lost subchannel - tell caller */

		if (!(schib->scsw.status & 0x1)) {
			udelay (100);	/* busy spin for 100 microsecs */
			continue;
		}

		rc = _tsch(sid,&irb);

		if (rc)
			return rc; /* lost subchannel - tell caller */

		if (irb.scsw.status & 0x7) {

			/* Hmm what does irb.scsw.devstat == 0xe mean? */
			if (((irb.scsw.devstat & 0x0c) == 0x0c) &&
				 (irb.scsw.schstat == 0))
				return 0;
			else
				return 500;
		}
		else
			return 1000;
	}
	return 0; /* notreached */
}

/************************************************************/
/*                                                          */
/* Name       - i370_getsid.                                */
/*                                                          */
/* Function   - Issue a Sense ID to a specific subchannel.  */
/*           When supported, a device will return its       */
/*           control unit and model information.            */
/*                                                          */
/************************************************************/


__initfunc(static int
i370_getsid(int sid, schib_t *schib, idchar_t *id))
{
	int     rc;
	int     lign_ccw[5];
	ccw_t	*ioccw;

	/* double-word align the ccw's */
	ioccw = (ccw_t *) (((((unsigned long) &lign_ccw[0]) + 7) >>3) << 3);

	/*
	 *	Build CCWS for Sense ID
	 */

	memset(id, 0, sizeof(idchar_t));
	ioccw[0].flags   = CCW_CC | CCW_SLI;
	ioccw[0].cmd     = CCW_CMD_SID;	     /* ccw command is read */
	ioccw[0].count   = sizeof(idchar_t); /* length of read bfr */
	ioccw[0].dataptr = id;	             /* address of read buffer */
	ioccw[1].cmd     = CCW_CMD_NOP;	     /* ccw is NOOP */
	ioccw[1].flags   = CCW_SLI;	     /* Suppress Length Incorrect */
	ioccw[1].dataptr = 0;	             /* buffer = 0 */
	ioccw[1].count   = 1;

	rc = i370_doio(sid, schib, ioccw);

	return rc;
}

/************************************************************/
/*                                                          */
/* Name       - i370_getrdc.                                */
/*                                                          */
/* Function   - Issue a read device characteristics to a    */
/*           specific subchannel. This information          */
/*           is used to auto-configure the device.          */
/*                                                          */
/************************************************************/

__initfunc(static int
i370_getrdc(int sid, schib_t *schib, devchar_t *rdc))
{
	int     rc;
	int     lign_ccw[5];
	ccw_t	*ioccw;

	/* double-word align the ccw's */
	ioccw = (ccw_t *) (((((unsigned long) &lign_ccw[0]) +7) >>3) << 3);

	/*
	 *	Build CCWS for Read Device Characteristics.
	 */

	memset(rdc, 0, sizeof(devchar_t));
	ioccw[0].flags = CCW_CC | CCW_SLI;  /* Write chained to NOOP + SLI */
	ioccw[0].cmd =   CCW_CMD_RDC;	    /* ccw command is read */
	ioccw[0].count = sizeof(devchar_t); /* length of read bfr */
	ioccw[0].dataptr = rdc;	/* address of read buffer */
	ioccw[1].cmd =   CCW_CMD_NOP;	/* ccw is NOOP */
	ioccw[1].flags = CCW_SLI;	/* Suppress Length Incorrect */
	ioccw[1].dataptr = 0;	/* buffer = 0 */
	ioccw[1].count =   1;

	rc = i370_doio(sid, schib, ioccw);

	return rc;
}

/************************************************************/
/*                                                          */
/* Name       - i370_getvol_eckd.                           */
/*                                                          */
/* Function   - Issue a read for the volume label from a    */
/*           ECKD device.                                   */
/*                                                          */
/************************************************************/

__initfunc(static int
i370_getvol_eckd(int sid, schib_t *schib, devchar_t *rdc))
{
	int     rc;

	orb_t	orb;
	irb_t	irb;
	de_t	debuf;		/* Define Extent Structure */
	locr_t	locbuf;		/* Locate Record Structure */
	char 	label[12];
	int     lign_ccw[7];
	ccw_t	*ioccw;	        /* DE, LOCR, READ DATA */

	/* double-word align the ccw's */
	ioccw = (ccw_t *) (((((unsigned long) &lign_ccw[0]) +7) >>3) << 3);

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
				memcpy(&rdc->devvol[0],&label[4],sizeof(&rdc->devvol));
				memcpy(&rdc->devvtype[0],&label[0],sizeof(&rdc->devtype));
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

/************************************************************/
/*                                                          */
/* Name       - i370_configure_device.                      */
/*                                                          */
/* Function   - Configure a valid device. Assign Linux      */
/*           major/minor device numbers.                    */
/*                                                          */
/************************************************************/

__initfunc(static void
i370_configure_device(long sid, schib_t *schib,
                      unitblk_t *devices, idchar_t *dev_id))
{
	int i_map, i_dev, rc;
	devchar_t rdc;

	/*----------------------------------------------------------*/
	/* Map the control unit ID to a major node entry            */
	/*----------------------------------------------------------*/
	for (i_map = 0; s390_map[i_map].cuid != 0; i_map++) {
		if ((s390_map[i_map].cuid == dev_id->idcuid) &&
		   ((s390_map[i_map].model == 0) ||
		    (s390_map[i_map].model == dev_id->idcumdl)))
			break;
	}

	/*----------------------------------------------------------*/
	/* Use the major node entry to register the device          */
	/*----------------------------------------------------------*/
	i_dev = s390_map[i_map].i_s390dev;
	if ((i_dev > -1) &&
	    (s390_devices[i_dev].curMinor < s390_devices[i_dev].maxMinor))
	{
		schib->isc = devices->unitisc = s390_devices[i_dev].isc;
		devices->unittype  = s390_devices[i_dev].drvType;
		devices->unitmajor = s390_devices[i_dev].major;
		devices->unitminor = s390_devices[i_dev].curMinor;
		devices->unitfops  = s390_devices[i_dev].fops;
		devices->unitirqh  = s390_devices[i_dev].irqh;
		devices->unitisc   = s390_devices[i_dev].isc;
		sprintf(devices->unitname, "%s%d",  /* want snprintf here. */
			s390_devices[i_dev].devName,
			s390_devices[i_dev].curMinor++);

		/* A generic variant of above. */
		*s390_devices[i_dev].unib = devices;

		printk ("Device %04X mapped to unix /dev/%s (%d, %d)\n",
			schib->devno,
			devices->unitname, devices->unitmajor, devices->unitminor);

		/*----------------------------------------------------*/
		/* We flag the first 3270 device as our console. This */
		/* can be overridden by specification in the IPL load */
		/* parameters. (This maps to linux /dev/graf)         */
		/*----------------------------------------------------*/
		if ((dev_id->idcuid == T3274) && (dev_con3270 == NULL)) {
			dev_con3270 = devices;
			if (dev_cons == NULL) {
				printk("This will be the system console\n");
				dev_cons = devices;
			}
		}

		rc = i370_getrdc(sid, schib, &rdc);
		if (!rc) {
			devices->unitmodl = rdc.devcumod;
			devices->unitclas = rdc.devclcd;
			devices->unittycd = rdc.devtycod;
			devices->unitdtyp = rdc.devtype;

			if (rdc.devcutyp == 0x3990) {
				 rc = i370_getvol_eckd(sid, schib, &rdc);
				 memcpy(&devices->unitvol,rdc.devvol,6);
			}
			devices->unitstat = UNIT_READY;
		} else {
			devices->unitcuid = dev_id->idcuid;
			devices->unitmodl = dev_id->idcumdl;
			devices->unitdtyp = dev_id->iddevid;
			devices->unitstat = 0; /* Not ready */
		}
	}
}

/* ============================== END OF FILE ==================== */
