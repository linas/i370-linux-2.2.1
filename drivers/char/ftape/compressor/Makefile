#
#       Copyright (C) 1997 Claus-Justus Heine.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Source: /home2/linas/archive/backlot/cvs-2014-11-21/cvsroot/linux/drivers/char/ftape/compressor/Makefile,v $
# $Revision: 1.1.1.1 $
# $Date: 1999/02/08 06:20:05 $
#
#      Makefile for the optional compressor for th zftape VFS
#      interface to the QIC-40/80/3010/3020 floppy-tape driver for
#      Linux.
#

#
#  This isn't used inside the kernel, only for my private development
#  version
#
ifndef TOPDIR
TOPDIR=../..
include $(TOPDIR)/MCONFIG
endif

O_TARGET := zft-compressor.o
O_OBJS    = zftape-compress.o lzrw3.o

M_OBJS    = $(O_TARGET)

CFLAGS_lzrw3.o	:= -O6 -funroll-all-loops

include $(TOPDIR)/Rules.make
