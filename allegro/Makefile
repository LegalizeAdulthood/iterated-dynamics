
# SRCDIR should be a path to the directory that will hold fractint.hlp
# You will have to copy fractint.hlp to SRCDIR and make it world readable.
# SRCDIR should also hold the .par, .frm, etc. files
SRCDIR = .
# BINDIR is where you put your X11 binaries
BINDIR = /usr/X11R6/bin
# MANDIR is where you put your chapter 1 man pages
MANDIR = /usr/X11R6/man/man1

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
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -Wall -mcpu=pentium -DNASM -fno-builtin
CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -Wall -mcpu=pentium -fno-builtin
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -DNASM
#CFLAGS = -I. $(DEFINES) -g -DBIG_ANSI_C -DLINUX -Os -fno-builtin

# Gcc is often the only compiler that works for this
# For HPUX, use CC = cc -Aa -D_HPUX_SOURCE
# For AIX, maybe use CC = xlc, but other AIX users found cc works, xlc doesn't.
# For Apollo use CC = cc -A cpu,mathlib_sr10  -A systype,bsd4.3 
# For Sun Solaris 2.x w/SparcCompilerC (cc), use CC = cc.
# For Sun Solaris 2.x w/GNU gcc, use CC = gcc
CC = gcc
#CC = cc

# For HPUX, use LIBS = -L/usr/lib/X11R4 -lX11 -lm -lcurses -ltermcap
# For AIX or OSF/1, add -lbsd
# For 386BSD, add -L/usr/X386/lib to LIBS
# For Apollo, change -lX11 to -L/usr/X11/libX11
# For Solaris, add -L/usr/openwin/lib; change -lncurses to -lcurses
# if you get undefined symbols like "w32addch".
# For Linux, use  
#LIBS = -L/usr/X11R6/lib -lX11 -lm -lcurses
LIBS = -L/usr/X11R6/lib -lX11 -lm -lncurses  `/usr/bin/allegro-config --libs`
#LIBS = -lX11 -lm -lcurses

# For using nasm, set:
#AS = nasm
# Note that because of the differences between the assembler syntaxes,
#  nasm is the only one that will work.
AS = none

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
3d.c ant.c big.h bigflt.c biginit.c biginit.h bignum.c \
bignumc.c calcfrac.c cmdfiles.c cmplx.h debugfla.doc decoder.c editpal.c \
encoder.c evolve.c externs.h fracsubr.c fractalb.c fractalp.c \
fractals.c fractint.c fractint.h fractsrc.doc fractype.h framain2.c \
frasetup.c gifview.c hc.c hc.doc hcmplx.c help.c helpcom.h \
intro.c jb.c jiim.c line3d.c loadfdos.c loadfile.c loadmap.c lorenz.c \
lsys.c lsys.h lsysf.c memory.c miscfrac.c miscovl.c miscres.c mpmath.h \
mpmath_c.c parser.c parserfp.c plot3d.c port.h prompts1.c \
prompts2.c prototyp.h realdos.c rotate.c slideshw.c soi.c soi1.c stereo.c \
testpt.c zoom.c drivers.h drivers.c d_x11.c d_disk.c d_allegro.c

NEWSRC = calcmand.c calmanfp.c diskvidu.c fpu087.c fracsuba.c general.c \
video.c unix.c unix.h Makefile versions calmanfx.asm

HELPFILES = help.src help2.src help3.src help4.src help5.src

SRCFILES = $(OLDSRC) $(NEWSRC) $(HELPFILES)

PARFILES = \
cellular.par demo.par fract18.par fract19.par fract200.par fractint.par \
icons.par lyapunov.par music.par phoenix.par

FRMFILES = fractint.frm fract200.frm

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

OLDRUN = $(PARFILES) $(FRMFILES) $(IFSFILES) $(LFILES) \
$(MAPFILES) demo.key

NEWRUN = fractint.doc README xfractint.man

