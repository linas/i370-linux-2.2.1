
#ifndef __cmsfiles__
#define __cmsfiles__

/* cms_fileio_read	Read a CMS file from specified mini disk

   The file is read just as is and stored in memory at specified spot. 
   Right now it only does 4K blocks and the file should be recfm F.
*/

extern long cms_fileio_read
	(short devno,
	 unsigned char *fspec,
	 unsigned long *mem_start,
	 unsigned long *mem_end,
	 long * file_size);

#endif
