SHELL = /bin/sh
STRIP = strip
INSTALL = /usr/bin/install

# Architecture
# automatic detection
ARCH = `uname -m | tr "_" "-"`
# ARCH = pentium
# ARCH = x86-64
# ARCH = athlon64

# Optimization flags
OPT = -O2
# OPT = -O

# Uncomment the second line if you want to compile with ncurses support
# (as in older versions of xfractint)
NCURSES =
# NCURSES = -DNCURSES

ifndef PREFIX
PREFIX = /usr
endif
ifndef DESTDIR
DESTDIR = $(PREFIX)
endif

# SRCDIR should be a path to the directory that will hold fractint.hlp
# SRCDIR should also hold the .par, .frm, etc. files
SRCDIR = $(DESTDIR)/share/xfractint
SHRDIR = $(PREFIX)/share/xfractint
# BINDIR is where you put your X11 binaries
BINDIR = $(DESTDIR)/bin
# MANDIR is where you put your chapter 1 man pages
MANDIR = $(DESTDIR)/share/man/man1

HFD = ./headers
UDIR = ./unix
COMDIR = ./common
DOSHELPDIR = ./dos_help

FDIR = formulas
IDIR = ifs
LDIR = lsystem
MDIR = maps
PDIR = pars
XDIR = extra

PWD = $(shell pwd)
BASEDIR = $(shell basename ${PWD})

NOBSTRING =
HAVESTRI =
DEBUG =

# For Ultrix, uncomment the NOBSTRING line below.
# For SunOS or Solaris, uncomment the NOBSTRING and HAVESTRI lines below, so
# bstring.h will not be included, and the library stricmp will be used.
# (Actually newer Solaris versions do not provide stricmp, so try without
# HAVESTRI if you run into problems.)
# For HPUX, uncomment the NOBSTRING line, change the DEFINES line, the CFLAGS
# line, the CC line, and the LIBS line.
# For AIX or OSF/1, change the DEFINES and LIB lines.
# For Apollo, uncomment the NOBSTRING line.  You must also remove the
#     source references to unistd.h, malloc.h, and alloc.h.
# For 386BSD, uncomment the NOBSTRING line.  Depending on your system, you
#     may have to change the "#elif !defined(__386BSD__)" at the top of
#     prompts2.c to "#else".
# For Red Hat Linux, uncomment the NOBSTRING line.
# For Cygwin, uncomment the NOBSTRING and HAVESTRI lines below.
#

NOBSTRING = -DNOBSTRING
#HAVESTRI = -DHAVESTRI
#DEBUG adds some sanity checking but will slow xfractint down
#DEBUG = -DEBUG
# If your compiler doesn't handle void *, define -DBADVOID
# If you get SIGFPE errors define -DFPUERR
# For HPUX, add -DSYS5
# and maybe add -DSYSV -D_CLASSIC_ANSI_TYPES
# For AIX, add -DNOBSTRING and -DDIRENT
# AIX may also need -D_POSIX_SOURCE -D_ALL_SOURCE -D_NONSTD_TYPES
# AIX may need -D_ALL_SOURCE -D_NONSTD_TYPES to compile help.c
# For Dec Alpha, add -DFTIME -DNOBSTRING -DDIRENT
# For SGI, you may have to add -DSYSVSGI
DEFINES = -DXFRACT $(NCURSES) $(NOBSTRING) $(HAVESTRI) $(DEBUG)

# Uncomment this if you get errors about "stdarg.h" missing.
#DEFINES += -DUSE_VARARGS

# To enable the long double type on Solaris, uncomment this and add
# "-lsunmath" to the LIBS definition below. Requires the sunmath library
# bundled with Sun C.
#DEFINES += -DUSE_SUNMATH

# Uncomment this for Cygwin
#DEFINES += -DCYGWIN -DDIRENT

# For using nasm, set:
#AS = /usr/bin/nasm
# Note that because of the differences between the assembler syntaxes,
#  nasm is the only one that will work.
AS = foo

# Below is for Linux with output file type of elf, turn all warnings on
AFLAGS = -f elf -w+orphan-labels

