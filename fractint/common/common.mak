
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all :

!ifndef DEBUG
.c.obj:
	  $(CC) /AM /W4 /FPi /c $(OptT) $*.c >> f_errs.txt

Optsize = $(CC) /AM /W4 /FPi /c $(OptS) $*.c >> f_errs.txt

Optnoalias = $(CC) /AM /W4 /FPi /c $(OptN) $*.c >> f_errs.txt
!else
.c.obj:
	  $(CC) /Zi /AM /W4 /FPi /c $(OptT) $*.c >> f_errs.txt

Optsize = $(CC) /Zi /AM /W4 /FPi /c $(OptS) $*.c >> f_errs.txt

Optnoalias = $(CC) /Zi /AM /W4 /FPi /c /DTESTFP $(OptN) $*.c >> f_errs.txt
!endif

3d.obj : 3d.c fractint.h

ant.obj : ant.c helpdefs.h

bigflt.obj : bigflt.c big.h
	$(Optnoalias)

biginit.obj : biginit.c big.h
	$(Optnoalias)

bignum.obj : bignum.c big.h
	$(Optnoalias)

# only used for non ASM version
#bignumc.obj : bignumc.c big.h
#        $(Optnoalias)

calcfrac.obj : calcfrac.c fractint.h mpmath.h

cmdfiles.obj : cmdfiles.c fractint.h
	$(Optsize)

decoder.obj : decoder.c fractint.h

diskvid.obj : diskvid.c fractint.h

editpal.obj : editpal.c fractint.h
	$(Optsize)

encoder.obj : encoder.c fractint.h fractype.h

evolve.obj : evolve.c fractint.h
        $(Optnoalias)

f16.obj : f16.c targa_lc.h

fracsubr.obj : fracsubr.c fractint.h helpdefs.h
	$(Optnoalias)

fractals.obj : fractals.c fractint.h fractype.h mpmath.h helpdefs.h

fractalp.obj : fractalp.c fractint.h fractype.h mpmath.h helpdefs.h

fractalb.obj : fractalb.c fractint.h fractype.h big.h helpdefs.h

fractint.obj : fractint.c fractint.h fractype.h helpdefs.h
	$(Optsize)

framain2.obj : framain2.c fractint.h fractype.h helpdefs.h
	$(Optsize)

frasetup.obj : frasetup.c

gifview.obj : gifview.c fractint.h

hcmplx.obj : hcmplx.c fractint.h

help.obj : help.c fractint.h helpdefs.h helpcom.h
	$(Optsize)

intro.obj : intro.c fractint.h helpdefs.h
	$(Optsize)

jb.obj : jb.c fractint.h helpdefs.h

jiim.obj : jiim.c helpdefs.h

line3d.obj : line3d.c fractint.h

loadfile.obj : loadfile.c fractint.h fractype.h
	$(Optsize)

loadfdos.obj : loadfdos.c fractint.h helpdefs.h
	$(Optsize)

loadmap.obj : loadmap.c targa.h fractint.h
	$(Optsize)

lorenz.obj : lorenz.c fractint.h fractype.h

lsys.obj : lsys.c fractint.h lsys.h

lsysf.obj : lsysf.c fractint.h lsys.h

memory.obj : memory.c

miscfrac.obj : miscfrac.c fractint.h mpmath.h

miscovl.obj : miscovl.c fractint.h fractype.h helpdefs.h
	$(Optsize)

miscres.obj : miscres.c fractint.h fractype.h helpdefs.h
	$(Optsize)

mpmath_c.obj : mpmath_c.c mpmath.h

parser.obj : parser.c fractint.h mpmath.h
	$(Optnoalias)

parserfp.obj : parserfp.c fractint.h mpmath.h
	$(Optnoalias)

plot3d.obj : plot3d.c fractint.h fractype.h
	$(Optnoalias)

printer.obj : printer.c fractint.h
	$(Optsize)

prompts1.obj : prompts1.c fractint.h fractype.h helpdefs.h
	$(Optsize)

prompts2.obj : prompts2.c fractint.h fractype.h helpdefs.h
	$(Optsize)

realdos.obj : realdos.c fractint.h helpdefs.h
	$(Optsize)

rotate.obj : rotate.c fractint.h helpdefs.h
	$(Optsize)

slideshw.obj : slideshw.c
	$(Optsize)

soi.obj : soi.c

soi1.obj : soi1.c

sound.obj  : sound.c
	$(Optnoalias)

stereo.obj : stereo.c helpdefs.h

targa.obj : targa.c targa.h fractint.h

testpt.obj: testpt.c fractint.h

tgaview.obj : tgaview.c fractint.h targa_lc.h port.h

tplus.obj : tplus.c tplus.h

uclock.obj  : uclock.c uclock.h

yourvid.obj : yourvid.c

zoom.obj : zoom.c fractint.h
	$(Optnoalias)
#	$(Optsize)

