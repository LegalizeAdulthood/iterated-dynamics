
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

# the following klooge lets us define an alternate link/def file for 
# the new overlay structure available under MSC7
#DEBUG = 1
!ifdef C7
DEFFILE  = fractint.def
LINKFILE = fractint.lnk
LINKER="link /dynamic:1024 $(LINKER)"
!else
DEFFILE  = 
LINKFILE = fractint.lnk
!endif

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : fractint.exe

.asm.obj:
	$(AS) /W3 $*; >> f_errs.txt
# for Quick Assembler
#       $(AS) $*.asm

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

lorenz.obj : lorenz.c fractint.h fractype.h

lsys.obj : lsys.c fractint.h lsys.h

plot3d.obj : plot3d.c fractint.h fractype.h
	$(Optnoalias)

3d.obj : 3d.c fractint.h

fractals.obj : fractals.c fractint.h fractype.h mpmath.h helpdefs.h

fractalp.obj : fractalp.c fractint.h fractype.h mpmath.h helpdefs.h

fractalb.obj : fractalb.c fractint.h fractype.h big.h helpdefs.h

calcfrac.obj : calcfrac.c fractint.h mpmath.h

miscfrac.obj : miscfrac.c fractint.h mpmath.h

fracsubr.obj : fracsubr.c fractint.h helpdefs.h
	$(Optnoalias)
       
jiim.obj : jiim.c helpdefs.h

fracsuba.obj : fracsuba.asm

parser.obj : parser.c fractint.h mpmath.h
	$(Optnoalias)

parserfp.obj : parserfp.c fractint.h mpmath.h
	$(Optnoalias)

parsera.obj: parsera.asm
# for MASM
!ifndef DEBUG
	$(AS) /W3 parsera; >> f_errs.txt
#	$(AS) /e parsera;
!else
	$(AS) /e /Zi parsera;
!endif
# for QuickAssembler
#   $(AS) /FPi parsera.asm

calmanfp.obj : calmanfp.asm
# for MASM
	$(AS) /e /W3 calmanfp; >> f_errs.txt
#	$(AS) /e calmanfp;
# for QuickAssembler
#   $(AS) /FPi calmanfp.asm

calmanp5.obj : calmanp5.asm
	$(AS) /W3 calmanp5; >> f_errs.txt

cmdfiles.obj : cmdfiles.c fractint.h
	$(Optsize)

loadfile.obj : loadfile.c fractint.h fractype.h
	$(Optsize)

loadfdos.obj : loadfdos.c fractint.h helpdefs.h
	$(Optsize)

decoder.obj : decoder.c fractint.h

diskvid.obj : diskvid.c fractint.h

encoder.obj : encoder.c fractint.h fractype.h

fr8514a.obj : fr8514a.asm

hgcfra.obj : hgcfra.asm

fractint.obj : fractint.c fractint.h fractype.h helpdefs.h
	$(Optsize)

framain2.obj : framain2.c fractint.h fractype.h helpdefs.h
	$(Optsize)

video.obj : video.asm

general.obj : general.asm

gifview.obj : gifview.c fractint.h

tgaview.obj : tgaview.c fractint.h targa_lc.h port.h

help.obj : help.c fractint.h helpdefs.h helpcom.h
	$(Optsize)

intro.obj : intro.c fractint.h helpdefs.h
	$(Optsize)

line3d.obj : line3d.c fractint.h

newton.obj : newton.asm
	$(AS) /e newton;

printer.obj : printer.c fractint.h
	$(Optsize)

prompts1.obj : prompts1.c fractint.h fractype.h helpdefs.h
	$(Optsize)

prompts2.obj : prompts2.c fractint.h fractype.h helpdefs.h
	$(Optsize)

rotate.obj : rotate.c fractint.h helpdefs.h
	$(Optsize)

editpal.obj : editpal.c fractint.h
	$(Optsize)

testpt.obj: testpt.c fractint.h

targa.obj : targa.c targa.h fractint.h

loadmap.obj : loadmap.c targa.h fractint.h
	$(Optsize)

yourvid.obj : yourvid.c

fpu387.obj : fpu387.asm

