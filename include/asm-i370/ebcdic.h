 /*       ebcdic .h -- Character set utilities 
  
  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

 */ 

 
#ifndef _I370_asm_EBCDIC_H
#define _I370_asm_EBCDIC_H

extern inline void
__i370_translate (char * area, unsigned short size, const unsigned char table[256])   
{
	/* XXX should check that size is 4095 or smaller */
	__asm__ (" TR    %O0(%2,%R0),%1"
		: "=m"(area)
		: "m"(table), "I"(size));
}

#define __i370_translate_to_ascii(area,size)\
        __i370_translate(area,size,ebcdic_to_ascii)
#define __i370_translate_to_ebcdic(area,size)\
        __i370_translate(area,size,ascii_to_ebcdic)

extern unsigned char ebcdic_to_ascii [256];
extern unsigned char ascii_to_ebcdic [256];


#endif  /* _I370_asm_EBCDIC_H */

