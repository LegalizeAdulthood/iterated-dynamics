
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

OBJ = 3d.obj ant.obj bigflt.obj biginit.obj bignum.obj calcfrac.obj \
cmdfiles.obj decoder.obj diskvid.obj editpal.obj encoder.obj evolve.obj \
f16.obj fracsubr.obj fractals.obj fractalp.obj fractalb.obj fractint.obj \
framain2.obj frasetup.obj gifview.obj hcmplx.obj help.obj \
history.obj intro.obj \
jb.obj jiim.obj line3d.obj loadfile.obj loadfdos.obj loadmap.obj \
lorenz.obj lsys.obj lsysf.obj memory.obj miscfrac.obj miscovl.obj \
miscres.obj mpmath_c.obj parser.obj parserfp.obj plot3d.obj printer.obj \
prompts1.obj prompts2.obj realdos.obj rotate.obj slideshw.obj soi.obj \
soi1.obj stereo.obj targa.obj testpt.obj tgaview.obj \
yourvid.obj zoom.obj

HFD = ..\headers

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : $(OBJ)

.c.obj:
	$(CC) /I$(HFD) $(OptT) $*.c >> f_errs.txt

Optsize = $(CC) /I$(HFD) $(OptS) $*.c >> f_errs.txt

Optnoalias = $(CC) /I$(HFD) $(OptN) $*.c >> f_errs.txt


3d.obj : 3d.c $(HFD)\fractint.h

ant.obj : ant.c $(HFD)\helpdefs.h

bigflt.obj : bigflt.c $(HFD)\big.h
	$(Optnoalias)

biginit.obj : biginit.c $(HFD)\big.h
	$(Optnoalias)

bignum.obj : bignum.c $(HFD)\big.h
	$(Optnoalias)

# only used for non ASM version
#bignumc.obj : bignumc.c $(HFD)\big.h
#        $(Optnoalias)

calcfrac.obj : calcfrac.c $(HFD)\fractint.h $(HFD)\mpmath.h

cmdfiles.obj : cmdfiles.c $(HFD)\fractint.h
	$(Optsize)

decoder.obj : decoder.c $(HFD)\fractint.h

diskvid.obj : diskvid.c $(HFD)\fractint.h

editpal.obj : editpal.c $(HFD)\fractint.h
	$(Optsize)

encoder.obj : encoder.c $(HFD)\fractint.h $(HFD)\fractype.h

evolve.obj : evolve.c $(HFD)\fractint.h
        $(Optnoalias)

f16.obj : f16.c $(HFD)\targa_lc.h

fracsubr.obj : fracsubr.c $(HFD)\fractint.h $(HFD)\helpdefs.h
	$(Optnoalias)

fractals.obj : fractals.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\mpmath.h $(HFD)\helpdefs.h

fractalp.obj : fractalp.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\mpmath.h $(HFD)\helpdefs.h

fractalb.obj : fractalb.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\big.h $(HFD)\helpdefs.h

fractint.obj : fractint.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\helpdefs.h
	$(Optsize)

framain2.obj : framain2.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\helpdefs.h
	$(Optsize)

frasetup.obj : frasetup.c

gifview.obj : gifview.c $(HFD)\fractint.h

hcmplx.obj : hcmplx.c $(HFD)\fractint.h

help.obj : help.c $(HFD)\fractint.h $(HFD)\helpdefs.h $(HFD)\helpcom.h
	$(Optsize)

history.obj : history.c $(HFD)\fractint.h $(HFD)\fractype.h
	$(Optsize)

intro.obj : intro.c $(HFD)\fractint.h $(HFD)\helpdefs.h
	$(Optsize)

jb.obj : jb.c $(HFD)\fractint.h $(HFD)\helpdefs.h

jiim.obj : jiim.c $(HFD)\helpdefs.h

line3d.obj : line3d.c $(HFD)\fractint.h

loadfile.obj : loadfile.c $(HFD)\fractint.h $(HFD)\fractype.h
	$(Optsize)

loadfdos.obj : loadfdos.c $(HFD)\fractint.h $(HFD)\helpdefs.h
	$(Optsize)

loadmap.obj : loadmap.c $(HFD)\targa.h $(HFD)\fractint.h
	$(Optsize)

lorenz.obj : lorenz.c $(HFD)\fractint.h $(HFD)\fractype.h

lsys.obj : lsys.c $(HFD)\fractint.h $(HFD)\lsys.h

lsysf.obj : lsysf.c $(HFD)\fractint.h $(HFD)\lsys.h

memory.obj : memory.c

miscfrac.obj : miscfrac.c $(HFD)\fractint.h $(HFD)\mpmath.h

miscovl.obj : miscovl.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\helpdefs.h
	$(Optsize)

miscres.obj : miscres.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\helpdefs.h
	$(Optsize)

mpmath_c.obj : mpmath_c.c $(HFD)\mpmath.h

parser.obj : parser.c $(HFD)\fractint.h $(HFD)\mpmath.h
	$(Optnoalias)

parserfp.obj : parserfp.c $(HFD)\fractint.h $(HFD)\mpmath.h
	$(Optnoalias)

plot3d.obj : plot3d.c $(HFD)\fractint.h $(HFD)\fractype.h
	$(Optnoalias)

printer.obj : printer.c $(HFD)\fractint.h
	$(Optsize)

prompts1.obj : prompts1.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\helpdefs.h
	$(Optsize)

prompts2.obj : prompts2.c $(HFD)\fractint.h $(HFD)\fractype.h $(HFD)\helpdefs.h
	$(Optsize)

realdos.obj : realdos.c $(HFD)\fractint.h $(HFD)\helpdefs.h
	$(Optsize)

rotate.obj : rotate.c $(HFD)\\fractint.h $(HFD)\\helpdefs.h
	$(Optsize)

slideshw.obj : slideshw.c
	$(Optsize)

soi.obj : soi.c

soi1.obj : soi1.c

stereo.obj : stereo.c $(HFD)\helpdefs.h

targa.obj : targa.c $(HFD)\targa.h $(HFD)\fractint.h

testpt.obj: testpt.c $(HFD)\fractint.h

tgaview.obj : tgaview.c $(HFD)\fractint.h $(HFD)\targa_lc.h $(HFD)\port.h

yourvid.obj : yourvid.c

zoom.obj : zoom.c $(HFD)\fractint.h
	$(Optnoalias)
#	$(Optsize)

