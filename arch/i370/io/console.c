/************************************************************/
/*							  */
/* Module ID  - console.				    */
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

#include <linux/fs.h>

/*================== End of Include Statements =============*/


void i370_cons_open (struct inode *tty, struct file *filp) 
{
	printk ("i370_cons_open\n");
}

void i370_cons_write (struct file *filp, const char *str, size_t len, loff_t n) 
{
	printk ("i370_cons_write: len=%d %s<<<\n", len, str);
}

void
i370_console_driver(void)
{


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
i370_cons_flih(int irq, void *dev_id, struct pt_regs *regs)
{
}

/*===================== End of Mainline ====================*/

struct file_operations i370_fop_cons =
{
   NULL,		 /* lseek - default */
   NULL,		 /* read - general block-dev read */
   i370_cons_write,	 /* write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   i370_cons_open,	 /* open */
   NULL,		 /* flush */
   NULL,		 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*===================== End of Function ====================*/
