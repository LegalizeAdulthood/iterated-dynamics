SHELL=/bin/sh

# SRCDIR should be a path to the directory that will hold fractint.hlp
# SRCDIR should also hold the .par, .frm, etc. files
SRCDIR = /usr/share/xfractint
# BINDIR is where you put your X11 binaries
BINDIR = /usr/X11R6/bin
# MANDIR is where you put your chapter 1 man pages
MANDIR = /usr/X11R6/man/man1

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
DEFINES = -DXFRACT $(NOBSTRING) $(HAVESTRI) $(DEBUG)

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

#CFLAGS = -I. -D_CONST $(DEFINES)
CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX -g \
         -march=pentium -DNASM -fno-builtin
#         -march=athlon64 -DNASM -fno-builtin
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -DNASM -fno-builtin

else

CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX -g \
         -march=pentium -fno-builtin
#         -march=athlon64 -fno-builtin
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
#LIBS = -L/usr/X11R6/lib -lX11 -lm -lncurses
LIBS = -L/usr/X11R6/lib -lX11 -lm -lncurses
#LIBS = -L/usr/X11R6/lib64 -lX11 -lm -lncurses
#LIBS = -lX11 -lm -lcurses

# HPUX fixes thanks to David Allport, Bill Broadley, and R. Lloyd.
# AIX fixes thanks to David Sanderson & Elliot Jaffe.
# OSF/1 fixes thanks to Ronald Record.
# 386BSD fixes thanks to Paul Richards and Andreas Gustafsson.
# Apollo fixes thanks to Carl Heidrich
# Linux fixes thanks to Darcy Boese
# Makefile dependency fixes thanks to Paul Roberts.
# Solaris fixes thanks to Darryl House

OLDSRC = \
3d.c ant.c bigflt.c biginit.c bignum.c \
bignumc.c calcfrac.c cmdfiles.c decoder.c editpal.c \
encoder.c evolve.c f16.c fracsubr.c fractalb.c fractalp.c \
fractals.c fractint.c framain2.c \
frasetup.c gifview.c hc.c hcmplx.c help.c history.c \
intro.c jb.c jiim.c line3d.c loadfdos.c loadfile.c loadmap.c lorenz.c \
lsys.c lsysf.c memory.c miscfrac.c miscovl.c miscres.c \
mpmath_c.c parser.c parserfp.c plot3d.c prompts1.c \
prompts2.c realdos.c rotate.c slideshw.c soi.c soi1.c stereo.c \
targa.c testpt.c tgaview.c zoom.c Makefile

NEWSRC = calcmand.c calmanfp.c diskvidu.c \
fpu087.c fracsuba.c general.c \
video.c unix.c unixscr.c unix.h Makefile xfract_a.inc \
calmanfx.asm

HEADERS = big.h biginit.h cmplx.h externs.h fmath.h fractint.h fractype.h \
helpcom.h lsys.h mpmath.h port.h prototyp.h targa.h targa_lc.h tplus.h

DOCS = debugfla.doc fractsrc.doc hc.doc

HELPFILES = help.src help2.src help3.src help4.src help5.src

SRCFILES = $(COMDIR)/$(OLDSRC) $(UDIR)/$(NEWSRC) $(DOSHELPDIR)/$(HELPFILES) \
$(HFD)/$(HEADERS) $(DOCS)

PARFILES = \
cellular.par demo.par fract18.par fract19.par fract200.par fractint.par \
icons.par lyapunov.par music.par newphoen.par orbits.par phoenix.par

FRMFILES = fractint.frm fract200.frm fract196.frm fract001.frm fract002.frm \
fract003.frm fract_sy.frm ikenaga.frm julitile.frm new_if.frm \
newton.frm fend.frm

IFSFILES = fractint.ifs

LFILES = fractint.l penrose.l tiling.l

MAPFILES = \
altern.map blues.map chroma.map default.map firestrm.map \
froth3.map froth316.map froth6.map froth616.map gamma1.map gamma2.map \
glasses1.map glasses2.map goodega.map green.map grey.map grid.map headache.map \
landscap.map lyapunov.map neon.map paintjet.map \
royal.map topo.map volcano.map

XTRAFILES = \
all_maps.zip frmtut.zip if_else.zip phctutor.zip

OLDRUN = $(PDIR)/$(PARFILES) $(FDIR)/$(FRMFILES) $(IDIR)/$(IFSFILES) \
$(LDIR)/$(LFILES) $(MDIR)/$(MAPFILES) demo.key

NEWRUN = fractint.doc read.me $(UDIR)/xfractint.man

NEWFILES = $(UDIR)/$(NEWSRC) $(NEWRUN)

