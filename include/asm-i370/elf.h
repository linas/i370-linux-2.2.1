#ifndef __I370_ELF_H
#define __I370_ELF_H

/*
 * XXX the I370 arch should be in the include file
 * "binutils/include/elf/common.h"  which defines EM_I370 to be 0x0009
 */
#define EM_I370	0x0009

/*
 * ELF register definitions..
 */
#include <asm/ptrace.h>

#define ELF_NGREG	16	/* includes gprs only */
#define ELF_NFPREG	16

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) (((x) == EM_I370) || ((x) == 0xf00f))

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_ARCH	EM_I370
#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2MSB

#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE	4096

/* This is the location that an ET_DYN program is loaded if exec'ed.  Typical
   use of this is to invoke "./ld.so someprog" to test out a new version of
   the loader.  We need to make sure that it is out of the way of the program
   that it will "exec", and that there is sufficient room for the brk.  */

#define ELF_ET_DYN_BASE         (0x08000000)

typedef unsigned long elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef double elf_fpreg_t;
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];

#define ELF_CORE_COPY_REGS(gregs, regs) \
	memcpy(gregs, regs, \
	       sizeof(struct pt_regs) < sizeof(elf_gregset_t)? \
	       sizeof(struct pt_regs): sizeof(elf_gregset_t));


/* This yields a mask that user programs can use to figure out what
   instruction set this cpu supports.  This could be done in userspace,
   but it's not easy, and we've already done it here.  */

#define ELF_HWCAP	(0)

/* This yields a string that ld.so will use to load implementation
   specific libraries for optimization.  This is more specific in
   intent than poking at uname or /proc/cpuinfo.

   For the moment, we have only optimizations for the Intel generations,
   but that could change... */

#define ELF_PLATFORM	(NULL)

/* The i370 stack will grow upwards; thus the mm stack flags need 
    to indicate this */
#define VM_STACK_FLAGS 0x0277

#ifdef __KERNEL__
#define SET_PERSONALITY(ex, ibcs2) \
	current->personality = (ibcs2 ? PER_SVR4 : PER_LINUX)
#endif

#endif
