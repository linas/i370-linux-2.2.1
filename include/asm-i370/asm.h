
/*
 * wrappers for misc assembly
 * Linas Vepstas February 1999
 * Copyright (C) Linas Vepstas 1999
 */

#ifndef __I370_ASM_H__
#define __I370_ASM_H__

#ifndef __ASSEMBLY__  

/* -------------------------------------------------------- */
/* get the current value of the stack pointer */
/* since this inlines, it will basically copy r13 to where-ever */
extern inline unsigned long _get_SP(void)
{
        unsigned long rc;
        asm volatile ("LR	%0,sp" : "=r" (rc) : );
        return rc;
}

/* set the current value of the stack pointer */
/* use with caution */
extern inline void _set_SP (unsigned long newsp)
{
        asm volatile ("LR	sp,%0" : : "r" (newsp) : "memory");
}

/* -------------------------------------------------------- */
/* get the current value of the tca pointer */
/* since this inlines, it will basically copy r12 to where-ever */
extern inline unsigned long _get_TCA(void)
{
        unsigned long rc;
        asm volatile ("LR	%0,rtca" : "=r" (rc) : );
        return rc;
}

/* set the current value of the tca pointer */
/* use with caution */
extern inline void _set_TCA (unsigned long newtca)
{
        asm volatile ("LR	rtca,%0" : : "r" (newtca) : "memory");
}

/* -------------------------------------------------------- */
/* copy fpregs to memory */
extern inline void _store_fpregs (double *memy)
{
        asm volatile (
		"STD	f0,%0;"
		"STD	f2,%1;"
		"STD	f4,%2;"
		"STD	f6,%3;"
		: "=m" (memy[0]), 
		  "=m" (memy[1]),
		  "=m" (memy[2]),
		  "=m" (memy[3]) );
}

/* copy mem to fpregs */
extern inline void _load_fpregs (double *memy)
{
        asm volatile (
		"LD	f0,%0;"
		"LD	f2,%1;"
		"LD	f4,%2;"
		"LD	f6,%3;"
		: : "m" (memy[0]), 
		    "m" (memy[1]),
		    "m" (memy[2]),
		    "m" (memy[3]) );
}

/* -------------------------------------------------------- */
/* the 64-bit clock words need to be 8-byte aligned.  But
 * it seems that the __attribute__ ((aligned (8)) doesn't
 * work on the stack.  So force alignment the hard way.
 */
/* Store Clock */
extern inline unsigned long long _stck (void)
{
   // unsigned long long tickee __attribute__ ((aligned (8)));
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *tockee;
   ptr = ((ptr+4) >> 3) << 3;
   tockee = (unsigned long long *) ptr;
   asm volatile ("STCK	%0" : "=m" (*tockee) );
   return *tockee;
}

/* Store Clock Comparator */
extern inline unsigned long long _stckc (void)
{
   // unsigned long long tickee __attribute__ ((aligned (8)));
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *tockee;
   ptr = ((ptr+4) / 8) * 8;
   tockee = (unsigned long long *) ptr;
   asm volatile ("STCKC	%0" : "=m" (*tockee) );
   return *tockee;
}

/* Set Clock Comparator */
extern inline void _sckc (unsigned long long tickee)
{
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *tockee;
   ptr = ((ptr+4) >> 3) << 3;
   tockee = (unsigned long long *) ptr;
   *tockee = tickee;
   asm volatile ("SCKC	%0" : : "m" (*tockee) );
}

/* -------------------------------------------------------- */
/* set storage key extended */
extern inline void _sske (unsigned int key, unsigned int realaddr)
{
   /* be sure to serialize memory access around this instruction */
   asm volatile ("SSKE	%0,%1" : : "r" (key), "r" (realaddr) : "memory");
}

/* examine storage key extended */
extern inline unsigned int _iske (unsigned int realaddr)
{
   unsigned int key;
   asm volatile ("ISKE	%0,%1" : "=r" (key) : "r" (realaddr) );
   return key;
}

/* set psw key from address */
extern inline void _spka (unsigned int key)
{
   asm volatile ("SPKA	0(%0)" : : "r" (key));
}

/* -------------------------------------------------------- */

extern inline void _lpsw (unsigned long long psw)
{
   asm volatile ("LPSW	%0" : : "m" (psw) : "memory");
}

/* -------------------------------------------------------- */
/* load control registers */

#define _lctl(REGNO,val) _lctl_r##REGNO (val)

#define def_lctl(REGNO)						\
extern inline void _lctl_r##REGNO (unsigned int value)		\
{								\
   /* be sure to serialize memory access around this instruction */\
   asm volatile ("LCTL  " #REGNO "," #REGNO ",%0"  		\
                   : : "m" (value) : "memory");			\
}

def_lctl(0)
def_lctl(1)
def_lctl(2)
def_lctl(3)
def_lctl(4)
def_lctl(5)
def_lctl(6)
def_lctl(7)
def_lctl(8)
def_lctl(9)
def_lctl(10)
def_lctl(11)
def_lctl(12)
def_lctl(13)
def_lctl(14)
def_lctl(15)

/* -------------------------------------------------------- */
/* Store Processor Address */
extern inline unsigned long _stap (void)
{
   unsigned short cpuid;
   asm volatile ("STAP	%0" : "=m" (cpuid) );
   return ((unsigned long) cpuid);
}

/* -------------------------------------------------------- */
/* Purge TLB */
extern inline void _ptlb (void)
{
   asm volatile ("PTLB" : : : "memory");
}

/* -------------------------------------------------------- */
/* Halt the system */

extern inline void i370_halt (void)
{
   // need to have 8-byte alignment for the psw
   // unsigned long long psw __attribute__ ((aligned (8)));
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *psw;
   ptr = ((ptr+4) / 8) * 8;
   psw = (unsigned long long *) ptr;
   asm volatile (
	"	L	r1,=X'000a0000';	"
	"	ST	r1,%0;			"
	"	L	r1,=A(.Lcontinue):	"
	"	O	r1,=X'80000000';	"
	"	ST	r1,4+%0;		"
	"	LPSW	%0;			"
	".Lcontinue:				"
	"	NOPR	r0;			"
	: "+m" (*psw) : : "r1", "memory");
}

#endif /* __ASSEMBLY__ */
#endif /* __I370_ASM_H__ */
