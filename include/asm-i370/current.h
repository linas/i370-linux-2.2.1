 /*
        current.h -- Access active thread pointer

  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.

  Date: $Id: current.h,v 1.5 1999/10/07 04:08:05 linas Exp $
  Description:

  Changes:
  
 */ 
 
#ifndef _I370_CURRENT_H
#define _I370_CURRENT_H

 /*	  -- N O T E S --
 ---------------------------------------------------------------------------
  .     The address of the currently executing thread (struct) is in 
  .     psa.Current. It has been kept in the custumary external pointer
  .     `current' for a long time, which was extended to a vector
  .     `current_set' to provide for multiprocessors. But this a bad 
  .     design, and by now most ports have gotten rid of it. If ever
  .     possible, we try to keep per processor variables in the PSA, as
  .     it is made for it, and to reduce memory contention and cache usage. 
  .     The present style of interfacing is, however, reserved for this case.
 */
 
#define AOFF_psa_current 0x224	/* 580 */

static 
struct task_struct * get_current(void)
     __attribute__((const));

static inline 
struct task_struct * get_current(void) 
{
  struct task_struct *curr;
  __asm__ ("L   %0,AOFF_psa_current(0);" : "=r"(curr));
  return curr;
};

static inline 
void   set_current(const struct task_struct *curr) 
{
  __asm__ ("ST   %0,AOFF_psa_current(0);" :: "r"(curr));
};

#define current (get_current())



#endif /* !(_I370_CURRENT_H) */


