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

O_OBJS :=  device.o con3270.o scrn3270.o video3270.o raw3210.o \
	eckd.o ckd.o graf.o fba.o tape.o tss.o osa.o ctca.o


include $(TOPDIR)/Rules.make
