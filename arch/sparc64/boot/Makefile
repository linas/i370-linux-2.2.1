# $Id: Makefile,v 1.1.1.1 1999/02/08 06:21:37 linas Exp $
# Makefile for the Sparc64 boot stuff.
#
# Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
# Copyright (C) 1997 Jakub Jelinek (jj@sunsite.mff.cuni.cz)

ROOT_IMG	=/usr/src/root.img
ELFTOAOUT	=elftoaout

all: boot

boot:
	@echo "Nothing special to be done for 'boot' on Linux/UltraSPARC."

tftpboot.img: piggyback $(TOPDIR)/vmlinux $(ROOT_IMG)
	$(ELFTOAOUT) $(TOPDIR)/vmlinux -o tftpboot.img
	./piggyback tftpboot.img $(TOPDIR)/System.map $(ROOT_IMG)

piggyback: piggyback.c
	$(HOSTCC) $(HOSTCFLAGS) -o piggyback piggyback.c

dep:

