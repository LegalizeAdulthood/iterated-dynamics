SHELL=/bin/sh

SRCS = \
    3d.cpp \
    ant.cpp \
    bigflt.cpp \
    biginit.cpp \
    bignum.cpp \
    bignumc.cpp \
    calcfrac.cpp \
    cmdfiles.cpp \
    decoder.cpp \
    diskvid.cpp \
    drivers.cpp \
    editpal.cpp \
    encoder.cpp \
    evolve.cpp \
    font8x8.cpp \
	fracsuba.cpp \
    fracsubr.cpp \
    fractalb.cpp \
    fractalp.cpp \
    fractals.cpp \
    fractint.cpp \
    framain2.cpp \
    frasetup.cpp \
    gifview.cpp \
    hcmplx.cpp \
    help.cpp \
    history.cpp \
    intro.cpp \
    jb.cpp \
    jiim.cpp \
    line3d.cpp \
    loadfdos.cpp \
    loadfile.cpp \
    loadmap.cpp \
    lorenz.cpp \
    lsys.cpp \
    lsysf.cpp \
    memory.cpp \
    miscfrac.cpp \
    miscovl.cpp \
    miscres.cpp \
    mpmath_c.cpp \
    parser.cpp \
    parserfp.cpp \
    plot3d.cpp \
    prompts1.cpp \
    prompts2.cpp \
    realdos.cpp \
    rotate.cpp \
    slideshw.cpp \
    soi.cpp \
    soi1.cpp \
    stereo.cpp \
    testpt.cpp \
    zoom.cpp \
    Halley.cpp \
    Newton.cpp \
	SoundState.cpp \
	EscapeTime.cpp \
	ThreeDimensionalState.cpp \
	MathUtil.cpp \
	UIChoices.cpp \
	SolidGuess.cpp \
	Tesseral.cpp \
	BoundaryTrace.cpp \
	DiffusionScan.cpp \
	WorkList.cpp \
	filesystem.cpp \
	Browse.cpp

OBJS = $(SRCS:.cpp=.o)

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .s .h .asm .cpp

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

depend:
	gccmakedep -- $(CFLAGS) -- $(SRCS)
#	makedepend $(CFLAGS) $(SRCS)

fractint.o: fractint.cpp
	$(CXX) -c $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" $<

help.o: help.cpp
	$(CXX) -c $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" $<

copy: $(FILES)
	mv $(FILES) backup

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

# DO NOT DELETE
3d.o: 3d.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/3d.h
ant.o: ant.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/ant.h ../headers/drivers.h \
  ../headers/fihelp.h ../headers/fracsubr.h ../headers/realdos.h
bigflt.o: bigflt.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h
biginit.o: biginit.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/biginit.h ../headers/calcfrac.h \
  ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/fractalp.h ../headers/prompts2.h \
  ../headers/realdos.h
bignum.o: bignum.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/big.h
bignumc.o: bignumc.cpp /usr/include/memory.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h
calcfrac.o: calcfrac.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/calcfrac.h ../headers/calcmand.h \
  ../headers/drivers.h ../headers/filesystem.h ../headers/fpu.h \
  ../headers/diskvid.h ../headers/fracsubr.h ../headers/fractalp.h \
  ../headers/fractals.h ../headers/line3d.h ../headers/lorenz.h \
  ../headers/miscfrac.h ../headers/miscres.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/soi.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/MathUtil.h ../headers/Formula.h \
  ../headers/WorkList.h ../headers/SolidGuess.h \
  ../headers/BoundaryTrace.h ../headers/Tesseral.h \
  ../headers/DiffusionScan.h
cmdfiles.o: cmdfiles.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h \
  ../headers/unix.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/fractype.h ../headers/Browse.h \
  ../headers/cmdfiles.h ../headers/drivers.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/fracsuba.h ../headers/fracsubr.h \
  ../headers/fractalb.h ../headers/fractals.h ../headers/history.h \
  ../headers/loadfile.h ../headers/loadmap.h ../headers/miscovl.h \
  ../headers/miscres.h ../headers/prompts1.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/ThreeDimensionalState.h \
  ../headers/CommandParser.h
decoder.o: decoder.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/decoder.h ../headers/gifview.h
diskvid.o: diskvid.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/externs.h ../headers/drivers.h ../headers/diskvid.h \
  ../headers/realdos.h
drivers.o: drivers.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/externs.h ../headers/cmplx.h ../headers/fractint.h \
  ../headers/drivers.h
editpal.o: editpal.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/editpal.h \
  ../headers/fihelp.h ../headers/filesystem.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/rotate.h
