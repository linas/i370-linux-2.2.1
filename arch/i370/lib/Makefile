#
# Makefile for i370-specific library files..
#

.S.o:
	$(CC) -D__ASSEMBLY__ -c $< -o $*.o

O_TARGET = lib.o
# O_OBJS  = checksum.o string.o strcase.o
O_OBJS  = string.o misc.o

ifdef CONFIG_SMP
O_OBJS += locks.o
endif


include $(TOPDIR)/Rules.make
