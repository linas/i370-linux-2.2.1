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

#include <asm/delay.h>
#include <asm/iorb.h>
#include <asm/spinlock.h>
#include <asm/unitblk.h>

#include <linux/fs.h>

/************************************************************/
/*							  */
/*		     M A I N L I N E		      */
/*		     ---------------		      */
/*							  */
/************************************************************/

/* FIXME. The IOCCW needs to stay unclobbered until the I/O operation
 * has completed, i.e. until after we get the secondary status interrupt.
 * Until then, we should be careful not to clobber this!
 * The current design is to just spin.
 */
static int align_ccw[5];
ccw_t *ioccw;

int i370_ctca_open (struct inode *inode, struct file *filp)
{
   printk ("ctca_open of /dev/%s (c %d %d)\n",
           filp->f_dentry->d_iname,
           inode->i_rdev >> 8, inode->i_rdev & 0xff);

	/* double-word align the ccw array */
	ioccw = (ccw_t *) (((((unsigned long) &align_ccw[0]) + 7) >>3) << 3);
	return 0;
}

ssize_t i370_ctca_write (struct file *filp, const char *str,
                         size_t len, loff_t *ignore)
{
	return 0;
}

ssize_t i370_ctca_read (struct file *filp, char *str,
                        size_t len, loff_t *ignore)
{
	return 0;
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

/*================== End of Handler ========================*/

struct file_operations i370_fop_ctca =
{
   NULL,		 /* lseek - default */
   i370_ctca_read,	 /* read - general block-dev read */
   i370_ctca_write,	 /* write - general block-dev write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   i370_ctca_open, /* open */
   NULL,		 /* flush */
   NULL,		 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*===================== End of Function ====================*/