encoder.o: encoder.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h ../headers/diskvid.h \
  ../headers/encoder.h ../headers/filesystem.h ../headers/loadfile.h \
  ../headers/miscres.h ../headers/prompts2.h ../headers/realdos.h \
  ../headers/busy.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/ThreeDimensionalState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h ../headers/Formula.h
evolve.o: evolve.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/include/ctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/calcfrac.h \
  ../headers/evolve.h ../headers/fihelp.h ../headers/fractalp.h \
  ../headers/history.h ../headers/miscres.h ../headers/prompts1.h \
  ../headers/realdos.h ../headers/zoom.h ../headers/Formula.h \
  ../headers/UIChoices.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/vector \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_vector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_bvector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/vector.tcc \
  ../headers/MathUtil.h
font8x8.o: font8x8.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
fracsuba.o: fracsuba.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fracsuba.h
fracsubr.o: fracsubr.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/memory.h \
  /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/calcfrac.h ../headers/drivers.h \
  ../headers/fihelp.h ../headers/fracsubr.h ../headers/fractalb.h \
  ../headers/fractalp.h ../headers/miscovl.h ../headers/miscres.h \
  ../headers/realdos.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/WorkList.h
fractalb.o: fractalb.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/fractalb.h \
  ../headers/fractalp.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h
fractalp.o: fractalp.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/ant.h \
  ../headers/Browse.h ../headers/calcfrac.h ../headers/fractalb.h \
  ../headers/fractalp.h ../headers/fractals.h ../headers/frasetup.h \
  ../headers/jb.h ../headers/lorenz.h ../headers/lsys.h \
  ../headers/miscfrac.h ../headers/prompts1.h ../headers/Halley.h \
  ../headers/Newton.h ../headers/Formula.h
fractals.o: fractals.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/externs.h \
  ../headers/calcfrac.h ../headers/fpu.h ../headers/fracsubr.h \
  ../headers/fractals.h ../headers/hcmplx.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/MathUtil.h ../headers/Formula.h
fractint.o: fractint.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/signal.h \
  /usr/include/bits/sigset.h /usr/include/bits/signum.h \
  /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
  /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
  /usr/include/asm-i486/sigcontext.h /usr/include/linux/compiler.h \
  /usr/include/bits/sigstack.h /usr/include/sys/ucontext.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/bits/sigthread.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h /usr/include/ctype.h \
  /usr/include/endian.h /usr/include/bits/endian.h ../headers/port.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/Browse.h \
  ../headers/cmdfiles.h ../headers/decoder.h ../headers/drivers.h \
  ../headers/encoder.h ../headers/evolve.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/fracsubr.h ../headers/framain2.h \
  ../headers/history.h ../headers/intro.h ../headers/loadfile.h \
  ../headers/miscovl.h ../headers/prompts1.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/CommandParser.h
framain2.o: framain2.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h /usr/include/ctype.h \
  /usr/include/endian.h /usr/include/bits/endian.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/sys/types.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/fractype.h ../headers/helpdefs.h \
  ../headers/ant.h ../headers/Browse.h ../headers/calcfrac.h \
  ../headers/diskvid.h ../headers/drivers.h ../headers/editpal.h \
  ../headers/encoder.h ../headers/evolve.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/fracsubr.h ../headers/fractals.h \
  ../headers/framain2.h ../headers/gifview.h ../headers/history.h \
  ../headers/jiim.h ../headers/line3d.h ../headers/loadfile.h \
  ../headers/lorenz.h ../headers/miscovl.h ../headers/miscfrac.h \
  ../headers/miscres.h ../headers/prompts1.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/rotate.h ../headers/stereo.h \
  ../headers/zoom.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/SoundState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h \
  ../headers/CommandParser.h
frasetup.o: frasetup.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/calcfrac.h \
  ../headers/fracsubr.h ../headers/fractals.h ../headers/frasetup.h \
  ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h
gifview.o: gifview.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/decoder.h ../headers/diskvid.h \
  ../headers/filesystem.h ../headers/fracsubr.h ../headers/framain2.h \
  ../headers/gifview.h ../headers/loadmap.h ../headers/prompts2.h \
  ../headers/stereo.h ../headers/busy.h ../headers/SoundState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h \
  ../headers/ThreeDimensionalState.h
hcmplx.o: hcmplx.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/hcmplx.h
help.o: help.cpp /usr/include/fcntl.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/fcntl.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/time.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/string.h \
  /usr/include/xlocale.h /usr/include/assert.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/miscres.h ../headers/prompts1.h \
  ../headers/prompts2.h ../headers/realdos.h
