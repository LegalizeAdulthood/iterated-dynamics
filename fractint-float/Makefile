
# SRCDIR should be a path to the directory that will hold fractint.hlp
# You will have to copy fractint.hlp to SRCDIR and make it world readable.
# SRCDIR should also hold the .par, .frm, etc. files
SRCDIR = .
# BINDIR is where you put your X11 binaries
BINDIR = /usr/X11R6/bin
# MANDIR is where you put your chapter 1 man pages
MANDIR = /usr/X11R6/man/man1

HFD = ./headers
UDIR = ./unix
COMDIR = ./common

FDIR = ./formulas
IDIR = ./ifs
LDIR = ./lsystem
MDIR = ./maps
PDIR = ./pars

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

#CFLAGS = -I. -D_CONST $(DEFINES)
CFLAGS = -I$(HFD) $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -Wall \
         -mcpu=pentium -DNASM -fno-builtin
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -DNASM
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -fno-builtin

# Gcc is often the only compiler that works for this
# For HPUX, use CC = cc -Aa -D_HPUX_SOURCE
# For AIX, maybe use CC = xlc, but other AIX users found cc works, xlc doesn't.
# For Apollo use CC = cc -A cpu,mathlib_sr10  -A systype,bsd4.3
# For Sun Solaris 2.x w/SparcCompilerC (cc), use CC = cc.
# For Sun Solaris 2.x w/GNU gcc, use CC = gcc
#CC = gcc
CC = gcc

# For HPUX, use LIBS = -L/usr/lib/X11R4 -lX11 -lm -lcurses -ltermcap
# For AIX or OSF/1, add -lbsd
# For 386BSD, add -L/usr/X386/lib to LIBS
# For Apollo, change -lX11 to -L/usr/X11/libX11
# For Solaris, add -L/usr/openwin/lib; change -lncurses to -lcurses
# if you get undefined symbols like "w32addch".
# For Linux, use
#LIBS = -L/usr/X11R6/lib -lX11 -lm -lncurses
LIBS = -L/usr/X11R6/lib -lX11 -lm -lncurses
#LIBS = -lX11 -lm -lcurses

# For using nasm, set:
AS = nasm
# Note that because of the differences between the assembler syntaxes,
#  nasm is the only one that will work.
#AS = foo

# Below is for Linux with output file type of elf, turn all warnings on
AFLAGS = -f elf -w+orphan-labels

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
frasetup.c gifview.c hc.c hcmplx.c help.c \
intro.c jb.c jiim.c line3d.c loadfdos.c loadfile.c loadmap.c lorenz.c \
lsys.c lsysf.c memory.c miscfrac.c miscovl.c miscres.c \
mpmath_c.c parser.c parserfp.c plot3d.c printer.c prompts1.c \
prompts2.c realdos.c rotate.c slideshw.c soi.c soi1.c stereo.c \
targa.c testpt.c tgaview.c tplus.c zoom.c

NEWSRC = calcmand.c calmanfp.c diskvidu.c \
fpu087.c fracsuba.c general.c tplus_a.c \
video.c unix.c unixscr.c unix.h Makefile versions \
calmanfx.asm

HEADERS = big.h biginit.h cmplx.h externs.h fmath.h fractint.h fractype.h \
helpcom.h lsys.h mpmath.h port.h prototyp.h targa.h targa_lc.h tplus.h

DOCS = debugfla.doc fractsrc.doc hc.doc

HELPFILES = help.src help2.src help3.src help4.src help5.src

SRCFILES = $(COMDIR)/$(OLDSRC) $(UDIR)/$(NEWSRC) $(HELPFILES) \
$(HFD)/$(HEADERS) $(DOCS)

PARFILES = \
cellular.par demo.par fract18.par fract19.par fract200.par fractint.par \
icons.par lyapunov.par music.par orbits.par phoenix.par

FRMFILES = fractint.frm fract200.frm fract196.frm

IFSFILES = fractint.ifs

LFILES = fractint.l penrose.l tiling.l

MAPFILES = \
Carlson1.map Digiorg1.map Digiorg2.map Gallet01.map Gallet02.map Gallet03.map \
Gallet04.map Gallet05.map Gallet06.map Gallet07.map Gallet08.map Gallet09.map \
Gallet10.map Gallet11.map Gallet12.map Gallet13.map Gallet14.map Gallet15.map \
Gallet16.map Gallet17.map Gallet18.map Lindaa01.map Lindaa02.map Lindaa03.map \
Lindaa04.map Lindaa05.map Lindaa06.map Lindaa07.map Lindaa08.map Lindaa09.map \
Lindaa10.map Lindaa11.map Lindaa12.map Lindaa14.map Lindaa15.map Lindaa16.map \
Lindaa17.map Morgan1.map Morgan2.map Morgan3.map Morgen3.map Skydye01.map \
Skydye02.map Skydye03.map Skydye04.map Skydye05.map Skydye06.map Skydye07.map \
Skydye08.map Skydye09.map Skydye10.map Skydye11.map Skydye12.map Wizzl011.map \
Wizzl012.map Wizzl013.map Wizzl014.map Wizzl015.map Wizzl016.map Wizzl017.map \
Wizzl018.map Wizzl019.map Wizzl020.map altern.map blues.map bud2.map bud3.map \
bud4.map bud5.map bud6.map bud7.map chroma.map damien1.map damien2.map  \
damien3.map damien4.map damien5.map default.map droz10.map droz11.map  \
droz12.map droz13.map droz14.map droz15.map droz21.map droz22.map droz23.map \
droz28.map droz31.map droz33.map droz34.map droz35.map droz36.map droz38.map \
droz39.map droz40.map droz44.map droz46.map droz49.map droz52.map droz54.map \
droz56.map droz60.map droz62.map droz8.map drozdis1.map firestrm.map \
froth3.map froth316.map froth6.map froth616.map gamma1.map gamma2.map \
glasses1.map glasses2.map goodega.map green.map grey.map grid.map headache.map \
landscap.map lkmtch00.map lkmtch01.map lkmtch02.map lkmtch03.map lkmtch04.map \
lkmtch05.map lkmtch06.map lkmtch07.map lkmtch08.map lkmtch09.map lkmtch10.map \
lkmtch11.map lkmtch12.map lkmtch13.map lkmtch14.map lkmtch15.map lkmtch16.map \
lkmtch17.map lkmtch18.map lkmtch19.map lyapunov.map neon.map paintjet.map \
royal.map topo.map volcano.map