RUNFILES = $(OLDRUN) $(NEWRUN)

FILES = $(SRCFILES) $(RUNFILES)

OBJS = \
$(COMDIR)/3d.o $(COMDIR)/ant.o $(COMDIR)/bigflt.o  $(COMDIR)/biginit.o \
$(COMDIR)/bignum.o $(COMDIR)/bignumc.o $(COMDIR)/calcfrac.o \
$(COMDIR)/cmdfiles.o $(COMDIR)/decoder.o \
$(COMDIR)/drivers.o $(COMDIR)/editpal.o \
$(COMDIR)/encoder.o $(COMDIR)/evolve.o $(COMDIR)/f16.o $(COMDIR)/fracsubr.o \
$(COMDIR)/fractalb.o $(COMDIR)/fractalp.o $(COMDIR)/fractals.o \
$(COMDIR)/fractint.o $(COMDIR)/framain2.o $(COMDIR)/frasetup.o \
$(COMDIR)/gifview.o $(COMDIR)/hcmplx.o $(COMDIR)/help.o $(COMDIR)/history.o \
$(COMDIR)/intro.o $(COMDIR)/jb.o $(COMDIR)/jiim.o $(COMDIR)/line3d.o \
$(COMDIR)/loadfdos.o $(COMDIR)/loadfile.o $(COMDIR)/loadmap.o \
$(COMDIR)/lorenz.o $(COMDIR)/lsys.o $(COMDIR)/lsysf.o $(COMDIR)/memory.o \
$(COMDIR)/miscfrac.o $(COMDIR)/miscovl.o $(COMDIR)/miscres.o \
$(COMDIR)/mpmath_c.o $(COMDIR)/parser.o $(COMDIR)/parserfp.o \
$(COMDIR)/plot3d.o $(COMDIR)/prompts1.o \
$(COMDIR)/prompts2.o $(COMDIR)/realdos.o $(COMDIR)/rotate.o \
$(COMDIR)/slideshw.o $(COMDIR)/soi.o $(COMDIR)/soi1.o $(COMDIR)/stereo.o \
$(COMDIR)/testpt.o $(COMDIR)/tgaview.o \
$(COMDIR)/zoom.o $(COMDIR)/Halley.o $(COMDIR)/Newton.o \
$(COMDIR)/SoundState.o


ifeq ($(AS),/usr/bin/nasm)

U_OBJS = \
$(UDIR)/calcmand.o $(UDIR)/calmanfp.o $(UDIR)/diskvidu.o $(UDIR)/fpu087.o \
$(UDIR)/general.o $(UDIR)/unix.o \
$(UDIR)/unixscr.o $(UDIR)/video.o \
$(UDIR)/calmanfx.o $(UDIR)/d_x11.o $(UDIR)/os_unix.o

else

U_OBJS = \
$(UDIR)/calcmand.o $(UDIR)/calmanfp.o $(UDIR)/diskvidu.o $(UDIR)/fpu087.o \
$(UDIR)/general.o $(UDIR)/unix.o \
$(UDIR)/unixscr.o $(UDIR)/video.o $(UDIR)/d_x11.o $(UDIR)/os_unix.o

endif

DEPFLAGS = -Y -I.$(HFD) $(DEFINES) -DBIG_ANSI_C -DLINUX

HOBJS = hc.o unix.o

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .cpp .s .h .asm

xfractint: fractint.hlp .WAIT
	if [ -f $(DOSHELPDIR)/helpdefs.h ] ; then mv -f $(DOSHELPDIR)/helpdefs.h $(HFD) ; fi
	cd common ; ${MAKE} all "CFLAGS= -I.${HFD} ${CFLAGS}" "SRCDIR=${SRCDIR}" \
	          "HFD=.${HFD}"
	cd unix ; ${MAKE} all "CFLAGS= -I.${HFD} ${CFLAGS}" "SRCDIR=${SRCDIR}" \
	          "AS=${AS}" "AFLAGS=${AFLAGS}" "HFD=.${HFD}"
	$(CXX) -o xfractint $(CFLAGS) $(OBJS) $(U_OBJS) $(LIBS)
#	strip xfractint

tar:	$(FILES)
	tar cfh xfractint.tar $(FILES)

tidy:
	rm -f $(HOBJS)
	cd common ; ${MAKE} tidy
	cd unix ; ${MAKE} tidy

clean:
	rm -f $(HOBJS) fractint.doc fractint.hlp hc xfractint
	rm -f $(HFD)/helpdefs.h
	cd $(COMDIR) ; ${MAKE} clean
	cd $(UDIR) ; ${MAKE} clean