history.o: history.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/history.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/ThreeDimensionalState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h ../headers/Formula.h
intro.o: intro.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/xlocale.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/fihelp.h \
  ../headers/intro.h ../headers/realdos.h
jb.o: jb.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/drivers.h \
  ../headers/fracsubr.h ../headers/fractals.h ../headers/jb.h \
  ../headers/loadmap.h ../headers/realdos.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h
jiim.o: jiim.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/calcfrac.h \
  ../headers/diskvid.h ../headers/drivers.h ../headers/editpal.h \
  ../headers/fihelp.h ../headers/filesystem.h ../headers/jiim.h \
  ../headers/lorenz.h ../headers/miscfrac.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h
line3d.o: line3d.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cmath \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cmath.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h \
  ../headers/unix.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/3d.h ../headers/diskvid.h \
  ../headers/drivers.h ../headers/filesystem.h ../headers/line3d.h \
  ../headers/miscres.h ../headers/prompts2.h ../headers/realdos.h \
  ../headers/ThreeDimensionalState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/MathUtil.h
loadfdos.o: loadfdos.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fihelp.h ../headers/loadfdos.h \
  ../headers/realdos.h
loadfile.o: loadfile.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/calcfrac.h \
  ../headers/drivers.h ../headers/evolve.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/fracsubr.h ../headers/framain2.h \
  ../headers/loadfdos.h ../headers/loadfile.h ../headers/lorenz.h \
  ../headers/miscres.h ../headers/prompts1.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/zoom.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/ThreeDimensionalState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h ../headers/Formula.h
loadmap.o: loadmap.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/filesystem.h ../headers/loadmap.h ../headers/miscres.h \
  ../headers/prompts2.h ../headers/realdos.h
lorenz.o: lorenz.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/3d.h ../headers/drivers.h \
  ../headers/encoder.h ../headers/fracsubr.h ../headers/jiim.h \
  ../headers/lorenz.h ../headers/miscres.h ../headers/plot3d.h \
  ../headers/realdos.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/ThreeDimensionalState.h
lsys.o: lsys.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/lsys.h ../headers/drivers.h ../headers/fpu.h \
  ../headers/miscres.h ../headers/realdos.h ../headers/MathUtil.h
lsysf.o: lsysf.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/lsys.h ../headers/realdos.h \
  ../headers/MathUtil.h
memory.o: memory.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/malloc.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/errno.h \
  /usr/include/bits/errno.h /usr/include/linux/errno.h \
  /usr/include/asm/errno.h /usr/include/asm-i486/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/filesystem.h ../headers/prompts2.h \
  ../headers/realdos.h
miscfrac.o: miscfrac.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/calcfrac.h ../headers/diskvid.h \
  ../headers/drivers.h ../headers/fpu.h ../headers/fracsubr.h \
  ../headers/fractals.h ../headers/loadmap.h ../headers/miscfrac.h \
  ../headers/miscres.h ../headers/realdos.h ../headers/testpt.h \
  ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/MathUtil.h ../headers/Formula.h
miscovl.o: miscovl.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/sys/types.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/externs.h ../headers/fractype.h ../headers/helpdefs.h \
  ../headers/cmdfiles.h ../headers/drivers.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/fractalp.h ../headers/framain2.h \
  ../headers/loadfile.h ../headers/miscovl.h ../headers/miscres.h \
  ../headers/prompts1.h ../headers/prompts2.h ../headers/realdos.h \
  ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  ../headers/SoundState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/ThreeDimensionalState.h \
  ../headers/Formula.h
miscres.o: miscres.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/malloc.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/sys/types.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h ../headers/filesystem.h ../headers/fracsubr.h \
  ../headers/fractalp.h ../headers/miscres.h ../headers/prompts1.h \
  ../headers/prompts2.h ../headers/realdos.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/MathUtil.h ../headers/Formula.h \
  ../headers/WorkList.h ../headers/DiffusionScan.h
mpmath_c.o: mpmath_c.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fpu.h ../headers/jiim.h
parser.o: parser.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/include/ctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/sstream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/istream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ios \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/localefwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/ios_base.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_classes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/streambuf \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/streambuf.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_ios.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/streambuf_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_facets.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwctype \
  /usr/include/wctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/ctype_base.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/ctype_inline.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/codecvt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/time_members.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/messages_members.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_ios.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/istream.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/locale \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_facets.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/typeinfo \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ostream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/ostream.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/sstream.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/fpu.h ../headers/fractals.h \
  ../headers/jiim.h ../headers/miscres.h ../headers/prompts2.h \
  ../headers/realdos.h ../headers/Formula.h ../headers/MathUtil.h
