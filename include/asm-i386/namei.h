/* $Id: namei.h,v 1.1 1999/02/08 06:18:51 linas Exp $
 * linux/include/asm-i386/namei.h
 *
 * Included from linux/fs/namei.c
 */

#ifndef __I386_NAMEI_H
#define __I386_NAMEI_H

/* This dummy routine maybe changed to something useful
 * for /usr/gnemul/ emulation stuff.
 * Look at asm-sparc/namei.h for details.
 */

#define __prefix_lookup_dentry(name, lookup_flags) \
        do {} while (0)

#endif /* __I386_NAMEI_H */