NEWFILES = $(NEWSRC) $(NEWRUN)

RUNFILES = $(OLDRUN) $(NEWRUN)

FILES = $(SRCFILES) $(RUNFILES)

ifeq ($(AS),nasm)

OBJS = \
3d.o ant.o bigflt.o  biginit.o bignum.o bignumc.o calcfrac.o \
calmanfp.o cmdfiles.o decoder.o diskvidu.o editpal.o encoder.o \
evolve.o fpu087.o fracsubr.o fractalb.o fractalp.o fractals.o \
fractint.o framain2.o frasetup.o general.o gifview.o hcmplx.o help.o \
intro.o jb.o jiim.o line3d.o loadfdos.o loadfile.o loadmap.o lorenz.o \
lsys.o lsysf.o memory.o miscfrac.o miscovl.o miscres.o mpmath_c.o parser.o \
parserfp.o plot3d.o prompts1.o prompts2.o \
realdos.o rotate.o slideshw.o soi.o soi1.o stereo.o testpt.o \
unix.o video.o zoom.o drivers.o d_x11.o d_disk.o \
calmanfx.o d_allegro.o

else

OBJS = \
3d.o ant.o bigflt.o  biginit.o bignum.o bignumc.o calcfrac.o \
calmanfp.o cmdfiles.o decoder.o diskvidu.o editpal.o encoder.o \
evolve.o fpu087.o fracsubr.o fractalb.o fractalp.o fractals.o \
fractint.o framain2.o frasetup.o general.o gifview.o hcmplx.o help.o \
intro.o jb.o jiim.o line3d.o loadfdos.o loadfile.o loadmap.o lorenz.o \
lsys.o lsysf.o memory.o miscfrac.o miscovl.o miscres.o mpmath_c.o parser.o \
parserfp.o plot3d.o prompts1.o prompts2.o \
realdos.o rotate.o slideshw.o soi.o soi1.o stereo.o testpt.o \
unix.o video.o zoom.o drivers.o d_x11.o d_disk.o d_allegro.o

endif

HOBJS = hc.o unix.o

HELP = help.src help2.src help3.src help4.src help5.src

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

xfractint: fractint.hlp .WAIT $(OBJS)
	$(CC) -o xfractint $(CFLAGS) $(OBJS) $(LIBS)
#	strip xfractint

tar:	$(FILES)
	tar cfh xfractint.tar $(FILES)

tidy:
	rm -f $(OBJS) $(HOBJS)

clean:
	rm -f $(OBJS) $(HOBJS) fractint.doc fractint.hlp hc xfractint helpdefs.h

install: xfractint fractint.hlp
	cp xfractint $(BINDIR)/xfractint
	strip $(BINDIR)/xfractint
	chmod a+x $(BINDIR)/xfractint
	cp fractint.hlp $(PARFILES) $(FRMFILES) $(IFSFILES) $(LFILES) ./maps/$(MAPFILES) $(SRCDIR)
	(cd $(SRCDIR); chmod a+r fractint.hlp $(PARFILES) $(FRMFILES) $(IFSFILES) $(LFILES) $(MAPFILES) )
	cp xfractint.man $(MANDIR)/xfractint.1
	chmod a+r $(MANDIR)/xfractint.1

fractint.hlp: hc $(HELP)
	./hc /c

.WAIT:

fractint.doc: doc

doc: hc $(HELP)
	./hc /p

hc:	$(HOBJS)
	$(CC) -o hc $(CFLAGS) $(HOBJS)

unix.o:	unix.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c unix.c

fractint.o: fractint.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c fractint.c

help.o: help.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c help.c

ifeq ($(AS),nasm)
calmanfx.o: calmanfx.asm
	$(AS) $(AFLAGS) calmanfx.asm
endif

sharsrc:	$(SRCFILES)
	shar -s shirriff@eng.sun.com $(SRCFILES) > xfsrc.shar

sharrun:	$(RUNFILES)
	shar $(RUNFILES) > xfrun.shar

