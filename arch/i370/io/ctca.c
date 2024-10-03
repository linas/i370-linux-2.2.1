/************************************************************/
/*							  */
/* Module ID  - ctca.				       */
/*							  */
/* Function   - ........................................... */
/*	      ........................................... */
/*	      ........................................... */
/*							  */
/*							  */
/* Parameters - (1) Parmname.			       */
/*		  Description - ......................... */
/*		  ....................................... */
/*		  Access - Read/Write/Update.	     */
/*							  */
/*	      (2) Parmname.			       */
/*		  Description - ......................... */
/*		  ....................................... */
/*		  Access - Read/Write/Update.	     */
/*							  */
/*							  */
/* Called By  - Kernel.				     */
/*							  */
/*							  */
/* Notes      - (1) ....................................... */
/*							  */
/*	      (2) ....................................... */
/*							  */
/*							  */
/* Name       - Neale Ferguson.			     */
/* Date       - August, 1999.			       */
/*							  */
/*							  */
/* Associated    - (1) Refer To ........................... */
/* Documentation					    */
/*		 (2) Refer To ........................... */
/*							  */
/************************************************************/
/************************************************************/
/*							  */
/*		       DEFINES			    */
/*		       -------			    */
/*							  */
/************************************************************/

/*=============== End of Defines ===========================*/

/************************************************************/
/*							  */
/*		INCLUDE STATEMENTS			*/
/*		------------------			*/
/*							  */
/************************************************************/

#include <linux/fs.h>

/*================== End of Include Statements =============*/

/************************************************************/
/*							  */
/*		TYPE DEFINITIONS			  */
/*		----------------			  */
/*							  */
/************************************************************/



/*================== End of Type Definitions ===============*/

/************************************************************/
/*							  */
/*	       FUNCTION PROTOTYPES			*/
/*	       -------------------			*/
/*							  */
/************************************************************/

/*================== End of Prototypes =====================*/

/************************************************************/
/*							  */
/*	     GLOBAL VARIABLE DECLARATIONS		 */
/*	     ----------------------------		 */
/*							  */
/************************************************************/

struct file_operations i370_fop_ctca =
{
   NULL,		 /* lseek - default */
   NULL,		 /* read - general block-dev read */
   NULL,		 /* write - general block-dev write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   NULL,		 /* open */
   NULL,		 /* flush */
   NULL,		 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*============== End of Variable Declarations ==============*/

/************************************************************/
/*							  */
/*		     M A I N L I N E		      */
/*		     ---------------		      */
/*							  */
/************************************************************/

void
i370_ctca_driver(void)
{
	printk("Hello CTCA\n");
}

/*===================== End of Mainline ====================*/

/************************************************************/
/*							  */
/* Name       - Interrupt handler.			  */
/*							  */
/* Function   - This Routine performs the following final   */
/*	      processing ................................ */
/*							  */
/* Parameters - None.				       */
/*							  */
/************************************************************/

void
i370_ctca_flih(int irq, void *dev_id, struct pt_regs *regs)
{
}

/*===================== End of Function ====================*/