parserfp.o: parserfp.cpp /usr/include/ctype.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/realdos.h ../headers/Formula.h
plot3d.o: plot3d.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/diskvid.h ../headers/line3d.h \
  ../headers/loadmap.h ../headers/plot3d.h \
  ../headers/ThreeDimensionalState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h
prompts1.o: prompts1.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/malloc.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/calcfrac.h \
  ../headers/drivers.h ../headers/fihelp.h ../headers/filesystem.h \
  ../headers/fracsuba.h ../headers/fracsubr.h ../headers/fractalp.h \
  ../headers/fractals.h ../headers/loadfile.h ../headers/loadmap.h \
  ../headers/lsys.h ../headers/miscres.h ../headers/prompts1.h \
  ../headers/prompts2.h ../headers/realdos.h ../headers/zoom.h \
  ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  ../headers/ThreeDimensionalState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/Formula.h ../headers/UIChoices.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/vector \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_vector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_bvector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/vector.tcc \
  ../headers/MathUtil.h
prompts2.o: prompts2.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/malloc.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/Browse.h \
  ../headers/cmdfiles.h ../headers/diskvid.h ../headers/drivers.h \
  ../headers/evolve.h ../headers/fihelp.h ../headers/filesystem.h \
  ../headers/fracsubr.h ../headers/history.h ../headers/line3d.h \
  ../headers/loadfile.h ../headers/loadmap.h ../headers/miscres.h \
  ../headers/prompts1.h ../headers/prompts2.h ../headers/realdos.h \
  ../headers/slideshw.h ../headers/zoom.h ../headers/busy.h \
  ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/SoundState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h ../headers/UIChoices.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/vector \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_vector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_bvector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/vector.tcc \
  ../headers/MathUtil.h
realdos.o: realdos.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/time.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/fcntl.h \
  /usr/include/bits/fcntl.h /usr/include/ctype.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/cmdfiles.h \
  ../headers/diskvid.h ../headers/drivers.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/miscres.h ../headers/prompts2.h \
  ../headers/realdos.h
rotate.o: rotate.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/diskvid.h ../headers/drivers.h \
  ../headers/fihelp.h ../headers/filesystem.h ../headers/loadmap.h \
  ../headers/prompts2.h ../headers/rotate.h
slideshw.o: slideshw.cpp /usr/include/ctype.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/string.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/realdos.h ../headers/slideshw.h
soi.o: soi.cpp /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/xlocale.h /usr/include/string.h /usr/include/malloc.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/soi.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h
soi1.o: soi1.cpp /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/xlocale.h /usr/include/string.h /usr/include/malloc.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/soi.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h
stereo.o: stereo.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/encoder.h \
  ../headers/fihelp.h ../headers/gifview.h ../headers/realdos.h \
  ../headers/rotate.h ../headers/stereo.h /usr/include/malloc.h
testpt.o: testpt.cpp ../headers/testpt.h
zoom.o: zoom.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/calcfrac.h ../headers/drivers.h ../headers/fracsubr.h \
  ../headers/framain2.h ../headers/miscfrac.h ../headers/realdos.h \
  ../headers/zoom.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/MathUtil.h ../headers/WorkList.h
Halley.o: Halley.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/cmplx.h ../headers/fractint.h ../headers/externs.h \
  ../headers/fractype.h ../headers/fpu.h ../headers/fractals.h \
  ../headers/Halley.h
Newton.o: Newton.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/cmplx.h ../headers/fractint.h ../headers/prototyp.h \
  ../headers/mpmath.h ../headers/big.h ../headers/cmplx.h \
  ../headers/biginit.h ../headers/fractint.h ../headers/helpcom.h \
  ../headers/externs.h ../headers/unixprot.h ../headers/externs.h \
  ../headers/fractype.h ../headers/mpmath.h ../headers/fpu.h \
  ../headers/calcfrac.h ../headers/fractals.h ../headers/Newton.h \
  ../headers/MathUtil.h
SoundState.o: SoundState.cpp /usr/include/string.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/include/ctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/sstream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/istream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ios \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/localefwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/ios_base.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_classes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/streambuf \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/streambuf.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_ios.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/streambuf_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_facets.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwctype \
  /usr/include/wctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/ctype_base.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/ctype_inline.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/codecvt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/time_members.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/messages_members.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_ios.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/istream.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/locale \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_facets.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/typeinfo \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ostream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/ostream.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/sstream.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractint.h ../headers/externs.h ../headers/helpdefs.h \
  ../headers/cmdfiles.h ../headers/drivers.h ../headers/fihelp.h \
  ../headers/filesystem.h ../headers/realdos.h ../headers/SoundState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h \
  ../headers/CommandParser.h ../headers/UIChoices.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/vector \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_vector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_bvector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/vector.tcc