depend:
	cd $(COMDIR) ; ${MAKE} depend "CFLAGS=${DEPFLAGS}"
	cd $(UDIR) ; ${MAKE} depend "CFLAGS=${DEPFLAGS}"

install: xfractint fractint.hlp
	strip xfractint
# only next 4 lines might need su
	cp xfractint $(BINDIR)/xfractint;
	chmod a+x $(BINDIR)/xfractint;
	cp $(UDIR)/xfractint.man $(MANDIR)/xfractint.1;
	chmod a+r $(MANDIR)/xfractint.1
# create directories if they don't exist
	if [ ! -d $(SRCDIR) ] ; then mkdir $(SRCDIR) ; fi
	if [ ! -d $(SRCDIR)/$(PDIR) ] ; then mkdir $(SRCDIR)/$(PDIR) ; fi
	if [ ! -d $(SRCDIR)/$(FDIR) ] ; then mkdir $(SRCDIR)/$(FDIR) ; fi
	if [ ! -d $(SRCDIR)/$(IDIR) ] ; then mkdir $(SRCDIR)/$(IDIR) ; fi
	if [ ! -d $(SRCDIR)/$(LDIR) ] ; then mkdir $(SRCDIR)/$(LDIR) ; fi
	if [ ! -d $(SRCDIR)/$(MDIR) ] ; then mkdir $(SRCDIR)/$(MDIR) ; fi
	if [ ! -d $(SRCDIR)/$(XDIR) ] ; then mkdir $(SRCDIR)/$(XDIR) ; fi
# copy all the files to the appropriate directories
	cp fractint.hlp sstools.ini $(DOCS) $(SRCDIR)
	cd ./$(PDIR); cp $(PARFILES) $(SRCDIR)/$(PDIR)
	cd ./$(FDIR); cp $(FRMFILES) $(SRCDIR)/$(FDIR)
	cd ./$(IDIR); cp $(IFSFILES) $(SRCDIR)/$(IDIR)
	cd ./$(LDIR); cp $(LFILES) $(SRCDIR)/$(LDIR)
	cd ./$(MDIR); cp $(MAPFILES) $(SRCDIR)/$(MDIR)
	cd ./$(XDIR); cp $(XTRAFILES) $(SRCDIR)/$(XDIR)
# set permissions
	cd $(SRCDIR); cd ..; chmod -R a+rw $(SRCDIR)
	cd $(SRCDIR); chmod a+r fractint.hlp
	cd $(SRCDIR); chmod a+rw sstools.ini
	cd $(SRCDIR)/$(PDIR); chmod a+rw $(PARFILES)
	cd $(SRCDIR)/$(FDIR); chmod a+rw $(FRMFILES)
	cd $(SRCDIR)/$(IDIR); chmod a+rw $(IFSFILES)
	cd $(SRCDIR)/$(LDIR); chmod a+rw $(LFILES)
	cd $(SRCDIR)/$(MDIR); chmod a+rw $(MAPFILES)
	cd $(SRCDIR)/$(XDIR); chmod a+rw $(XTRAFILES)

uninstall:
	cd $(SRCDIR)/$(PDIR); rm -f $(PARFILES)
	cd $(SRCDIR)/$(FDIR); rm -f $(FRMFILES)
	cd $(SRCDIR)/$(IDIR); rm -f $(IFSFILES)
	cd $(SRCDIR)/$(LDIR); rm -f $(LFILES)
	cd $(SRCDIR)/$(MDIR); rm -f $(MAPFILES)
	cd $(SRCDIR)/$(XDIR); rm -f $(XTRAFILES)
	cd $(SRCDIR); rm -f fractint.hlp sstools.ini $(DOCS)
	cd $(SRCDIR); rmdir $(PDIR) $(FDIR) $(IDIR) $(LDIR) $(MDIR) $(XDIR)
	cd $(SRCDIR); cd ..; rmdir $(SRCDIR)
# only next might need su
	rm -f $(BINDIR)/xfractint $(MANDIR)/xfractint.1

fractint.hlp: hc $(DOSHELPDIR)/$(HELP)
	cd $(DOSHELPDIR); ../hc /c; mv fractint.hlp ..

.WAIT:

fractint.doc: doc

doc: hc $(DOSHELPDIR)/$(HELPFILES)
	./hc /p

hc:	$(HOBJS)
	$(CXX) -o hc $(CFLAGS) $(HOBJS)

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

hc.o: $(DOSHELPDIR)/hc.cpp
	$(CXX) $(CFLAGS) -c $<

unix.o: $(UDIR)/unix.cpp
	$(CXX) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c $<

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.
