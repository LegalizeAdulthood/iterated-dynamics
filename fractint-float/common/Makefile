SHELL=/bin/sh

OBJS = \
3d.o ant.o bigflt.o  biginit.o bignum.o bignumc.o calcfrac.o \
cmdfiles.o decoder.o editpal.o encoder.o \
evolve.o f16.o fracsubr.o fractalb.o fractalp.o fractals.o \
fractint.o framain2.o frasetup.o gifview.o hcmplx.o help.o \
intro.o jb.o jiim.o line3d.o loadfdos.o loadfile.o loadmap.o lorenz.o \
lsys.o lsysf.o memory.o miscfrac.o miscovl.o miscres.o mpmath_c.o parser.o \
parserfp.o plot3d.o printer.o prompts1.o prompts2.o \
realdos.o rotate.o slideshw.o soi.o soi1.o stereo.o targa.o testpt.o tgaview.o \
tplus.o zoom.o

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

fractint.o: fractint.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c fractint.c

help.o: help.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c help.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.

3d.o: 3d.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

ant.o: ant.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h ${HFD}/helpdefs.h

bigflt.o: bigflt.c ${HFD}/big.h ${HFD}/biginit.h

biginit.o: biginit.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h \
${HFD}/big.h 

bignum.o: bignum.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h 

bignumc.o: bignumc.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h 

calcfrac.o: calcfrac.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/mpmath.h ${HFD}/targa_lc.h \
			${HFD}/prototyp.h ${HFD}/helpcom.h

cmdfiles.o: cmdfiles.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/prototyp.h ${HFD}/mpmath.h \
			${HFD}/helpcom.h

decoder.o: decoder.c ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/port.h ${HFD}/fractint.h ${HFD}/helpcom.h

editpal.o: editpal.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

encoder.o: encoder.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

f16.o: f16.c ${HFD}/targa_lc.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/port.h ${HFD}/fractint.h ${HFD}/helpcom.h

fracsubr.o: fracsubr.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/mpmath.h ${HFD}/prototyp.h \
			${HFD}/helpcom.h

fractalb.o: fractalb.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h \
${HFD}/helpdefs.h ${HFD}/fractype.h

fractalp.o: fractalp.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/mpmath.h ${HFD}/helpdefs.h ${HFD}/fractype.h \
			${HFD}/prototyp.h ${HFD}/helpcom.h

fractals.o: fractals.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/mpmath.h ${HFD}/helpdefs.h ${HFD}/fractype.h \
			${HFD}/prototyp.h ${HFD}/helpcom.h

fractint.o: fractint.c ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/port.h ${HFD}/fractint.h ${HFD}/helpcom.h \
			${HFD}/fractype.h ${HFD}/helpdefs.h

framain2.o: framain2.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h \
${HFD}/fractype.h ${HFD}/helpdefs.h

frasetup.o: frasetup.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h \
${HFD}/mpmath.h ${HFD}/helpdefs.h ${HFD}/fractype.h

gifview.o: gifview.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

hcmplx.o: hcmplx.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h ${HFD}/mpmath.h

help.o: help.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/helpcom.h ${HFD}/helpdefs.h ${HFD}/prototyp.h ${HFD}/mpmath.h

intro.o: intro.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/helpdefs.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

jb.o: jb.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/mpmath.h ${HFD}/helpdefs.h ${HFD}/prototyp.h ${HFD}/helpcom.h

jiim.o: jiim.c ${HFD}/helpdefs.h ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/prototyp.h ${HFD}/mpmath.h \
		${HFD}/helpcom.h

line3d.o: line3d.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

loadfdos.o: loadfdos.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/helpdefs.h ${HFD}/prototyp.h ${HFD}/mpmath.h \
			${HFD}/helpcom.h

loadfile.o: loadfile.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/targa_lc.h ${HFD}/prototyp.h \
			${HFD}/mpmath.h ${HFD}/helpcom.h

loadmap.o: loadmap.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

lorenz.o: lorenz.c ${HFD}/mpmath.h ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/prototyp.h ${HFD}/helpcom.h

lsys.o: lsys.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

lsysf.o: lsysf.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h ${HFD}/lsys.h

miscfrac.o: miscfrac.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h \
${HFD}/fractype.h ${HFD}/targa_lc.h

miscovl.o: miscovl.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/helpdefs.h ${HFD}/prototyp.h \
			${HFD}/mpmath.h ${HFD}/helpcom.h

miscres.o: miscres.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/helpdefs.h ${HFD}/prototyp.h \
			${HFD}/mpmath.h ${HFD}/helpcom.h

mpmath_c.o: mpmath_c.c ${HFD}/mpmath.h ${HFD}/prototyp.h ${HFD}/port.h ${HFD}/fractint.h ${HFD}/helpcom.h

parser.o: parser.c ${HFD}/mpmath.h ${HFD}/prototyp.h ${HFD}/port.h ${HFD}/fractint.h ${HFD}/helpcom.h

parserfp.o: parserfp.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

plot3d.o: plot3d.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

printer.o: printer.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

prompts1.o: prompts1.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/helpdefs.h ${HFD}/prototyp.h \
			${HFD}/mpmath.h ${HFD}/helpcom.h

prompts2.o: prompts2.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/helpdefs.h ${HFD}/prototyp.h \
			${HFD}/mpmath.h ${HFD}/helpcom.h

realdos.o: realdos.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/fractype.h ${HFD}/helpdefs.h ${HFD}/prototyp.h \
			${HFD}/mpmath.h ${HFD}/helpcom.h

rotate.o: rotate.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/helpdefs.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

slideshw.o: slideshw.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

stereo.o: stereo.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h ${HFD}/helpdefs.h

targa.o: targa.c ${HFD}/targa.h ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

testpt.o: testpt.c

tgaview.o: tgaview.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/targa_lc.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h

tp3d.o: tp3d.c ${HFD}/mpmath.h ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/helpcom.h

tplus.o: tplus.c ${HFD}/port.h ${HFD}/tplus.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/fractint.h ${HFD}/helpcom.h

zoom.o: zoom.c ${HFD}/fractint.h ${HFD}/port.h ${HFD}/prototyp.h ${HFD}/mpmath.h ${HFD}/helpcom.h
