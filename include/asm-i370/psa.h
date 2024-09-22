 /*
  psa.h -- Prefixed Storage Area Definition

  The PSA is used to implement SMP on Linux/390.  Each processor has
  its own 4K PSA page that stores any/all processor-specific data.
  Although each PSA appears at a different 'absolute' address, it
  always maps to real address zero for any given processor.

  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  Updates Copyright (C) 1999 Linas Vepstas
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.
 */ 

#ifndef __ASM_I370_PSA_H
#define __ASM_I370_PSA_H

#include <asm/ptrace.h>

#ifndef __ASSEMBLY__

#define _i370_vm_guest_p()       (_PSA_.initflag.vm)
#define _i370_hercules_guest_p() (_PSA_.initflag.hercules)
#define _i370_primary_cpu_p()    (_PSA_.initflag.primary)

struct task_struct;

struct PSA {		/* PSA: Prefixed Storage Area */
   
  /* Interrupt vectors are at low addresses.  See processor.h
   * much presently omitted...... */

#ifdef CONFIG_HERCULES_GUEST
  // Hercules uses the first two words for a short-form PSW
  // And we hack this so it is in the ELF file. Fix this once
  // we get an ELF boatloader for Hercules.
  char           reserved_0 [504];
#else
  char           reserved_0 [512];
#endif

  char           eyecatcher [4];/* "PSA " in ascii                    */

  /* XXX Danger Danger! The compiler currently pads out the non-word
   * sized structures to a full-word.  Note, however, that this
   * is subject to change in future compilers!  Thus, we hand-pad
   * the struct below to fullword! Doctor Doctor, it hurts when I do this!
   */
  struct         _initflag
  { unsigned     vm       :1;	/* a VM guest                         */
    unsigned     hercules :1;	/* a hercules guest                   */
    unsigned              :5;
    unsigned     primary  :1;	/* the primary (booting) CPU          */
    char         pad[3];        /* pad to full-word                   */
  } initflag;

  struct task_struct* Current;	/* thread executing on this CPU       */

  unsigned short cpuadp, cpuadl;/* processor address (from STIDP)     */
  unsigned short cpuno;		/* cpu number as defined by Linux     */

  /*  It is unclear to me whether the logical cpu number defined by Linux
   *  will survive; as the cpu address range is sufficiently small. Only
   *  the equation cpuad = 0 <=> primary cpu would be affected, as far
   *  as I can see at this moment.
   */
  unsigned short cpumask;	/* bit set according to  ...          */
  char *         istack;	/* address of first frame in interrupt stack */


  unsigned int   local_bh_count;
  unsigned int   local_irq_count;

#ifdef NOT_RIGHT_NOW
  /*             PSASUPER       FIRST BYTE                            */ 
  
  unsigned mchk   : 1;	        /* Machine Check FLIH in progress     */
  unsigned rst    : 1;	        /* Restart in progress                */
  unsigned svc    : 1;	        /* SVC FLIH in progress               */
  unsigned pgm    : 1;	        /* Program interruption in progress   */
  unsigned io     : 1;          /* IO interruption handler in progress*/
  unsigned ext    : 1;	        /* External FLIH in progress          */
  unsigned disp   : 1;		/* Dispatcher called                  */
  unsigned ist    : 1;		/* On the interrupt stack, after FLIH */

  /*             PSASUPER       SECOND BYTE                           */ 

  unsigned acr    : 1;	        
  unsigned ts     : 1;		/* Task switch requested              */
  unsigned        : 0;
  /*             PSASUPER       THIRD BYTE                            */ 
  unsigned        : 0;
  /*             PSASUPER       FOURTH BYTE                           */ 
  unsigned        : 0;
#endif

  /* pfx trace is at 0xd00 see PFX_TRACE in head.S */
  /* INTERRUPT_BASE is 0xf00 in processor.h  */
  char reserved_544 [0xf00-0x220];
  psw_t		 psw;
  irregs_t       irregs;       /* some but not all of the GPR's       */

  char reserved_3912 [0x1000-0xf48];  

  };

typedef struct PSA psa_t;
extern struct PSA _PSA_ ;	/* Defined in setup.c */

#else  // __ASSEMBLY__

#define _psa_current 0x208	/* decimal 520 */


#endif // __ASSEMBLY__
#endif // __ASM_I370_PSA_H