OLDRUN = $(PDIR)/$(PARFILES) $(FDIR)/$(FRMFILES) $(IDIR)/$(IFSFILES) \
$(LDIR)/$(LFILES) $(MDIR)/$(MAPFILES) demo.key

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
$(COMDIR)/gifview.o $(COMDIR)/hcmplx.o $(COMDIR)/help.o \
$(COMDIR)/intro.o $(COMDIR)/jb.o $(COMDIR)/jiim.o $(COMDIR)/line3d.o \
$(COMDIR)/loadfdos.o $(COMDIR)/loadfile.o $(COMDIR)/loadmap.o \
$(COMDIR)/lorenz.o $(COMDIR)/lsys.o $(COMDIR)/lsysf.o $(COMDIR)/memory.o \
$(COMDIR)/miscfrac.o $(COMDIR)/miscovl.o $(COMDIR)/miscres.o \
$(COMDIR)/mpmath_c.o $(COMDIR)/parser.o $(COMDIR)/parserfp.o \
$(COMDIR)/plot3d.o $(COMDIR)/printer.o $(COMDIR)/prompts1.o \
$(COMDIR)/prompts2.o $(COMDIR)/realdos.o $(COMDIR)/rotate.o \
$(COMDIR)/slideshw.o $(COMDIR)/soi.o $(COMDIR)/soi1.o $(COMDIR)/stereo.o \
$(COMDIR)/targa.o $(COMDIR)/testpt.o $(COMDIR)/tgaview.o \
$(COMDIR)/tplus.o $(COMDIR)/zoom.o


ifeq ($(AS),nasm)

U_OBJS = \
$(UDIR)/calcmand.o $(UDIR)/calmanfp.o $(UDIR)/diskvidu.o $(UDIR)/fpu087.o \
$(UDIR)/fracsuba.o $(UDIR)/general.o $(UDIR)/tplus_a.o $(UDIR)/unix.o \
$(UDIR)/unixscr.o $(UDIR)/video.o \
$(UDIR)/calmanfx.o

else

U_OBJS = \
$(UDIR)/calcmand.o $(UDIR)/calmanfp.o $(UDIR)/diskvidu.o $(UDIR)/fpu087.o \
$(UDIR)/fracsuba.o $(UDIR)/general.o $(UDIR)/tplus_a.o $(UDIR)/unix.o \
$(UDIR)/unixscr.o $(UDIR)/video.o

endif

HOBJS = $(COMDIR)/hc.o unix.o

HELP = help.src help2.src help3.src help4.src help5.src

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

xfractint: fractint.hlp .WAIT
	mv -f helpdefs.h $(HFD)
	cd common ; ${MAKE} all "CFLAGS= -I.${HFD} ${CFLAGS}" "SRCDIR=${SRCDIR}" \
	          "HFD=.${HFD}"
	cd unix ; ${MAKE} all "CFLAGS= -I.${HFD} ${CFLAGS}" "SRCDIR=${SRCDIR}" \
	          "AS=${AS}" "AFLAGS=${AFLAGS}" "HFD=.${HFD}"
	$(CC) -o xfractint $(CFLAGS) $(OBJS) $(U_OBJS) $(LIBS)
#	strip xfractint

tar:	$(FILES)
	tar cfh xfractint.tar $(FILES)

tidy:
	rm -f $(HOBJS)
	cd common ; ${MAKE} tidy
	cd unix ; ${MAKE} tidy

clean:
	rm -f $(HOBJS) fractint.doc fractint.hlp hc xfractint
	rm -f ./headers/helpdefs.h
	cd common ; ${MAKE} clean
	cd unix ; ${MAKE} clean

install: xfractint fractint.hlp
	cp xfractint $(BINDIR)/xfractint
	strip $(BINDIR)/xfractint
	chmod a+x $(BINDIR)/xfractint
	cp fractint.hlp $(PDIR)/$(PARFILES) $(FDIR)/$(FRMFILES) \
	   $(IDIR)/$(IFSFILES) $(LDIR)/$(LFILES) $(MDIR)/$(MAPFILES) $(SRCDIR)
	(cd $(SRCDIR); chmod a+r fractint.hlp $(PDIR)/$(PARFILES) \
	   $(FDIR)/$(FRMFILES) $(IDIR)/$(IFSFILES) $(LDIR)/$(LFILES) \
	   $(MDIR)/$(MAPFILES) )
	cp $(UDIR)/xfractint.man $(MANDIR)/xfractint.1
	chmod a+r $(MANDIR)/xfractint.1

fractint.hlp: hc $(HELP)
	./hc /c

.WAIT:

fractint.doc: doc

doc: hc $(HELP)
	./hc /p

hc:	$(HOBJS)
	$(CC) -o hc $(CFLAGS) $(HOBJS)

unix.o: $(UDIR)/unix.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c $(UDIR)/unix.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.

hc.o: $(COMDIR)/hc.c $(HFD)/helpcom.h $(HFD)/port.h

