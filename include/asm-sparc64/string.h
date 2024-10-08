/* $Id: string.h,v 1.1.1.1 1999/02/08 06:19:09 linas Exp $
 * string.h: External definitions for optimized assembly string
 *           routines for the Linux Kernel.
 *
 * Copyright (C) 1995,1996 David S. Miller (davem@caip.rutgers.edu)
 * Copyright (C) 1996,1997 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 */

#ifndef __SPARC64_STRING_H__
#define __SPARC64_STRING_H__

/* Really, userland/ksyms should not see any of this stuff. */

#ifdef __KERNEL__

#include <asm/asi.h>

extern void __memmove(void *,const void *,__kernel_size_t);
extern __kernel_size_t __memcpy(void *,const void *,__kernel_size_t);
extern __kernel_size_t __memcpy_short(void *,const void *,__kernel_size_t,long,long);
extern __kernel_size_t __memcpy_entry(void *,const void *,__kernel_size_t,long,long);
extern __kernel_size_t __memcpy_16plus(void *,const void *,__kernel_size_t,long,long);
extern __kernel_size_t __memcpy_384plus(void *,const void *,__kernel_size_t,long,long);
extern __kernel_size_t __memset(void *,int,__kernel_size_t);

#ifndef EXPORT_SYMTAB_STROPS

/* First the mem*() things. */
#define __HAVE_ARCH_BCOPY
#define __HAVE_ARCH_MEMMOVE

#undef memmove
#define memmove(_to, _from, _n) \
({ \
	void *_t = (_to); \
	__memmove(_t, (_from), (_n)); \
	_t; \
})

#define __HAVE_ARCH_MEMCPY

extern inline void *__constant_memcpy(void *to, const void *from, __kernel_size_t n)
{
	if(n) {
		if(n <= 32) {
			__builtin_memcpy(to, from, n);
		} else {
			__memcpy(to, from, n);
		}
	}
	return to;
}

extern inline void *__nonconstant_memcpy(void *to, const void *from, __kernel_size_t n)
{
	__memcpy(to, from, n);
	return to;
}

#undef memcpy
#define memcpy(t, f, n) \
(__builtin_constant_p(n) ? \
 __constant_memcpy((t),(f),(n)) : \
 __nonconstant_memcpy((t),(f),(n)))

#define __HAVE_ARCH_MEMSET

extern inline void *__constant_memset(void *s, char c, __kernel_size_t count)
{
	extern __kernel_size_t __bzero(void *, __kernel_size_t);

	if(!c)
		__bzero(s, count);
	else
		__memset(s, c, count);
	return s;
}

extern inline void *__nonconstant_memset(void *s, char c, __kernel_size_t count)
{
	__memset(s, c, count);
	return s;
}

#undef memset
#define memset(s, c, count) \
(__builtin_constant_p(c) ? \
 __constant_memset((s), (c), (count)) : \
 __nonconstant_memset((s), (c), (count)))

#define __HAVE_ARCH_MEMSCAN

#undef memscan
#define memscan(__arg0, __char, __arg2)						\
({										\
	extern void *__memscan_zero(void *, size_t);				\
	extern void *__memscan_generic(void *, int, size_t);			\
	void *__retval, *__addr = (__arg0);					\
	size_t __size = (__arg2);						\
										\
	if(__builtin_constant_p(__char) && !(__char))				\
		__retval = __memscan_zero(__addr, __size);			\
	else									\
		__retval = __memscan_generic(__addr, (__char), __size);		\
										\
	__retval;								\
})

#define __HAVE_ARCH_MEMCMP

/* Now the str*() stuff... */
#define __HAVE_ARCH_STRLEN

extern __kernel_size_t __strlen(const char *);

#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
extern __kernel_size_t strlen(const char *);
#else /* !EGCS */
/* Ugly but it works around a bug in our original sparc64-linux-gcc.  */
#undef strlen
#define strlen(__arg0)					\
({	int __strlen_res = __strlen(__arg0) + 1;	\
	__strlen_res -= 1;				\
	__strlen_res;					\
})
#endif /* !EGCS */

#define __HAVE_ARCH_STRNCMP

extern int __strncmp(const char *, const char *, __kernel_size_t);

extern inline int __constant_strncmp(const char *src, const char *dest, __kernel_size_t count)
{
	register int retval;
	switch(count) {
	case 0: return 0;
	case 1: return (src[0] - dest[0]);
	case 2: retval = (src[0] - dest[0]);
		if(!retval && src[0])
		  retval = (src[1] - dest[1]);
		return retval;
	case 3: retval = (src[0] - dest[0]);
		if(!retval && src[0]) {
		  retval = (src[1] - dest[1]);
		  if(!retval && src[1])
		    retval = (src[2] - dest[2]);
		}
		return retval;
	case 4: retval = (src[0] - dest[0]);
		if(!retval && src[0]) {
		  retval = (src[1] - dest[1]);
		  if(!retval && src[1]) {
		    retval = (src[2] - dest[2]);
		    if (!retval && src[2])
		      retval = (src[3] - dest[3]);
		  }
		}
		return retval;
	case 5: retval = (src[0] - dest[0]);
		if(!retval && src[0]) {
		  retval = (src[1] - dest[1]);
		  if(!retval && src[1]) {
		    retval = (src[2] - dest[2]);
		    if (!retval && src[2]) {
		      retval = (src[3] - dest[3]);
		      if (!retval && src[3])
		        retval = (src[4] - dest[4]);
		    }
		  }
		}
		return retval;
	default:
		retval = (src[0] - dest[0]);
		if(!retval && src[0]) {
		  retval = (src[1] - dest[1]);
		  if(!retval && src[1]) {
		    retval = (src[2] - dest[2]);
		    if(!retval && src[2])
		      retval = __strncmp(src+3,dest+3,count-3);
		  }
		}
		return retval;
	}
}

#undef strncmp
#define strncmp(__arg0, __arg1, __arg2)	\
(__builtin_constant_p(__arg2) ?	\
 __constant_strncmp(__arg0, __arg1, __arg2) : \
 __strncmp(__arg0, __arg1, __arg2))

#endif /* !EXPORT_SYMTAB_STROPS */

#endif /* __KERNEL__ */

#endif /* !(__SPARC64_STRING_H__) */