shar:	$(FILES)
	shar $(FILES) > xfract.shar

sharc:	$(FILES)
	shar $(FILES) > xfract.shr; gzip xfract.shr

shartw: $(OLDSRC) $(HELPFILES)
	shar -s shirriff@sprite.Berkeley.EDU $(OLDSRC) $(HELPFILES)> source.tw;compress \
	source.tw

create:
	sccs create -r3.0 $(FILES)

checkout:
	for i in $(FILES) ; do \
	    sccs edit $$i; \
	done

checkin: $(OLDRUN) $(OLDSRC)
	for i in $(FILES) ; do \
	    sccs delget  -y"released 3.02" $$i; \
	done

unlock: $(FILES)
	co -u -f $(FILES)

outdate: $(FILES)
	rcs -o3.5 $(FILES)

merge: $(SRCFILES) $(HELPFILES)
	for i in $(SRCFILES) ; do \
	    rcsmerge -r3.1 -r3.1.1.1 $$i; \
	done

tags: $(SRCFILES)
	ctags $(SRCFILES)

rcsdiff:
	cp /dev/null diffs.out ; \
	for i in $(OLDSRC) ; do \
	    rcsdiff -c $$i >> diffs.out; \
	done
diff:
	echo "use make -i"; \
	for i in $(FILES) ; do \
	    (diff -c $$i v18 > diffs/$$i; true;); \
	    true; \
	done

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.

3d.o: 3d.c fractint.h port.h prototyp.h mpmath.h helpcom.h

ant.o: ant.c fractint.h port.h prototyp.h mpmath.h helpcom.h helpdefs.h

bigflt.o: bigflt.c big.h biginit.h

biginit.o: biginit.c fractint.h port.h prototyp.h mpmath.h helpcom.h \
big.h 

bignum.o: bignum.c fractint.h port.h prototyp.h mpmath.h helpcom.h 

bignumc.o: bignumc.c fractint.h port.h prototyp.h mpmath.h helpcom.h 

calcfrac.o: calcfrac.c fractint.h port.h fractype.h mpmath.h \
			prototyp.h helpcom.h

calmanfp.o: calmanfp.c fractint.h port.h fractype.h

ifeq ($(AS),nasm)
calmanfx.o: calmanfx.asm xfract_a.inc
endif

cmdfiles.o: cmdfiles.c fractint.h port.h fractype.h prototyp.h mpmath.h \
			helpcom.h drivers.h

decoder.o: decoder.c prototyp.h mpmath.h port.h fractint.h helpcom.h

diskvidu.o: diskvidu.c fractint.h port.h drivers.h

editpal.o: editpal.c fractint.h port.h prototyp.h mpmath.h helpcom.h

encoder.o: encoder.c fractint.h port.h fractype.h prototyp.h mpmath.h \
        helpcom.h drivers.h

fpu087.o: fpu087.c fractint.h port.h mpmath.h

fracsubr.o: fracsubr.c fractint.h port.h fractype.h mpmath.h prototyp.h \
			helpcom.h

fractalb.o: fractalb.c fractint.h port.h prototyp.h mpmath.h helpcom.h \
helpdefs.h fractype.h

fractalp.o: fractalp.c fractint.h port.h mpmath.h helpdefs.h fractype.h \
			prototyp.h helpcom.h

fractals.o: fractals.c fractint.h port.h mpmath.h helpdefs.h fractype.h \
			prototyp.h helpcom.h

fractint.o: fractint.c prototyp.h mpmath.h port.h fractint.h helpcom.h \
			fractype.h helpdefs.h drivers.h

framain2.o: framain2.c fractint.h port.h prototyp.h mpmath.h helpcom.h \
         fractype.h helpdefs.h drivers.h

frasetup.o: frasetup.c fractint.h port.h prototyp.h mpmath.h helpcom.h \
mpmath.h helpdefs.h fractype.h

