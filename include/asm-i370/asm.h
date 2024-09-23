/*
 * wrappers for misc assembly
 * Linas Vepstas February 1999
 * Copyright (C) Linas Vepstas 1999
 */

#ifndef __I370_ASM_H__
#define __I370_ASM_H__

#ifndef __ASSEMBLY__

/* -------------------------------------------------------- */
/* Get and set the current value of the stack pointer SP and the stack
 * top pointer STP.  Note that SP points at the base, and that saved
 * regs, args, frame, * etc. are a positive offset from SP.  The STP
 * becomes the SP during a subrutine call.
 */
/* Get the current value of the stack pointer */
/* Since this inlines, it will basically copy r13 to where-ever */
extern inline unsigned long _get_SP(void)
{
        unsigned long rc;
        asm volatile ("LR	%0,r13" : "=r" (rc) : );
        return rc;
}

extern inline unsigned long _get_STP(void)
{
        unsigned long rc;
        asm volatile ("LR	%0,r11" : "=r" (rc) : );
        return rc;
}

/* Set the current value of the stack pointer */
/* Use with caution */
extern inline void _set_SP (unsigned long newsp)
{
        asm volatile ("LR	r13,%0" : : "r" (newsp) : "memory");
}

extern inline void _set_STP (unsigned long newsp)
{
        asm volatile ("LR	r11,%0" : : "r" (newsp) : "memory");
}

/* -------------------------------------------------------- */
/* copy fpregs to memory */
extern inline void _store_fpregs (double *memy)
{
        asm volatile (
		"STD	f0,%0;		\n"
		"STD	f2,%1;		\n"
		"STD	f4,%2;		\n"
		"STD	f6,%3;		\n"
		: "=m" (memy[0]),
		  "=m" (memy[1]),
		  "=m" (memy[2]),
		  "=m" (memy[3]) );
}

/* copy mem to fpregs */
extern inline void _load_fpregs (double *memy)
{
        asm volatile (
		"LD	f0,%0;		\n"
		"LD	f2,%1;		\n"
		"LD	f4,%2;		\n"
		"LD	f6,%3;		\n"
		: : "m" (memy[0]),
		    "m" (memy[1]),
		    "m" (memy[2]),
		    "m" (memy[3]) );
}

/* -------------------------------------------------------- */
/* the 64-bit clock words need to be 8-byte aligned.  But
 * it seems that the __attribute__ ((aligned (8)) doesn't
 * work on the stack.  So force alignment the hard way.
 */
/* Store Clock */
extern inline unsigned long long _stck (void)
{
   // unsigned long long tickee __attribute__ ((aligned (8)));
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *tockee;
   ptr = ((ptr+4) >> 3) << 3;
   tockee = (unsigned long long *) ptr;
   asm volatile ("STCK	%0" : "=m" (*tockee) );
   return *tockee;
}

/* Store Clock Comparator */
extern inline unsigned long long _stckc (void)
{
   // unsigned long long tickee __attribute__ ((aligned (8)));
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *tockee;
   ptr = ((ptr+4) / 8) * 8;
   tockee = (unsigned long long *) ptr;
   asm volatile ("STCKC	%0" : "=m" (*tockee) );
   return *tockee;
}

/* Set Clock Comparator */
extern inline void _sckc (unsigned long long tickee)
{
   char area[12];
   unsigned long ptr = (unsigned long ) area;
   unsigned long long *tockee;
   ptr = ((ptr+4) >> 3) << 3;
   tockee = (unsigned long long *) ptr;
   *tockee = tickee;
   asm volatile ("SCKC	%0" : : "m" (*tockee) );
}

/* -------------------------------------------------------- */
/* Set Storage Key Extended */
extern inline void _sske (unsigned int key, unsigned int realaddr)
{
   /* be sure to serialize memory access around this instruction */
   asm volatile ("SSKE	%0,%1" : : "r" (key), "r" (realaddr) : "memory");
}

/* Examine Storage Key Extended */
extern inline unsigned int _iske (unsigned int realaddr)
{
   unsigned int key;
   asm volatile ("ISKE	%0,%1" : "=r" (key) : "r" (realaddr) );
   return key;
}

/* Set PSW Key from Address */
extern inline void _spka (unsigned int key)
{
   asm volatile ("SPKA	0(%0)" : : "r" (key));
}

/* -------------------------------------------------------- */

extern inline void _lpsw (unsigned long long psw)
{
   asm volatile ("LPSW	%0" : : "m" (psw) : "memory");
}

/* -------------------------------------------------------- */
/* Load Control Registers */

#define _lctl(REGNO,val) _lctl_r##REGNO (val)

#define def_lctl(REGNO)						\
extern inline void _lctl_r##REGNO (unsigned int value)		\
{								\
   /* be sure to serialize memory access around this instruction */\
   asm volatile ("LCTL  " #REGNO "," #REGNO ",%0"  		\
                   : : "m" (value) : "memory");			\
}

