 /*
        string.h -- String inlines 

  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

  Date: $Id: string.h,v 1.3 1999/09/19 17:22:29 linas Exp $
  Known Bugs: Generally, it is not checked for architecturally given
        length restrictions. 
  Changes:

 */ 

 /*	  -- N O T E S --
 ---------------------------------------------------------------------------
  .     These functions presently use the String Feature -- therefore
  .     they are conditionalised by CONFIG_STRING_INLINES and
  .     CONFIG_STRING_FEATURE, with the intent to make them standard after 
  .     a sufficient time of debugging and to add more generally useable
  .     versions a bit later. After all, the arch/i370/lib/string.c versions
  .     should be no longer useful; we could try to make these here `static'.
  .     The __asm__ interface is not fully satisfactoring, if improvement
  .     should be possible, 
 */
 
#ifndef _I370_STRING_H_
#define _I370_STRING_H_
#define __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRNCPY
#define __HAVE_ARCH_STRLEN
#define __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRCAT
#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_BCOPY
#define __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMCMP
#define __HAVE_ARCH_MEMCHR

#include <linux/autoconf.h>

#ifdef CONFIG_STRING_INLINES

 
extern inline
int memcmp(const void *vs1, const void *vs2, size_t n) 
{ register long  target __asm__ ("r0")  =  (long) vs1;
  register long  length __asm__ ("r1")  =  (long) n;
  register long  source __asm__ ("r14") =  (long) vs2;
 
  __asm__ (" .set _i370_implied_op1,%0;
             .set _i370_implied_op3,%1;
             .set _i370_implied_op2,%2;             
             LR   r15,r1;
             CLCL r0,r14;
             BZ   3f;
             BM   1f;
             LA   r1,1(0,0);
             B    3f;
        1:   BCTR r1,0;
        3: "  
	   ::"r"(target), "r"(length), "r"(source)
	   :"cc",  "r15" );
  return length;		/* contains now the code */
};

extern inline
void* memcpy (void *dest, const void *src, size_t n)
{
  register long target __asm__ ("r0")  =  (long) dest;
  register long length __asm__ ("r1")  =  (long) n;
  register long source __asm__ ("r14") =  (long) src;
  __asm__ (" .set _i370_implied_op1,%0;
             .set _i370_implied_op3,%1;
             .set _i370_implied_op2,%2;             
             LR   r15,%0;
             MVCL r0,r14;"
	   :: "r"(target),  "r"(length), "r"(source)
	   :"cc",  "r0", "r1", "r15" );
  
   return dest;
};

							   
extern inline 
void *memset(void *s, int c, size_t n)
{
  register long target  __asm__ ("0")  =  (long) s;
  register long length  __asm__ ("1")  =  (long) n;
  register long padding __asm__ ("15") =  (long) c << 24;
  	/* not used :  r14 */

  __asm__ (" .set _i370_implied_op1,%0;
             .set _i370_implied_op3,%1;
             .set _i370_implied_op2,%2;             
             MVCL r0,r14;"
	   :: "r"(target), "r"(length), "r"(padding)
	   :"cc" );
    return s;
}

#ifdef CONFIG_STRING_FEATURE

extern inline 
int strcmp(const char *s1, const char *s2)
{
   register int result __asm__("r0");
  __asm__(".set  _i379_implied_op1,%0;
           SLR   r0,r0;
       1:  CLST  %1,%2; 
           BZ    3f; 
           BP    2f;  
           BO    1b; 
           BCTR  r0,r0;
           B     3f;
       2:  LA    r0,1(0,0);
       3:    " : "=r"(result): "r"(s1), "r"(s2));
  return result;
};


extern inline 
char* strcpy (char *dest, const char *src)
{
  __asm__("1: SLR r0,r0; MVST %0, %1; BO 1b; "
	  :: "r"(dest), "r"(src):
	  "r0" , "cc");
  return dest;
};

extern inline
size_t strlen (const char *head) 
{
  register int length __asm__("r0");
  __asm__(".set  _i370_implied_op1,%0;
           SLR    r0,r0;
      1:   SRST   r0,%1;
           BO     1b;
           SR     r0,%1"
	  :"=r"(length) : "r"(head));
  return length;
};

#endif // STRING_INLINES
#endif // STRING_FEATURE

extern int strcasecmp(const char *, const char *);

#endif // STRING_H






