#
# Makefile for the SNI specific part of the kernel
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# $Id: Makefile,v 1.1.1.1 1999/02/08 06:21:24 linas Exp $
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

all: sni.o
O_TARGET := sni.o
O_OBJS	 := hw-access.o int-handler.o io.o pci.o pcimt_scache.o reset.o setup.o

int-handler.o: int-handler.S

clean:

include $(TOPDIR)/Rules.make
