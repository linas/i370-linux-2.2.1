
/*
 * wrappers for misc assembly
 */

/* set storage key extended */
extern inline void _sske (unsigned int key, unsigned int realaddr)
{
   /* be sure to serialize memory access around this nstruction */
   asm volatile ("SSKE %1,%0" : "r" (realaddr), "r" (key) : : "memory");
}

/* examine storage key extended */
extern inline unsigned int _iske (unsigned int realaddr)
{
   unsigned int key;
   asm volatile ("ISKE %1,%0" : "r" (realaddr) : "r" (key) );
   return key;
}