#Maybe -D_CONST will fix problems with constant type in include files?
#For HPUX, use CFLAGS = -I. $(DEFINES) -I/usr/include/X11R4 +O3 +Obb1000
#For SGI, add -cckr to CFLAGS
#For 386BSD, add -I/usr/X386/include to CFLAGS
#For Apollo add -I/usr/include/X11 to CFLAGS
#Some systems need -static on the CFLAGS.
#For Linux, add -DLINUX to CFLAGS
#If your version of Linux doesn't define SignalHandler add -DNOSIGHAND to CFLAGS
#For Solaris, use CFLAGS = -I. -I/usr/openwin/include $(DEFINES) -g
#If you have the nasm assembler on your system add -DNASM to CFLAGS

ifeq ($(AS),/usr/bin/nasm)

CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX -DNASM -fno-builtin
#CFLAGS = -I. -D_CONST $(DEFINES)
#CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX \
#         -march=$(ARCH) -DNASM -fno-builtin
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -DNASM -fno-builtin

else

CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX -fno-builtin
#CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX \
#         -march=$(ARCH) -fno-builtin
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -fno-builtin

endif

# Gcc is often the only compiler that works for this
# For HPUX, use CC = cc -Aa -D_HPUX_SOURCE
# For AIX, maybe use CC = xlc, but other AIX users found cc works, xlc doesn't.
# For Apollo use CC = cc -A cpu,mathlib_sr10  -A systype,bsd4.3
# For Sun Solaris 2.x w/SparcCompilerC (cc), use CC = cc.
# For Sun Solaris 2.x w/GNU gcc, use CC = gcc
#CC = gcc
CC = /usr/bin/gcc

# For HPUX, use LIBS = -L/usr/lib/X11R4 -lX11 -lm -lcurses -ltermcap
# For AIX or OSF/1, add -lbsd
# For 386BSD, add -L/usr/X386/lib to LIBS
# For Apollo, change -lX11 to -L/usr/X11/libX11
# For Solaris, add -L/usr/openwin/lib; change -lncurses to -lcurses
# if you get undefined symbols like "w32addch".
# For Linux, use
# LIBS = -L/usr/X11R6/lib -lX11 -lm -lncurses
# LIBS = -lX11 -lm -lcurses

ifeq ($(ARCH),athlon64)
LIBS = -L/usr/X11R6/lib64 -lX11 -lm
else
LIBS = -L/usr/X11R6/lib -lX11 -lm
endif

ifeq ($(NCURSES),-DNCURSES)
LIBS += -lncurses
endif

# HPUX fixes thanks to David Allport, Bill Broadley, and R. Lloyd.
# AIX fixes thanks to David Sanderson & Elliot Jaffe.
# OSF/1 fixes thanks to Ronald Record.
# 386BSD fixes thanks to Paul Richards and Andreas Gustafsson.
# Apollo fixes thanks to Carl Heidrich
# Linux fixes thanks to Darcy Boese
# Makefile dependency fixes thanks to Paul Roberts.
# Solaris fixes thanks to Darryl House

OLDSRC = \
$(COMDIR)/3d.c $(COMDIR)/ant.c $(COMDIR)/bigflt.c $(COMDIR)/biginit.c \
$(COMDIR)/bignum.c $(COMDIR)/bignumc.c $(COMDIR)/calcfrac.c \
$(COMDIR)/cmdfiles.c $(COMDIR)/decoder.c $(COMDIR)/editpal.c \
$(COMDIR)/encoder.c $(COMDIR)/evolve.c $(COMDIR)/f16.c \
$(COMDIR)/fracsubr.c $(COMDIR)/fractalb.c $(COMDIR)/fractalp.c \
$(COMDIR)/fractals.c $(COMDIR)/fractint.c $(COMDIR)/framain2.c \
$(COMDIR)/frasetup.c $(COMDIR)/gifview.c $(COMDIR)/hcmplx.c \
$(COMDIR)/help.c $(COMDIR)/history.c $(COMDIR)/intro.c \
$(COMDIR)/jb.c $(COMDIR)/jiim.c $(COMDIR)/line3d.c \
$(COMDIR)/loadfdos.c $(COMDIR)/loadfile.c $(COMDIR)/loadmap.c \
$(COMDIR)/lorenz.c $(COMDIR)/lsys.c $(COMDIR)/lsysf.c \
$(COMDIR)/memory.c $(COMDIR)/miscfrac.c $(COMDIR)/miscovl.c \
$(COMDIR)/miscres.c $(COMDIR)/mpmath_c.c $(COMDIR)/parser.c \
$(COMDIR)/parserfp.c $(COMDIR)/plot3d.c $(COMDIR)/printer.c \
$(COMDIR)/prompts1.c $(COMDIR)/prompts2.c $(COMDIR)/realdos.c \
$(COMDIR)/rotate.c $(COMDIR)/slideshw.c $(COMDIR)/soi.c \
$(COMDIR)/soi1.c $(COMDIR)/stereo.c $(COMDIR)/targa.c \
$(COMDIR)/testpt.c $(COMDIR)/tgaview.c $(COMDIR)/zoom.c $(COMDIR)/Makefile