def_lctl(0)
def_lctl(1)
def_lctl(2)
def_lctl(3)
def_lctl(4)
def_lctl(5)
def_lctl(6)
def_lctl(7)
def_lctl(8)
def_lctl(9)
def_lctl(10)
def_lctl(11)
def_lctl(12)
def_lctl(13)
def_lctl(14)
def_lctl(15)

/* -------------------------------------------------------- */
/* Store Control Registers */

#define _stctl(REGNO,val) _stctl_r##REGNO (val)

#define def_stctl(REGNO)						\
extern inline unsigned long _stctl_r##REGNO (void)		\
{								\
   /* be sure to serialize memory access around this instruction */\
   unsigned long area;				\
   asm volatile ("STCTL  " #REGNO "," #REGNO ",%0"  		\
                   : "=m" (area) );			\
   return(area);					\
}

def_stctl(0)
def_stctl(1)
def_stctl(2)
def_stctl(3)
def_stctl(4)
def_stctl(5)
def_stctl(6)
def_stctl(7)
def_stctl(8)
def_stctl(9)
def_stctl(10)
def_stctl(11)
def_stctl(12)
def_stctl(13)
def_stctl(14)
def_stctl(15)

/* -------------------------------------------------------- */
/* Store Processor Address */
extern inline unsigned long _stap (void)
{
	unsigned short cpuid;
	asm volatile ("STAP	%0" : "=m" (cpuid) );
	return ((unsigned long) cpuid);
}

/* -------------------------------------------------------- */
/* Signal Processor */
extern inline long _sigp (unsigned long addr,
                         unsigned long code)
{
	long rc;

	asm volatile (
		"SIGP   %1,%2(0);       \n"
		"IPM    r1;     \n"
		"SRL    r1,28;  \n"
		"LR     %0,r1;  \n"
		: "=r" (rc)
		: "r" (addr), "m" (code)
		: "r1", "memory");
	return (rc);
}

/* -------------------------------------------------------- */
/* Purge Table Lookaside Buffer */
extern inline void _ptlb (void)
{
	asm volatile ("PTLB" : : : "memory");
}

/* Invalidate Page Table Entry */
extern inline void _ipte (void *sto, void *pfx)
{
	asm volatile (
		"IPTE %0,%1\n"
		:
		: "r" (sto), "r" (pfx)
		: "memory");
}

/* -------------------------------------------------------- */
/* Halt the system */

extern inline void i370_halt (void)
{
	// need to have 8-byte alignment for the psw
	// unsigned long long psw __attribute__ ((aligned (8)));
	char area[12];
	unsigned long ptr = (unsigned long ) area;
	unsigned long long *psw;
	ptr = ((ptr+4) / 8) * 8;
	psw = (unsigned long long *) ptr;
	asm volatile (
	"	L	r1,=X'000a0000';	\n"
	"	ST	r1,%0;			\n"
	"	L	r1,=A(1f);		\n"
	"	O	r1,=X'80000000';	\n"
	"	ST	r1,4+%0;		\n"
	"	LPSW	%0;			\n"
	"1:	NOPR	r0;			\n"
	: "+m" (*psw) : : "r1", "memory");
}

/* -------------------------------------------------------- */
/* Enabled wait will yeild the CPU but keep the interrupts hot */

extern inline void i370_enabled_wait (void)
{
	// need to have 8-byte alignment for the psw
	// unsigned long long psw __attribute__ ((aligned (8)));
	char area[12];
	unsigned long ptr = (unsigned long ) area;
	unsigned long long *psw;
	ptr = ((ptr+4) / 8) * 8;
	psw = (unsigned long long *) ptr;

	// The flags should be set for:
	// DAT off,  real mode, supervisor state
	// external, i/o and machine check enabled
	// PER ??
	// key 6 storage (as defined in processor.h
	asm volatile (
	"	L	r1,=X'036e0000';	\n"
	"	ST	r1,%0;			\n"
	"	L	r1,=A(1f);		\n"
	"	O	r1,=X'80000000';	\n"
	"	ST	r1,4+%0;		\n"
	"	LPSW	%0;			\n"
	"1:	NOPR	r0;			\n"
	: "+m" (*psw) : : "r1", "memory");
}

/* -------------------------------------------------------- */
/* Load Real Address...                                     */
/* -------------------------------------------------------- */

extern inline void * _lra(unsigned long *va)
{
 void *ra;

	asm volatile (
		"LRA     %0,%1;		\n"
		: "=r" (ra)
		: "m" (*va) );
	return(ra);
}


/* -------------------------------------------------------- */

#endif /* __ASSEMBLY__ */
#endif /* __I370_ASM_H__ */