general.o: general.c fractint.h port.h drivers.h

gifview.o: gifview.c fractint.h port.h prototyp.h mpmath.h helpcom.h

hc.o: hc.c helpcom.h port.h

hcmplx.o: hcmplx.c fractint.h port.h prototyp.h mpmath.h helpcom.h mpmath.h

help.o: help.c fractint.h port.h helpcom.h helpdefs.h prototyp.h mpmath.h \
        drivers.h

intro.o: intro.c fractint.h port.h helpdefs.h prototyp.h mpmath.h helpcom.h \
        drivers.h

jb.o: jb.c fractint.h port.h mpmath.h helpdefs.h prototyp.h helpcom.h

jiim.o: jiim.c helpdefs.h fractint.h port.h fractype.h prototyp.h mpmath.h \
		helpcom.h

line3d.o: line3d.c fractint.h port.h prototyp.h mpmath.h helpcom.h

loadfdos.o: loadfdos.c fractint.h port.h helpdefs.h prototyp.h mpmath.h \
			helpcom.h

loadfile.o: loadfile.c fractint.h port.h fractype.h prototyp.h \
			mpmath.h helpcom.h

loadmap.o: loadmap.c fractint.h port.h prototyp.h mpmath.h helpcom.h

lorenz.o: lorenz.c mpmath.h fractint.h port.h fractype.h prototyp.h helpcom.h \
         drivers.h

lsys.o: lsys.c fractint.h port.h prototyp.h mpmath.h helpcom.h

lsysf.o: lsysf.c fractint.h port.h prototyp.h mpmath.h helpcom.h lsys.h

miscfrac.o: miscfrac.c fractint.h port.h prototyp.h mpmath.h helpcom.h \
fractype.h

miscovl.o: miscovl.c fractint.h port.h fractype.h helpdefs.h prototyp.h \
			mpmath.h helpcom.h

miscres.o: miscres.c fractint.h port.h fractype.h helpdefs.h prototyp.h \
			mpmath.h helpcom.h drivers.h

mpmath_c.o: mpmath_c.c mpmath.h prototyp.h port.h fractint.h helpcom.h

parser.o: parser.c mpmath.h prototyp.h port.h fractint.h helpcom.h

parserfp.o: parserfp.c fractint.h port.h prototyp.h mpmath.h helpcom.h

plot3d.o: plot3d.c fractint.h port.h fractype.h prototyp.h mpmath.h helpcom.h

prompts1.o: prompts1.c fractint.h port.h fractype.h helpdefs.h prototyp.h \
			mpmath.h helpcom.h drivers.h

prompts2.o: prompts2.c fractint.h port.h fractype.h helpdefs.h prototyp.h \
			mpmath.h helpcom.h drivers.h

realdos.o: realdos.c fractint.h port.h fractype.h helpdefs.h prototyp.h \
			mpmath.h helpcom.h drivers.h

rotate.o: rotate.c fractint.h port.h helpdefs.h prototyp.h mpmath.h helpcom.h

slideshw.o: slideshw.c fractint.h port.h prototyp.h mpmath.h helpcom.h drivers.h

stereo.o: stereo.c fractint.h port.h prototyp.h mpmath.h helpcom.h helpdefs.h

testpt.o: testpt.c

unix.o: unix.c port.h

video.o: video.c fractint.h port.h prototyp.h mpmath.h helpcom.h drivers.h

zoom.o: zoom.c fractint.h port.h prototyp.h mpmath.h helpcom.h

#----------------------------------------------------------------------
d_disk.o: d_disk.c fractint.h port.h prototyp.h helpcom.h helpdefs.h \
        drivers.h
d_x11.o: d_x11.c fractint.h port.h prototyp.h helpcom.h helpdefs.h \
        drivers.h
drivers.o: drivers.c fractint.h port.h drivers.h
d_allegro.o: d_allegro.c fractint.h port.h prototyp.h helpcom.h helpdefs.h \
        drivers.h

