# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Linus Torvalds
# Changes for PPC by Gary Thomas
# Rewritten by Cort Dougan and Paul Mackerras
#

KERNELLOAD = 0x10000

ASFLAGS         =
LINKFLAGS       =  -T arch/i370/vmlinux.lds -Ttext $(KERNELLOAD) -Bstatic
# LINKFLAGS       =  -Ttext $(KERNELLOAD) -Bstatic
CFLAGSINC       = -D__KERNEL__ -I$(TOPDIR)/include -D__i370__
CFLAGS          := $(CFLAGS) -D__i370__ -fsigned-char -pipe \
                -fno-builtin -Wno-uninitialized 
CPP             = $(CC) -E $(CFLAGS)

HEAD := arch/i370/kernel/head.o

ARCH_SUBDIRS = arch/i370/kernel arch/i370/lib
SUBDIRS := $(SUBDIRS) $(ARCH_SUBDIRS)
ARCHIVES := arch/i370/kernel/kernel.o arch/i370/lib/lib.o $(ARCHIVES)
CORE_FILES := arch/i370/kernel/kernel.o arch/i370/lib/lib.o $(CORE_FILES)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot 

BOOT_TARGETS = netboot znetboot zImage \
         znetboot.initrd zImage.initrd 

$(BOOT_TARGETS): vmlinux

.SUFFIXES:	.lst

.s.lst:
	$(AS) -als=$*.lst $< 


tags:
	etags */*.c include/{asm,linux}/*.h arch/i370/kernel/*.{c,h}

archclean:
	rm -f arch/i370/kernel/{mk_defs,ppc_defs.h,find_name,checks}
	@$(MAKEBOOT) clean

archmrproper:

archdep:
	$(MAKEBOOT) fastdep