NEWSRC = \
$(UDIR)/calcmand.c $(UDIR)/calmanfp.c $(UDIR)/diskvidu.c \
$(UDIR)/fpu087.c $(UDIR)/fracsuba.c $(UDIR)/general.c \
$(UDIR)/xfcurses.c $(UDIR)/video.c $(UDIR)/unix.c $(UDIR)/unixscr.c \
$(UDIR)/Makefile $(UDIR)/xfract_a.inc $(UDIR)/calmanfx.asm

HEADERS = \
$(HFD)/big.h $(HFD)/biginit.h $(HFD)/cmplx.h $(HFD)/externs.h \
$(HFD)/fmath.h $(HFD)/fractint.h $(HFD)/fractype.h $(HFD)/helpcom.h \
$(HFD)/lsys.h $(HFD)/mpmath.h $(HFD)/port.h $(HFD)/prototyp.h \
$(HFD)/targa.h $(HFD)/targa_lc.h $(HFD)/tplus.h $(HFD)/unix.h \
$(HFD)/xfcurses.h

DOCS = debugfla.txt fractsrc.txt hc.txt

HELPFILES = \
$(DOSHELPDIR)/help.src $(DOSHELPDIR)/help2.src $(DOSHELPDIR)/help3.src \
$(DOSHELPDIR)/help4.src $(DOSHELPDIR)/help5.src

SRCFILES = $(OLDSRC) $(NEWSRC) $(HELPFILES) $(HEADERS) $(DOCS)

PARFILES = \
$(PDIR)/cellular.par $(PDIR)/demo.par $(PDIR)/fract18.par \
$(PDIR)/fract19.par $(PDIR)/fract200.par $(PDIR)/fractint.par \
$(PDIR)/icons.par $(PDIR)/lyapunov.par $(PDIR)/music.par \
$(PDIR)/newphoen.par $(PDIR)/orbits.par $(PDIR)/phoenix.par

FRMFILES = \
$(FDIR)/fractint.frm $(FDIR)/fract200.frm $(FDIR)/fract196.frm \
$(FDIR)/fract001.frm $(FDIR)/fract002.frm $(FDIR)/fract003.frm \
$(FDIR)/fract_sy.frm $(FDIR)/ikenaga.frm $(FDIR)/julitile.frm \
$(FDIR)/new_if.frm $(FDIR)/newton.frm

IFSFILES = $(IDIR)/fractint.ifs

LFILES = $(LDIR)/fractint.l $(LDIR)/penrose.l $(LDIR)/tiling.l

MAPFILES = \
$(MDIR)/altern.map $(MDIR)/blues.map $(MDIR)/chroma.map \
$(MDIR)/default.map $(MDIR)/firestrm.map $(MDIR)/froth3.map \
$(MDIR)/froth316.map $(MDIR)/froth6.map $(MDIR)/froth616.map \
$(MDIR)/gamma1.map $(MDIR)/gamma2.map $(MDIR)/glasses1.map \
$(MDIR)/glasses2.map $(MDIR)/goodega.map $(MDIR)/green.map \
$(MDIR)/grey.map $(MDIR)/grid.map $(MDIR)/headache.map \
$(MDIR)/landscap.map $(MDIR)/lyapunov.map $(MDIR)/neon.map \
$(MDIR)/paintjet.map $(MDIR)/royal.map $(MDIR)/topo.map $(MDIR)/volcano.map

XTRAFILES = \
$(XDIR)/all_maps.zip $(XDIR)/frmtut.zip $(XDIR)/if_else.zip \
$(XDIR)/phctutor.zip

