/************************************************************/
/* Generic driver for 3210                                  */
/************************************************************/

#include <linux/fs.h>

int i370_tty3210_open (struct inode *tty, struct file *filp)
{
	printk ("i370_cons_open\n");
	return 0;
}

ssize_t i370_tty3210_write (struct file *filp, const char *str,
                            size_t len, loff_t *noidea)
{
	printk ("i370_cons_write: len=%ld %s<<<\n", len, str);
	return 0;
}

void
i370_tty3210_driver(void)
{
}

void
i370_tty3210_flih(int irq, void *dev_id, struct pt_regs *regs)
{
}

/*===================== End of Mainline ====================*/

struct file_operations i370_fop_tty3210 =
{
   NULL,		 /* lseek - default */
   NULL,		 /* read - general block-dev read */
   i370_tty3210_write,	 /* write */
   NULL,		 /* readdir - bad */
   NULL,		 /* poll */
   NULL,		 /* ioctl */
   NULL,		 /* mmap */
   i370_tty3210_open,	 /* open */
   NULL,		 /* flush */
   NULL,		 /* release */
   NULL,		 /* fsync */
   NULL,		 /* fasync */
   NULL,		 /* check_media_change */
   NULL,		 /* revalidate */
};

/*===================== End of Function ====================*/
