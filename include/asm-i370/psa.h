 /*
        psa.h -- Prefixed Storage Area Definition
  
  This file is part of Linux/390.

  Written by Peter Schulte-Stracke <Peter.Schulte-Stracke@t-online.de>
  
  Copyright 1999 by Peter Schulte-Stracke. This is free software. Use
  is permitted under the obligations of the GNU General Public Licence.
  See file COPYRIGHT for details. There is NO warranty.
  
  Date: $Id: psa.h,v 1.2 1999/10/07 05:15:14 linas Exp $
  Data Types:
  Special Variables:
  Known Bugs:
  Changes:
  
 */ 

#ifndef __ASM_I370_PSA_H
#define __ASM_I370_PSA_H
#ifndef __ASSEMBLY__

 /*	  -- N O T E S --
 ----------------------------------------------------------------------
  .     The PSA is a per processor area used primarily by low level
  .	routines. It is neccessary for task switching including
  .	interrupt handling, and some low level kernel assembler code.
  .	
  .     Cf. asm/current.h for more considerations.
  */

#define _i370_vm_guest_p()       (_PSA_.reserved_0[20] & 0x80)
#define _i370_hercules_guest_p() (_PSA_.reserved_0[20] & 0x40)
#define _i370_primary_cpu_p()    (_PSA_.reserved_0[20] & 0x01)

struct task_struct;

struct PSA {		/* PSA: Prefixed Storage Area */
   
   /* much presently omitted...... */
  char           reserved_0 [512];

  char           eyecatcher [4];/* "PSA " in ascii                    */
  struct         _initflag
  { unsigned     vm       :1;	/* a VM guest                         */
    unsigned     hercules :1;	/* a hercules guest                   */
    unsigned              :5;
    unsigned     primary  :1;	/* the primary (booting) CPU          */
  } initflag;
  unsigned short cpuadp, cpuadl;/* processor address (from STIDP)     */
  unsigned short cpuno;		/* cpu number as defined by Linux     */

  /*  It is unclear to me whether the logical cpu number defined by Linux
   *  will survive; as the cpu address range is sufficiently small. Only
   *  the equation cpuad = 0 <=> primary cpu would be affected, as far
   *  as I can see at this moment.
   */
  unsigned short cpumask;	/* bit set according to  ...          */
  char *         istack;	/* address of first frame in interrupt stack */

  char reserved_528 [576-528];

  char           super[0];	/* four bytes of flags redefined below*/
  
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

  struct task_struct* Current;	/* thread executing on this CPU       */

  unsigned int   local_bh_count;
  unsigned int   local_irq_count;

  char reserved_592 [1728-592];

  unsigned long  regs1 [16];	/* a register save area               */

  char reserved_1792 [4096-1792]; /* was tca ... */

  };

typedef struct PSA psa_t;
extern struct PSA _PSA_ ;	/* defined in head.S */

#else  // __ASSEMBLY__

#define _psa_current 0x224	/* 580 */


#endif // __ASSEMBLY__
#endif // __ASM_I370_PSA_H
