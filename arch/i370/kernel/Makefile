#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

.S.o:
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $*.o

O_TARGET := kernel.o
OX_OBJS := i370_ksyms.o setup.o

O_OBJS := traps.o syscalls.o idle.o time.o mm.o process.o bitops.o irq.o \
	fault.o signal.o

ifdef CONFIG_SMP
O_OBJS += smp.o
endif

all: head.o kernel.o

head.o: head.S $(TOPDIR)/include/linux/tasks.h 
# entry.o: entry.S 

include $(TOPDIR)/Rules.make
