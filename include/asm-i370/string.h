 /*
        asm/string.h -- String inlines 

  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

  Date: $Id: string.h,v 1.5 1999/11/08 05:08:17 linas Exp $
  Known Bugs: Generally, it is not checked for architecturally given
        length restrictions. 
  Changes: corrected #defines for inlines not provided; corrected
        register usage in memcmp (ptr 1999-09-27)
	Corrected operand template in i370_constant_memcpy, made constant 
	expressions out of constant arguments in bzero, bcopy, memset, 
	changed code for strcmp

 */ 

 /*	  -- N O T E S --
 ---------------------------------------------------------------------------
  .     These functions presently use the String Feature -- therefore
  .     they are conditionalised by CONFIG_STRING_INLINES and
  .     CONFIG_STRING_FEATURE, with the intent to make them standard after 
  .     a sufficient time of debugging and to add more generally useable
  .     versions a bit later. After all, the arch/i370/lib/string.c versions
  .     should be no longer useful; we could try to make these here `static'.
  .     Should rewrite this for extended versions of Move/Compare.
 */
 
#ifndef _I370_STRING_H_
#define _I370_STRING_H_

#define __HAVE_ARCH_STRNCPY	/* in arch/i370/lib/string.c */
//#define __HAVE_ARCH_STRCAT
//#define __HAVE_ARCH_BCOPY
//#define __HAVE_ARCH_MEMMOVE
//#define __HAVE_ARCH_MEMCHR

#include <linux/autoconf.h>
#include <linux/types.h>
#ifdef CONFIG_STRING_INLINES

#define __HAVE_ARCH_MEMCMP
 
extern inline
int memcmp(const void *vs1, const void *vs2, size_t n) 
{ register long  target __asm__ ("r0")  =  (long) vs1;
  register long  length __asm__ ("r1")  =  (long) n;
  register long  source __asm__ ("r14") =  (long) vs2;
 
  __asm__ __volatile__ (".set _i370_implied_op1,%0;\n\t
             .set _i370_implied_op3,%1;\n\t
             .set _i370_implied_op2,%2;\n\t             
             LR   r15,r1;\n\t
             CLCL r0,r14;\n\t
             BZ   3f;\n\t
             BM   1f;\n\t
             LA   r1,1(0,0);\n\t
             B    3f;\n
1:\tBCTR r1,0;
3:\t;"
	   :"+r"(target), "+r"(length), "+r"(source)
	   ::"cc",  "r15" );
  return length;		/* contains now the code */
};

#define __HAVE_ARCH_MEMCPY

#define memcpy(t, s, n) \
if(__builtin_constant_p(n) && ((n) < 256)) { \
       i370_bcopy((t), (s), (n)) ;} \
else { i370_memcpy((t), (s), (n)); }

extern inline
void* i370_memcpy (void *dest, const void *src, __kernel_size_t n)
{
  register long target __asm__ ("r0")  =  (long) dest;
  register long length __asm__ ("r1")  =  (long) n;
  register long source __asm__ ("r14") =  (long) src;
  __asm__ __volatile__(".set _i370_implied_op1,%0;\n\t
             .set _i370_implied_op3,%1;\n\t
             .set _i370_implied_op2,%2;\n\t             
             LR   r15,%1;\n\t
             MVCL r0,r14;"
	   : "+r"(target),  "+r"(length), "+r"(source)
	   ::"cc",  "r0", "r1", "r14", "r15" );
  
   return dest;
};

#define i370_bcopy(t, s, n) \
  __asm__ __volatile__("MVC\t%O0(%B1,%R0),%2" \
                     : "=m"(*(char*)t) : "I"(n+0), "m"(*(char*)s))


#define __HAVE_ARCH_MEMSET
#define memset(t, c, n) \
if(__builtin_constant_p(n) && ((n) < 256)) { \
      if (__builtin_constant_p(c) && ((c) == 0)) { \
	     i370_bzero ((t), (n));} \
      else { i370_constant_memset((t) , (c), (n));}} \
 else { i370_memset((t), (c), (n));}						      

extern inline 
void *i370_memset(void *s, int c, size_t n)
{
  register long target  __asm__ ("0")  =  (long) s;
  register long length  __asm__ ("1")  =  (long) n;
  register long padding __asm__ ("15") =  (long) c << 24;
  	/* r14  ignored */

  __asm__ __volatile__(".set _i370_implied_op1,%0;\n\t
             .set _i370_implied_op3,%1;\n\t
             .set _i370_implied_op2,%2; \n\t            
             MVCL r0,r14;"
	   : "+r"(target), "+r"(length): "r"(padding)
	   :"cc" );
    return s;
}
#define i370_bzero(area, size)\
   __asm__ __volatile__("XC   %O0(%B1,%R0),%0" : "=m"(*(char*)area) : "I"(size+0))

#define i370_constant_memset(area, chr , size)\
   __asm__ __volatile__ ("MVI %0,%B1;\n\tMVC %O0+1(%B2,%R0),%0"\
                        : "=m"(*(char*)area) : "I"(chr+0), "I"(size-1))
#ifdef CONFIG_STRING_FEATURE

#define __HAVE_ARCH_STRCMP

extern inline 
int strcmp(const char *s1, const char *s2)
{
 /*   register int result __asm__("r0");
   __asm__ __volatile__(".set  _i379_implied_op1,%0;
           SLR   r0,r0;
       1:  CLST  %1,%2; 
           BZ    3f; 
           BP    2f;  
           BO    1b; 
           BCTR  r0,r0;
           B     3f;
       2:  LA    r0,1(0,0);
       3:    " : "=r"(result): "r"(s1), "r"(s2));*/
  int result;
  __asm__ __volatile__ (
	 "LA      %0,1(0,0);\n1:\t"
	 "CLST    %1,%2;\n\t"
	 "BP      3f;\n\t"
	 "BM      2f;\n\t"
	 "BO      1b;\n\t"
	 "SLR     %0,%0;\n2:\t"
	 "LCR     %0,%0;\n3:\n"
	 : "=r"(result): "r"(s1), "r"(s2)); 
  return result;
};

#define __HAVE_ARCH_STRCPY

extern inline 
char* strcpy (char *dest, const char *src)
{
  __asm__ __volatile__("1: SLR r0,r0; MVST %0, %1; BO 1b; "
	  :: "r"(dest), "r"(src):
	  "r0" , "cc");
  return dest;
};

#define __HAVE_ARCH_STRLEN

extern inline
size_t strlen (const char *head) 
{
  register int length __asm__("r0");
  __asm__ __volatile__(".set  _i370_implied_op1,%0;
           SLR    r0,r0;
      1:   SRST   r0,%1;
           BO     1b;
           SR     r0,%1"
	  :"=r"(length) : "r"(head));
  return length;
};

#endif /* STRING_INLINES */
#endif /* STRING_FEATURE */

extern int strcasecmp(const char *, const char *);
extern void *memchr (const void *,int, size_t);


#endif /* STRING_H */