fpu087.obj : fpu087.asm
	$(AS) /e /W3 fpu087; >> f_errs.txt

f16.obj : f16.c targa_lc.h

mpmath_c.obj : mpmath_c.c mpmath.h

hcmplx.obj : hcmplx.c fractint.h

jb.obj : jb.c fractint.h helpdefs.h

zoom.obj : zoom.c fractint.h
	$(Optnoalias)
#	$(Optsize)

miscres.obj : miscres.c fractint.h fractype.h helpdefs.h
	$(Optsize)

miscovl.obj : miscovl.c fractint.h fractype.h helpdefs.h
	$(Optsize)

realdos.obj : realdos.c fractint.h helpdefs.h
	$(Optsize)

tplus.obj : tplus.c tplus.h

tplus_a.obj : tplus_a.asm

lyapunov.obj : lyapunov.asm
	$(AS) /e /W3 lyapunov; >> f_errs.txt

slideshw.obj : slideshw.c
	$(Optsize)

biginit.obj : biginit.c big.h
	$(Optnoalias)

bignum.obj : bignum.c big.h
	$(Optnoalias)

bigflt.obj : bigflt.c big.h
	$(Optnoalias)

bignuma.obj : bignuma.asm big.inc
# for MASM
        $(AS) /e /W3 bignuma.asm; >> f_errs.txt
# for QuickAssembler
#   $(AS) /FPi bignuma.asm >> f_errs.txt

# only used for non ASM version
#bignumc.obj : bignumc.c big.h
#        $(Optnoalias)

stereo.obj : stereo.c helpdefs.h 

ant.obj : ant.c helpdefs.h

frasetup.obj : frasetup.c 

lsysf.obj : lsysf.c fractint.h lsys.h

lsysaf.obj: lsysaf.asm
# for MASM
	$(AS) /e /W3 lsysaf.asm; >> f_errs.txt
# for QuickAssembler
#   $(AS) /FPi lsysaf.asm

memory.obj : memory.c

soi.obj : soi.c

soi1.obj : soi1.c

evolve.obj : evolve.c fractint.h
        $(Optnoalias)

sound.obj  : sound.c
	$(Optnoalias)

uclock.obj  : uclock.c uclock.h

fractint.exe : fractint.obj help.obj loadfile.obj encoder.obj gifview.obj \
     general.obj calmanfp.obj fractals.obj fractalp.obj calcfrac.obj \
     testpt.obj decoder.obj rotate.obj yourvid.obj prompts1.obj prompts2.obj parser.obj \
     parserfp.obj parsera.obj diskvid.obj line3d.obj 3d.obj newton.obj cmdfiles.obj \
     intro.obj slideshw.obj jiim.obj miscfrac.obj \
     targa.obj loadmap.obj printer.obj fracsubr.obj fracsuba.obj \
     video.obj tgaview.obj f16.obj fr8514a.obj loadfdos.obj stereo.obj\
     hgcfra.obj fpu087.obj fpu387.obj mpmath_c.obj \
     lorenz.obj plot3d.obj jb.obj zoom.obj miscres.obj miscovl.obj \
     realdos.obj lsys.obj editpal.obj tplus.obj tplus_a.obj \
     lyapunov.obj fractint.hlp hcmplx.obj \
     biginit.obj bignum.obj bigflt.obj bignuma.obj \
     fractalb.obj ant.obj frasetup.obj framain2.obj \
     lsysf.obj lsysaf.obj memory.obj evolve.obj soi.obj \
     soi1.obj calmanp5.obj sound.obj uclock.obj\
     $(DEFFILE) $(LINKFILE)
!ifndef DEBUG
	$(LINKER) /ST:8800 /SE:160 /PACKC /F /NOE @$(LINKFILE) > foo
!else
	$(LINKER) /CO /ST:7500 /SE:210 /PACKC /F /NOE @$(LINKFILE) > foo
!endif
!ifdef C7
        @echo (Any overlay_thunks (L4059) warnings from the linker are harmless) >> foo
!endif
#        more < f_errs.txt
#	type foo
        
!ifndef DEBUG
	hc /a
!endif
