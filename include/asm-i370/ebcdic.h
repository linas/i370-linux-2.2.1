 /*       ebcdic .h -- Character set utilities 
  
  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

  Date: $Id: ebcdic.h,v 1.1 1999/09/19 17:22:29 linas Exp $
  Changes:

 */ 

 
#ifndef _I370_asm_EBCDIC_H
#define _I370_asm_EBCDIC_H

#define __i370_translate(area,size,table)\
__asm__ (" TR    %O0(%2,%R0),%1;"\
	  : ="m"(area): "m"(table), "I"(size))

#define __i370_translate_to_ascii(area,size)\
        __i370_translate(area,size,tables_ebcdic_to_ascii)
#define __i370_translate_to_ebcdic(area,size)\
        __i370_translate(area,size,tables_ascii_to_ebcdic)

extern unsigned char tables_ebcdic_to_ascii [256];
extern unsigned char tables_ascii_to_ebcdic [256];

				/* very preliminary */

#endif  // _I370_asm_EBCDIC_H