OLDRUN = $(PARFILES) $(FRMFILES) $(IFSFILES) $(LFILES) $(MAPFILES)

NEWRUN = fractint.doc read.me $(UDIR)/xfractint.man

NEWFILES = $(UDIR)/$(NEWSRC) $(NEWRUN)

RUNFILES = $(OLDRUN) $(NEWRUN)

FILES = $(SRCFILES) $(RUNFILES)

OBJS = \
$(COMDIR)/3d.o $(COMDIR)/ant.o $(COMDIR)/bigflt.o  $(COMDIR)/biginit.o \
$(COMDIR)/bignum.o $(COMDIR)/bignumc.o $(COMDIR)/calcfrac.o \
$(COMDIR)/cmdfiles.o $(COMDIR)/decoder.o $(COMDIR)/editpal.o \
$(COMDIR)/encoder.o $(COMDIR)/evolve.o $(COMDIR)/f16.o $(COMDIR)/fracsubr.o \
$(COMDIR)/fractalb.o $(COMDIR)/fractalp.o $(COMDIR)/fractals.o \
$(COMDIR)/fractint.o $(COMDIR)/framain2.o $(COMDIR)/frasetup.o \
$(COMDIR)/gifview.o $(COMDIR)/hcmplx.o $(COMDIR)/help.o $(COMDIR)/history.o\
$(COMDIR)/intro.o $(COMDIR)/jb.o $(COMDIR)/jiim.o $(COMDIR)/line3d.o \
$(COMDIR)/loadfdos.o $(COMDIR)/loadfile.o $(COMDIR)/loadmap.o \
$(COMDIR)/lorenz.o $(COMDIR)/lsys.o $(COMDIR)/lsysf.o $(COMDIR)/memory.o \
$(COMDIR)/miscfrac.o $(COMDIR)/miscovl.o $(COMDIR)/miscres.o \
$(COMDIR)/mpmath_c.o $(COMDIR)/parser.o $(COMDIR)/parserfp.o \
$(COMDIR)/plot3d.o $(COMDIR)/printer.o $(COMDIR)/prompts1.o \
$(COMDIR)/prompts2.o $(COMDIR)/realdos.o $(COMDIR)/rotate.o \
$(COMDIR)/slideshw.o $(COMDIR)/soi.o $(COMDIR)/soi1.o $(COMDIR)/stereo.o \
$(COMDIR)/targa.o $(COMDIR)/testpt.o $(COMDIR)/tgaview.o \
$(COMDIR)/zoom.o


ifeq ($(AS),/usr/bin/nasm)

U_OBJS = \
$(UDIR)/calcmand.o $(UDIR)/calmanfp.o $(UDIR)/diskvidu.o $(UDIR)/fpu087.o \
$(UDIR)/fracsuba.o $(UDIR)/general.o $(UDIR)/unix.o $(UDIR)/xfcurses.o \
$(UDIR)/unixscr.o $(UDIR)/video.o \
$(UDIR)/calmanfx.o

else

U_OBJS = \
$(UDIR)/calcmand.o $(UDIR)/calmanfp.o $(UDIR)/diskvidu.o $(UDIR)/fpu087.o \
$(UDIR)/fracsuba.o $(UDIR)/general.o $(UDIR)/unix.o $(UDIR)/xfcurses.o \
$(UDIR)/unixscr.o $(UDIR)/video.o

endif

HOBJS = $(DOSHELPDIR)/hc.o unix.o

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

xfractint: fractint.hlp $(SRCFILES)
	if [ -f $(DOSHELPDIR)/helpdefs.h ] ; then mv -f $(DOSHELPDIR)/helpdefs.h $(HFD) ; fi
	cd common ; ${MAKE} all "CC=${CC}" "CFLAGS= -I.${HFD} ${CFLAGS} ${OPT}" "SRCDIR=${SHRDIR}" \
	          "HFD=.${HFD}"
	cd unix ; ${MAKE} all "CC=${CC}" "CFLAGS= -I.${HFD} ${CFLAGS} ${OPT}" "SRCDIR=${SHRDIR}" \
	          "AS=${AS}" "AFLAGS=${AFLAGS}" "HFD=.${HFD}"
	$(CC) -o xfractint $(CFLAGS) $(OPT) $(OBJS) $(U_OBJS) $(LIBS)