EscapeTime.o: EscapeTime.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractint.h ../headers/fractals.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h
ThreeDimensionalState.o: ThreeDimensionalState.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/include/ctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/sstream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/istream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ios \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/localefwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/ios_base.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_classes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/streambuf \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/streambuf.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_ios.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/streambuf_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_facets.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwctype \
  /usr/include/wctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/ctype_base.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/ctype_inline.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/codecvt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/time_members.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/messages_members.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_ios.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/istream.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/locale \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/locale_facets.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/typeinfo \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ostream \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/ostream.tcc \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/sstream.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractint.h ../headers/cmdfiles.h ../headers/filesystem.h \
  ../headers/ThreeDimensionalState.h ../headers/CommandParser.h \
  ../headers/cmdfiles.h ../headers/CommandParser.h
MathUtil.o: MathUtil.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cmath \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cmath.tcc \
  ../headers/MathUtil.h
UIChoices.o: UIChoices.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/string \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++config.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/os_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/cpu_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stringfwd.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/char_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algobase.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/climits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/limits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdlib \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/iosfwd \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++locale.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstdio \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/sys_errlist.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/clocale \
  /usr/include/locale.h /usr/include/bits/locale.h \
  /usr/include/langinfo.h /usr/include/nl_types.h /usr/include/iconv.h \
  /usr/include/libintl.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++io.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/gthr-default.h \
  /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h \
  /usr/include/bits/initspin.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cctype \
  /usr/include/ctype.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/postypes.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cwchar \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ctime \
  /usr/include/stdint.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/functexcept.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception_defines.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_pair.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/cpp_type_traits.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_types.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator_base_funcs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/concept_check.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_iterator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/debug/debug.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/memory \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/c++allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/ext/new_allocator.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/new \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/exception \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_construct.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_uninitialized.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_raw_storage_iter.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/limits \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_function.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/atomicity.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/i486-linux-gnu/bits/atomic_word.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/algorithm \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_algo.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_heap.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_tempbuf.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/basic_string.tcc \
  ../headers/port.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/fihelp.h ../headers/prompts1.h \
  ../headers/UIChoices.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/vector \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_vector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/stl_bvector.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/bits/vector.tcc
SolidGuess.o: SolidGuess.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/xlocale.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/cmplx.h ../headers/externs.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/big.h \
  ../headers/cmplx.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/calcfrac.h ../headers/drivers.h ../headers/fracsubr.h \
  ../headers/SolidGuess.h ../headers/WorkList.h
Tesseral.o: Tesseral.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/xlocale.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/externs.h ../headers/calcfrac.h \
  ../headers/fracsubr.h ../headers/miscres.h ../headers/Tesseral.h \
  ../headers/WorkList.h
BoundaryTrace.o: BoundaryTrace.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/xlocale.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/externs.h ../headers/calcfrac.h \
  ../headers/fracsubr.h ../headers/realdos.h ../headers/BoundaryTrace.h \
  ../headers/WorkList.h
DiffusionScan.o: DiffusionScan.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstring \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cstddef \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/xlocale.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/cmplx.h ../headers/externs.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/big.h \
  ../headers/cmplx.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/calcfrac.h ../headers/fracsubr.h ../headers/miscres.h \
  ../headers/DiffusionScan.h ../headers/WorkList.h
WorkList.o: WorkList.cpp \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/cmplx.h ../headers/fractint.h ../headers/externs.h \
  ../headers/fractype.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/big.h ../headers/calcfrac.h \
  ../headers/diskvid.h ../headers/fracsubr.h ../headers/EscapeTime.h \
  ../headers/MathUtil.h ../headers/WorkList.h
filesystem.o: filesystem.cpp /usr/include/string.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/sys/dir.h \
  /usr/include/dirent.h /usr/include/bits/dirent.h \
  /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
  /usr/include/linux/limits.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/fractint.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/filesystem.h ../headers/miscres.h
Browse.o: Browse.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/errno.h \
  /usr/include/bits/errno.h /usr/include/linux/errno.h \
  /usr/include/asm/errno.h /usr/include/asm-i486/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/Browse.h \
  ../headers/drivers.h ../headers/encoder.h ../headers/EscapeTime.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/../../../../include/c++/4.1.2/cassert \
  /usr/include/assert.h ../headers/fihelp.h ../headers/filesystem.h \
  ../headers/Formula.h ../headers/framain2.h ../headers/fracsubr.h \
  ../headers/loadfile.h ../headers/lorenz.h ../headers/miscres.h \
  ../headers/prompts2.h ../headers/realdos.h ../headers/zoom.h
