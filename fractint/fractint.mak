
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

HFD=.\headers
COMDIR = .\common
DOSDIR = .\dos

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : common dos fractint.exe

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
	cd $(COMDIR)
	$(MAKE) all /F common.mak
	cd ..

dos :
	cd $(DOSDIR)
	$(MAKE) all /F dos.mak
	cd ..

fractint.exe : $(COMDIR)\3d.obj $(COMDIR)\ant.obj $(COMDIR)\bigflt.obj \
     $(COMDIR)\biginit.obj $(COMDIR)\bignum.obj $(COMDIR)\calcfrac.obj \
     $(COMDIR)\cmdfiles.obj $(COMDIR)\decoder.obj $(COMDIR)\diskvid.obj \
     $(COMDIR)\editpal.obj $(COMDIR)\encoder.obj $(COMDIR)\evolve.obj \
     $(COMDIR)\f16.obj $(COMDIR)\fracsubr.obj $(COMDIR)\fractals.obj \
     $(COMDIR)\fractalp.obj $(COMDIR)\fractalb.obj $(COMDIR)\fractint.obj \
     $(COMDIR)\framain2.obj $(COMDIR)\frasetup.obj $(COMDIR)\gifview.obj \
     $(COMDIR)\hcmplx.obj $(COMDIR)\help.obj $(COMDIR)\intro.obj \
     $(COMDIR)\jb.obj $(COMDIR)\jiim.obj $(COMDIR)\line3d.obj \
     $(COMDIR)\loadfile.obj $(COMDIR)\loadfdos.obj $(COMDIR)\loadmap.obj \
     $(COMDIR)\lorenz.obj $(COMDIR)\lsys.obj $(COMDIR)\lsysf.obj \
     $(COMDIR)\memory.obj $(COMDIR)\miscfrac.obj $(COMDIR)\miscovl.obj \
     $(COMDIR)\miscres.obj $(COMDIR)\mpmath_c.obj $(COMDIR)\parser.obj \
     $(COMDIR)\parserfp.obj $(COMDIR)\plot3d.obj $(COMDIR)\printer.obj \
     $(COMDIR)\prompts1.obj $(COMDIR)\prompts2.obj $(COMDIR)\realdos.obj \
     $(COMDIR)\rotate.obj $(COMDIR)\slideshw.obj $(COMDIR)\soi.obj \
     $(COMDIR)\soi1.obj $(COMDIR)\stereo.obj $(COMDIR)\targa.obj \
     $(COMDIR)\testpt.obj $(COMDIR)\tgaview.obj \
     $(COMDIR)\yourvid.obj $(COMDIR)\zoom.obj \
     $(DOSDIR)\sound.obj $(DOSDIR)\tplus.obj $(DOSDIR)\uclock.obj \
     $(DOSDIR)\bignuma.obj $(DOSDIR)\calcmand.obj $(DOSDIR)\calmanp5.obj \
     $(DOSDIR)\calmanfp.obj $(DOSDIR)\fpu087.obj $(DOSDIR)\fpu387.obj \
     $(DOSDIR)\fr8514a.obj $(DOSDIR)\fracsuba.obj $(DOSDIR)\general.obj \
     $(DOSDIR)\hgcfra.obj $(DOSDIR)\lsysa.obj $(DOSDIR)\lsysaf.obj \
     $(DOSDIR)\lyapunov.obj $(DOSDIR)\mpmath_a.obj $(DOSDIR)\newton.obj \
     $(DOSDIR)\parsera.obj $(DOSDIR)\tplus_a.obj $(DOSDIR)\video.obj \
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
