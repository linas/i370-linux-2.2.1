
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
/* Store Clock */
extern inline unsigned long long _stck (void)
{
   unsigned long long tickee;
   asm volatile ("STCK	%0" : "=m" (tickee) );
   return tickee;
}

/* Store Clock Comparator */
extern inline unsigned long long _stckc (void)
{
   unsigned long long tickee;
   asm volatile ("STCKC	%0" : "=m" (tickee) );
   return tickee;
}

/* Set Clock Comparator */
extern inline void _sckc (unsigned long long tickee)
{
   asm volatile ("SCKC	%0" : : "m" (tickee) );
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

