#
# Makefile for i370-specific library files..
#
.S.o:
	$(CC) -D__ASSEMBLY__ -c $< -o $*.o

O_TARGET = lib.o
# O_OBJS  = strcase.o
O_OBJS  = tables.o checksum.o string.o misc.o

ifdef CONFIG_SMP
O_OBJS += locks.o
endif


include $(TOPDIR)/Rules.make