#	strip xfractint

fractint:
	if [ -x xfractint ] ; then mv -f xfractint xfractint.x11 ; fi
	rm -f common/encoder.o common/help.o common/realdos.o
	rm -f unix/unixscr.o unix/video.o unix/xfcurses.o
	make NCURSES=-DNCURSES ; mv xfractint fractint
	rm -f common/encoder.o common/help.o common/realdos.o
	rm -f unix/unixscr.o unix/video.o unix/xfcurses.o	
	if [ -x xfractint.x11 ] ; then mv -f xfractint.x11 xfractint ; fi
	
# tar: $(FILES)
#	tar cvfj xfractint.tar.bz2 $(FILES)

tar: clean
	cd .. ; tar cvfj $(BASEDIR).tar.bz2 $(BASEDIR)

tidy:
	rm -f $(HOBJS)
	cd common ; ${MAKE} tidy
	cd unix ; ${MAKE} tidy

clean:
	rm -f build-stamp *~ */*~ core
	rm -f $(HOBJS) fractint.doc fractint.hlp hc fractint xfractint
	rm -f $(HFD)/helpdefs.h
	cd $(COMDIR) ; ${MAKE} clean
	cd $(UDIR) ; ${MAKE} clean

install: xfractint fractint.hlp
	$(STRIP) xfractint
	$(INSTALL) -d $(BINDIR) $(MANDIR) $(SRCDIR)/$(PDIR) $(SRCDIR)/$(FDIR) \
		$(SRCDIR)/$(IDIR) $(SRCDIR)/$(LDIR) $(SRCDIR)/$(MDIR) $(SRCDIR)/$(XDIR)
	$(INSTALL) xfractint -T $(BINDIR)/xfractint;
	$(INSTALL) -m 644 -T $(UDIR)/xfractint.man $(MANDIR)/xfractint.1;
	$(INSTALL) -m 644 -t $(SRCDIR) fractint.hlp sstools.ini $(DOCS)
	$(INSTALL) -m 644 -t $(SRCDIR)/$(PDIR) $(PARFILES)
	$(INSTALL) -m 644 -t $(SRCDIR)/$(FDIR) $(FRMFILES)
	$(INSTALL) -m 644 -t $(SRCDIR)/$(IDIR) $(IFSFILES)
	$(INSTALL) -m 644 -t $(SRCDIR)/$(LDIR) $(LFILES)
	$(INSTALL) -m 644 -t $(SRCDIR)/$(MDIR) $(MAPFILES)
	$(INSTALL) -m 644 -t $(SRCDIR)/$(XDIR) $(XTRAFILES)

uninstall:
	cd $(SRCDIR); rm -f $(PARFILES)
	cd $(SRCDIR); rm -f $(FRMFILES)
	cd $(SRCDIR); rm -f $(IFSFILES)
	cd $(SRCDIR); rm -f $(LFILES)
	cd $(SRCDIR); rm -f $(MAPFILES)
	cd $(SRCDIR); rm -f $(XTRAFILES)
	cd $(SRCDIR); rm -f fractint.hlp sstools.ini $(DOCS)
	cd $(SRCDIR); rmdir $(PDIR) $(FDIR) $(IDIR) $(LDIR) $(MDIR) $(XDIR)
# only next 2 lines might need su
	cd $(SRCDIR); cd ..; rmdir $(SRCDIR)
	rm -f $(BINDIR)/xfractint $(MANDIR)/xfractint.1

fractint.hlp: hc $(DOSHELPDIR)/$(HELP)
	cd $(DOSHELPDIR); ../hc /c; mv fractint.hlp ..

fractint.doc: doc

doc: hc $(HELPFILES)
	cd $(DOSHELPDIR) ; ../hc /p ; mv -f fractint.doc ..

hc:	$(HOBJS)
	$(CC) -o hc $(CFLAGS) $(HOBJS)

unix.o: $(UDIR)/unix.c
	$(CC) $(CFLAGS) $(OPT) -DSRCDIR=\"$(SHRDIR)\" -c $(UDIR)/unix.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.

hc.o: $(DOSHELPDIR)/hc.c $(HFD)/helpcom.h $(HFD)/port.h

