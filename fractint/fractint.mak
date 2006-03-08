
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

# the following klooge lets us define an alternate link/def file for
# the new overlay structure available under MSC7
#DEBUG = 1

!ifndef WINFRACT
!ifdef C7
DEFFILE  = fractint.def
LINKFILE = fractint.lnk
LINKER="link /dynamic:1024 $(LINKFILE)"
!else
DEFFILE  =
LINKFILE = fractint.lnk
!endif
!else
DEFFILE  = winfract.def
LINKFILE = winfract.lnk
LINKER = link

CDEBUG = /Oeciltaz /Ob2 /Gs
LDEBUG =
!ifdef MSC6
CDEBUG = /Oeciltaz /Gs
!endif
!ifdef DEBUG
CDEBUG = /Zi /Od
LDEBUG = /CO
!endif
!endif

HFD = .\headers
COMDIR = .\common
DOSDIR = .\dos
WINDIR = .\win

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

!ifndef WINFRACT
all : common dos fractint.exe
!else
all : common dos win winfract.hlp res
!endif


!ifdef WINFRACT
common :
	cd $(COMDIR)
	$(MAKE) all /F comwin.mak
	cd ..

dos :
	cd $(DOSDIR)
	$(MAKE) all /F doswin.mak
	cd ..

win :
	cd $(WINDIR)
	$(MAKE) all /F win.mak
	cd ..

.rc.res:
	  rc -r -i$(HFD) $*.rc

winfract.hlp: winfract.rtf mathtool.rtf
	hc winfract

winfract.res: winfract.rc mathtool.rc $(HFD)\mathtool.h coord.dlg $(HFD)\winfract.h \
	  $(HFD)\dialog.h zoom.dlg
	rc -r -i$(HFD) winfract.rc
!else
common :
	cd $(COMDIR)
	$(MAKE) all /F common.mak
	cd ..

dos :
	cd $(DOSDIR)
	$(MAKE) all /F dos.mak
	cd ..
!endif


!ifdef WINFRACT
winfract.exe : $(COMDIR)\3d.obj $(COMDIR)\ant.obj $(COMDIR)\bigflt.obj \
     $(COMDIR)\biginit.obj $(COMDIR)\bignum.obj $(COMDIR)\calcfrac.obj \
     $(COMDIR)\decoder.obj $(COMDIR)\editpal.obj $(COMDIR)\encoder.obj \
     $(COMDIR)\evolve.obj $(COMDIR)\f16.obj $(COMDIR)\fracsubr.obj \
     $(COMDIR)\fractals.obj $(COMDIR)\fractalp.obj $(COMDIR)\fractalb.obj \
     $(COMDIR)\frasetup.obj $(COMDIR)\gifview.obj $(COMDIR)\hcmplx.obj \
     $(COMDIR)\help.obj $(COMDIR)\jb.obj $(COMDIR)\jiim.obj \
     $(COMDIR)\line3d.obj $(COMDIR)\loadfile.obj $(COMDIR)\loadmap.obj \
     $(COMDIR)\lorenz.obj $(COMDIR)\lsys.obj $(COMDIR)\lsysf.obj \
     $(COMDIR)\memory.obj $(COMDIR)\miscfrac.obj $(COMDIR)\miscovl.obj \
     $(COMDIR)\miscres.obj $(COMDIR)\mpmath_c.obj $(COMDIR)\parser.obj \
     $(COMDIR)\parserfp.obj $(COMDIR)\plot3d.obj $(COMDIR)\prompts1.obj \
     $(COMDIR)\prompts2.obj $(COMDIR)\soi.obj $(COMDIR)\soi1.obj \
     $(COMDIR)\stereo.obj $(COMDIR)\testpt.obj $(COMDIR)\tgaview.obj \
     $(DOSDIR)\uclock.obj $(DOSDIR)\bignuma.obj $(DOSDIR)\calcmand.obj \
     $(DOSDIR)\calmanp5.obj $(DOSDIR)\calmanfp.obj $(DOSDIR)\fpu087.obj \
     $(DOSDIR)\fpu387.obj $(DOSDIR)\fracsuba.obj $(DOSDIR)\lsysa.obj \
     $(DOSDIR)\lsysaf.obj $(DOSDIR)\lyapunov.obj $(DOSDIR)\mpmath_a.obj \
     $(DOSDIR)\newton.obj $(DOSDIR)\parsera.obj \
     $(WINDIR)\dialog.obj $(WINDIR)\mainfrac.obj $(WINDIR)\mathtool.obj \
     $(WINDIR)\profile.obj $(WINDIR)\select.obj \
     $(WINDIR)\windos.obj $(WINDIR)\windos2.obj $(WINDIR)\winfract.obj \
     $(WINDIR)\winstubs.obj $(WINDIR)\wintext.obj $(WINDIR)\wgeneral.obj \
     $(DEFFILE) $(LINKFILE)
     $(LINKER) $(LDEBUG) /NOE @$(LINKFILE) > foo

res: winfract.res winfract.exe
	 rc -k -i$(HFD) winfract
!else
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

!endif

