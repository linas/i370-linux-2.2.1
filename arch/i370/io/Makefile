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

O_TARGET := io.o
# OX_OBJS := i370_ksyms.o setup.o


O_OBJS :=  device.o con3270.o scrn3270.o video3270.o eckd.o ckd.o \
               graf.o console.o fba.o tape.o tss.o osa.o ctca.o


# all: 



include $(TOPDIR)/Rules.make
