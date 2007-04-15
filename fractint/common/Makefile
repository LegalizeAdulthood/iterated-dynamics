SHELL=/bin/sh

OBJS = \
3d.o ant.o bigflt.o  biginit.o bignum.o bignumc.o calcfrac.o \
cmdfiles.o decoder.o drivers.o editpal.o encoder.o \
evolve.o f16.o fracsubr.o fractalb.o fractalp.o fractals.o \
fractint.o framain2.o frasetup.o gifview.o hcmplx.o help.o history.o \
intro.o jb.o jiim.o line3d.o loadfdos.o loadfile.o loadmap.o lorenz.o \
lsys.o lsysf.o memory.o miscfrac.o miscovl.o miscres.o mpmath_c.o parser.o \
parserfp.o plot3d.o prompts1.o prompts2.o \
realdos.o rotate.o slideshw.o soi.o soi1.o stereo.o testpt.o tgaview.o \
zoom.o

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

depend:
	gccmakedep -- $(CFLAGS) -- $(OBJS:.o=.c)
#	makedepend $(CFLAGS) $(OBJS:.o=.c)

fractint.o: fractint.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c fractint.c

help.o: help.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c help.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE
