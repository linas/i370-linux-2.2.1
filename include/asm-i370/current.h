 /*
        current.h -- Access active thread pointer

  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

  Date: $Id: current.h,v 1.8 1999/10/07 04:30:03 linas Exp $
  Description:

  Changes:
  
 */ 
 
#ifndef _I370_CURRENT_H
#define _I370_CURRENT_H

#include <asm/psa.h>

 /*	  -- N O T E S --
 ---------------------------------------------------------------------------
  .     The address of the currently executing thread (struct) is in 
  .     psa.Current.  By putting this pointer in the PSA, all processors
  .     will have access to the different pointers located at the same 
  .     real address (but different absolute addresses).

  .     In general, we will keep per processor variables in the PSA, as
  .     it is made for it, and to reduce memory contention and cache usage. 
 */

#define current _PSA_.Current

#endif /* !(_I370_CURRENT_H) */


