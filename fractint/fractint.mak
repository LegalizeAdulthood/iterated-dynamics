
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

HFD=../headers


# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : common fractint.exe

.asm.obj:
	$(AS) /W3 $*; >> f_errs.txt
# for Quick Assembler
#       $(AS) $*.asm

!ifndef DEBUG
.c.obj:
	  $(CC) /AM /W4 /FPi /c /I$(HDF) $(OptT) $*.c >> f_errs.txt

Optsize = $(CC) /AM /W4 /FPi /c /I$(HDF) $(OptS) $*.c >> f_errs.txt

Optnoalias = $(CC) /AM /W4 /FPi /c /I$(HDF) $(OptN) $*.c >> f_errs.txt
!else
.c.obj:
	  $(CC) /Zi /AM /W4 /FPi /c /I$(HDF) $(OptT) $*.c >> f_errs.txt

Optsize = $(CC) /Zi /AM /W4 /FPi /c /I$(HDF) $(OptS) $*.c >> f_errs.txt

Optnoalias = $(CC) /Zi /AM /W4 /FPi /c /I$(HDF) /DTESTFP $(OptN) $*.c >> f_errs.txt
!endif

common :
	cd .\common
	$(MAKE) all /I$(HDF) /V /F common.mak
#	cd ..

lsysa.obj: lsysa.asm

fracsuba.obj : fracsuba.asm

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

calcmand.obj : calcmand.asm

calmanfp.obj : calmanfp.asm
# for MASM
	$(AS) /e /W3 calmanfp; >> f_errs.txt
#	$(AS) /e calmanfp;
# for QuickAssembler
#   $(AS) /FPi calmanfp.asm

calmanp5.obj : calmanp5.asm
	$(AS) /W3 calmanp5; >> f_errs.txt

hgcfra.obj : hgcfra.asm

fr8514a.obj : fr8514a.asm

video.obj : video.asm

general.obj : general.asm

newton.obj : newton.asm
	$(AS) /e newton;

fpu387.obj : fpu387.asm

fpu087.obj : fpu087.asm
	$(AS) /e /W3 fpu087; >> f_errs.txt

mpmath_a.obj : mpmath_a.asm

tplus_a.obj : tplus_a.asm

lyapunov.obj : lyapunov.asm
	$(AS) /e /W3 lyapunov; >> f_errs.txt

bignuma.obj : bignuma.asm big.inc
# for MASM
        $(AS) /e /W3 bignuma.asm; >> f_errs.txt
# for QuickAssembler
#   $(AS) /FPi bignuma.asm >> f_errs.txt

lsysaf.obj: lsysaf.asm
# for MASM
	$(AS) /e /W3 lsysaf.asm; >> f_errs.txt
# for QuickAssembler
#   $(AS) /FPi lsysaf.asm

fractint.exe : fractint.obj help.obj loadfile.obj encoder.obj gifview.obj \
     general.obj calcmand.obj calmanfp.obj fractals.obj fractalp.obj calcfrac.obj \
     testpt.obj decoder.obj rotate.obj yourvid.obj prompts1.obj prompts2.obj parser.obj \
     parserfp.obj parsera.obj diskvid.obj line3d.obj 3d.obj newton.obj cmdfiles.obj \
     intro.obj slideshw.obj jiim.obj miscfrac.obj \
     targa.obj loadmap.obj printer.obj fracsubr.obj fracsuba.obj \
     video.obj tgaview.obj f16.obj fr8514a.obj loadfdos.obj stereo.obj\
     hgcfra.obj fpu087.obj fpu387.obj mpmath_c.obj mpmath_a.obj \
     lorenz.obj plot3d.obj jb.obj zoom.obj miscres.obj miscovl.obj \
     realdos.obj lsys.obj lsysa.obj editpal.obj tplus.obj tplus_a.obj \
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
