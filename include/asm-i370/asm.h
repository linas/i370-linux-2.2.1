
/*
 * wrappers for misc assembly
 * Linas Vepstas February 1998
 * Copyright (C) Linas Vepstas 1999
 */

/* -------------------------------------------------------- */
/* get the current value of the stack pointer */
/* since this inlines, it will basically copy r13 to where-ever */
extern inline unsigned long _get_SP(void)
{
        unsigned long rc;
        asm volatile ("LA	%0,0(,sp)" : "=r" (rc) : );
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
        asm volatile ("LA	%0,0(,tca)" : "=r" (rc) : );
        return rc;
}

/* set the current value of the tca pointer */
/* use with caution */
extern inline void _set_TCA (unsigned long newtca)
{
        asm volatile ("LR	sp,%0" : : "r" (newtca) : "memory");
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
/* Store Clock */
extern inline unsigned long long _stck (void)
{
   unsigned long long tickee __attribute__ ((aligned (8)));
   asm volatile ("STCK	%0" : "=m" (tickee) );
   return tickee;
}

/* Store Clock Comparator */
extern inline unsigned long long _stckc (void)
{
   unsigned long long tickee __attribute__ ((aligned (8)));
   asm volatile ("STCKC	%0" : "=m" (tickee) );
   return tickee;
}

/* Set Clock Comparator */
extern inline void _sckc (unsigned long long tickee)
{
   unsigned long long tockee __attribute__ ((aligned (8))) = tickee;
   asm volatile ("SCKC	%0" : : "m" (tockee) );
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

/* -------------------------------------------------------- */
/* load control registers */
#define _lctl(REGNO)						\
extern inline void _lctl##REGNO (unsigned int value)		\
{								\
   /* be sure to serialize memory access around this instruction */\
   asm volatile ("LCTL  " #REGNO "," #REGNO ",%0"  		\
                   : : "m" (value) : "memory");			\
}

_lctl(0)
_lctl(1)
_lctl(2)
_lctl(3)
_lctl(4)
_lctl(5)
_lctl(6)
_lctl(7)
_lctl(8)
_lctl(9)
_lctl(10)
_lctl(11)
_lctl(12)
_lctl(13)
_lctl(14)
_lctl(15)

