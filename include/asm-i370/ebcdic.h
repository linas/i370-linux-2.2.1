 /*       asm/ebcdic .h -- Character set utilities

  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>

  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

  Usage: use i370_translate for generic operands, of fixed and
  variable length, and with a table specified. For the default tables use
  i370_translate_to_ebcdic &c. NOTE that variable lengh operands with a
  length < 256 (! due to compiler/assembler bug for now) are much faster
  treated with __i370_translate[_to...], which modifies the translate
  instruction at run-time.

  Date: $Id: ebcdic.h,v 1.5 1999/11/11 04:26:29 linas Exp $
  Changes: Uses now test for constant lengh operand.

  */

#include <linux/types.h>

#ifndef _I370_asm_EBCDIC_H
#define _I370_asm_EBCDIC_H

#define i370_translate(a, s, t) \
if(__builtin_constant_p(s) && ((s) < 256))  { \
 _constant_i370_translate((a), (s), (t)); } \
 else {_i370_translate((a), (s), (t));}

#define  _constant_i370_translate(area, size, table) \
  __asm__ (" TR    %O0(%B2,%R0),%1;" \
	   : "=m"(*area): "m"(*table), "I"(size))


#define __i370_translate(area,size,table)\
  __asm__ ("BCTR   %2,0;\n\t"\
	   "EX %2,1f;\n\t"\
	   "B 2f;\n1:\t"\
	   "TR %O0(0,%R0),%1;\n2:\t;\n"\
	  : "=m"(*area) : "m"(*table), "a"(size))


static inline
void  _i370_translate (char * area, size_t size, const char * table)
{
       register int rest, amount;
       for (rest = size; rest > 0; rest -= 256)
           {
               amount = (rest>256) ? 256 : rest;
               __i370_translate(area, amount, table);
               area += 256;
           };
}

extern unsigned char ebcdic_to_ascii [256];
extern unsigned char ascii_to_ebcdic [256];

#define __i370_translate_to_ascii(area,size)\
        __i370_translate(area,size,ebcdic_to_ascii)
#define __i370_translate_to_ebcdic(area,size)\
        __i370_translate(area,size,ascii_to_ebcdic)

#define _i370_translate_to_ascii(area,size)\
        _i370_translate(area,size,ebcdic_to_ascii)
#define _i370_translate_to_ebcdic(area,size)\
        _i370_translate(area,size,ascii_to_ebcdic)

#define i370_translate_to_ascii(area,size)\
        i370_translate(area,size,ebcdic_to_ascii)
#define i370_translate_to_ebcdic(area,size)\
        i370_translate(area,size,ascii_to_ebcdic)

#endif  /* _I370_asm_EBCDIC_H */

