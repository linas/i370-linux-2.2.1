/*
 *  $Id: build.c,v 1.1 1999/02/08 06:21:07 linas Exp $
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 1997 Martin Mares
 */

/*
 * This file builds a disk-image from three different files:
 *
 * - bootsect: exactly 512 bytes of 8086 machine code, loads the rest
 * - setup: 8086 machine code, sets up system parm
 * - system: 80386 code for actual system
 *
 * It does some checking that all files are of the correct type, and
 * just writes the result to stdout, removing headers and padding to
 * the right amount. It also writes some system data to stderr.
 */

/*
 * Changes by tytso to allow root device specification
 * High loaded stuff by Hans Lermen & Werner Almesberger, Feb. 1996
 * Cross compiling fixes by Gertjan van Wingerde, July 1996
 * Rewritten by Martin Mares, April 1997
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/boot.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long u32;

#define DEFAULT_MAJOR_ROOT 0
#define DEFAULT_MINOR_ROOT 0

/* Minimal number of setup sectors (see also bootsect.S) */
#define SETUP_SECTS 4

byte buf[1024];
int fd;
int is_big_kernel;

void die(const char * str, ...)
{
	va_list args;
	va_start(args, str);
	vfprintf(stderr, str, args);
	fputc('\n', stderr);
	exit(1);
}

/* Reading of ld86 output (Minix format) */

#define MINIX_HEADER_LEN 32

void minix_open(const char *name)
{
	static byte hdr[] = { 0x01, 0x03, 0x10, 0x04, 0x20, 0x00, 0x00, 0x00 };
	static u32 *lb = (u32 *) buf;

	if ((fd = open(name, O_RDONLY, 0)) < 0)
		die("Unable to open `%s': %m", name);
	if (read(fd, buf, MINIX_HEADER_LEN) != MINIX_HEADER_LEN)
		die("%s: Unable to read header", name);
	if (memcmp(buf, hdr, sizeof(hdr)) || lb[5])
		die("%s: Non-Minix header", name);
	if (lb[3])
		die("%s: Illegal data segment");
	if (lb[4])
		die("%s: Illegal bss segment");
	if (lb[7])
		die("%s: Illegal symbol table");
}

void usage(void)
{
	die("Usage: build [-b] bootsect setup system [rootdev] [> image]");
}

int main(int argc, char ** argv)
{
	unsigned int i, c, sz, setup_sectors;
	u32 sys_size;
	byte major_root, minor_root;
	struct stat sb;

	if (argc > 2 && !strcmp(argv[1], "-b"))
	  {
	    is_big_kernel = 1;
	    argc--, argv++;
	  }
	if ((argc < 4) || (argc > 5))
		usage();
	if (argc > 4) {
		if (!strcmp(argv[4], "CURRENT")) {
			if (stat("/", &sb)) {
				perror("/");
				die("Couldn't stat /");
			}
			major_root = major(sb.st_dev);
			minor_root = minor(sb.st_dev);
		} else if (strcmp(argv[4], "FLOPPY")) {
			if (stat(argv[4], &sb)) {
				perror(argv[4]);
				die("Couldn't stat root device.");
			}
			major_root = major(sb.st_rdev);
			minor_root = minor(sb.st_rdev);
		} else {
			major_root = 0;
			minor_root = 0;
		}
	} else {
		major_root = DEFAULT_MAJOR_ROOT;
		minor_root = DEFAULT_MINOR_ROOT;
	}
	fprintf(stderr, "Root device is (%d, %d)\n", major_root, minor_root);

	minix_open(argv[1]);				    /* Copy the boot sector */
	i = read(fd, buf, sizeof(buf));
	fprintf(stderr,"Boot sector %d bytes.\n",i);
	if (i != 512)
		die("Boot block must be exactly 512 bytes");
	if (buf[510] != 0x55 || buf[511] != 0xaa)
		die("Boot block hasn't got boot flag (0xAA55)");
	buf[508] = minor_root;
	buf[509] = major_root;
	if (write(1, buf, 512) != 512)
		die("Write call failed");
	close (fd);

	minix_open(argv[2]);				    /* Copy the setup code */
	for (i=0 ; (c=read(fd, buf, sizeof(buf)))>0 ; i+=c )
		if (write(1, buf, c) != c)
			die("Write call failed");
	if (c != 0)
		die("read-error on `setup'");
	close (fd);

	setup_sectors = (i + 511) / 512;		    /* Pad unused space with zeros */
	/* for compatibility with ancient versions of LILO */
	if (setup_sectors < SETUP_SECTS)
		setup_sectors = SETUP_SECTS;
	fprintf(stderr, "Setup is %d bytes.\n", i);
	memset(buf, 0, sizeof(buf));
	while (i < setup_sectors * 512) {
		c = setup_sectors * 512 - i;
		if (c > sizeof(buf))
			c = sizeof(buf);
		if (write(1, buf, c) != c)
			die("Write call failed");
		i += c;
	}

	if ((fd = open(argv[3], O_RDONLY, 0)) < 0)	    /* Copy the image itself */
		die("Unable to open `%s': %m", argv[3]);
	if (fstat (fd, &sb))
		die("Unable to stat `%s': %m", argv[3]);
	sz = sb.st_size;
	fprintf (stderr, "System is %d kB\n", sz/1024);
	sys_size = (sz + 15) / 16;
	if (sys_size > (is_big_kernel ? 0xffff : DEF_SYSSIZE))
		die("System is too big. Try using %smodules.",
			is_big_kernel ? "" : "bzImage or ");
	while (sz > 0) {
		int l, n;

		l = (sz > sizeof(buf)) ? sizeof(buf) : sz;
		if ((n=read(fd, buf, l)) != l) {
			if (n < 0)
				die("Error reading %s: %m", argv[3]);
			else
				die("%s: Unexpected EOF", argv[3]);
		}
		if (write(1, buf, l) != l)
			die("Write failed");
		sz -= l;
	}
	close(fd);

	if (lseek(1, 497, SEEK_SET) != 497)		    /* Write sizes to the boot sector */
		die("Output: seek failed");
	buf[0] = setup_sectors;
	if (write(1, buf, 1) != 1)
		die("Write of setup sector count failed");
	if (lseek(1, 500, SEEK_SET) != 500)
		die("Output: seek failed");
	buf[0] = (sys_size & 0xff);
	buf[1] = ((sys_size >> 8) & 0xff);
	if (write(1, buf, 2) != 2)
		die("Write of image length failed");

	return 0;					    /* Everything is OK */
}
