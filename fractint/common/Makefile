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
    f16.cpp \
    font8x8.cpp \
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
    tgaview.cpp \
    zoom.cpp \
    Halley.cpp

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
help.o: help.cpp
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c $<

copy: $(FILES)
	mv $(FILES) backup

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

# DO NOT DELETE
3d.o: 3d.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
ant.o: ant.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/fihelp.h
bigflt.o: bigflt.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h
biginit.o: biginit.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h
bignum.o: bignum.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
bignumc.o: bignumc.cpp /usr/include/memory.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h
calcfrac.o: calcfrac.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
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
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/targa_lc.h ../headers/drivers.h
cmdfiles.o: cmdfiles.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h \
  ../headers/unix.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/fractype.h ../headers/drivers.h
decoder.o: decoder.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
diskvid.o: diskvid.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
drivers.o: drivers.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
editpal.o: editpal.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
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
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/helpdefs.h ../headers/fihelp.h
encoder.o: encoder.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h
evolve.o: evolve.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/fihelp.h
f16.o: f16.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/targa_lc.h
font8x8.o: font8x8.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
fracsubr.o: fracsubr.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/memory.h \
  /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
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
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h ../headers/fihelp.h
fractalb.o: fractalb.cpp \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
fractalp.o: fractalp.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
fractals.o: fractals.cpp \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
fractint.o: fractint.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/signal.h \
  /usr/include/bits/sigset.h /usr/include/bits/signum.h \
  /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
  /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
  /usr/include/linux/compiler.h /usr/include/bits/sigstack.h \
  /usr/include/sys/ucontext.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h /usr/include/ctype.h \
  /usr/include/endian.h /usr/include/bits/endian.h ../headers/port.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h
framain2.o: framain2.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h /usr/include/ctype.h \
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
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h
frasetup.o: frasetup.cpp \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
gifview.o: gifview.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
hcmplx.o: hcmplx.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
help.o: help.cpp /usr/include/fcntl.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/fcntl.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
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
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h
history.o: history.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h
intro.o: intro.cpp /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/fihelp.h
jb.o: jb.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/drivers.h
jiim.o: jiim.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
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
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/drivers.h \
  ../headers/fihelp.h
line3d.o: line3d.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
loadfdos.o: loadfdos.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fihelp.h
loadfile.o: loadfile.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/errno.h \
  /usr/include/bits/errno.h /usr/include/linux/errno.h \
  /usr/include/asm/errno.h /usr/include/asm-generic/errno.h \
  /usr/include/asm-generic/errno-base.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/targa_lc.h \
  ../headers/drivers.h ../headers/fihelp.h
loadmap.o: loadmap.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
lorenz.o: lorenz.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h
lsys.o: lsys.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/lsys.h ../headers/drivers.h
lsysf.o: lsysf.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/lsys.h ../headers/drivers.h
memory.o: memory.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h /usr/include/malloc.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/errno.h \
  /usr/include/bits/errno.h /usr/include/linux/errno.h \
  /usr/include/asm/errno.h /usr/include/asm-generic/errno.h \
  /usr/include/asm-generic/errno-base.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
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
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
miscfrac.o: miscfrac.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/bits/xopen_lim.h \
  /usr/include/bits/stdio_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/targa_lc.h ../headers/drivers.h
miscovl.o: miscovl.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
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
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h
miscres.o: miscres.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/malloc.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
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
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h
mpmath_c.o: mpmath_c.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
parser.o: parser.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
parserfp.o: parserfp.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/time.h \
  /usr/include/bits/time.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
plot3d.o: plot3d.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h
prompts1.o: prompts1.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/ctype.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/malloc.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/fihelp.h \
  ../headers/drivers.h
prompts2.o: prompts2.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
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
  /usr/include/linux/limits.h /usr/include/malloc.h /usr/include/fcntl.h \
  /usr/include/bits/fcntl.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h
realdos.o: realdos.cpp /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
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
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h \
  ../headers/fihelp.h
rotate.o: rotate.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h ../headers/fihelp.h
slideshw.o: slideshw.cpp /usr/include/ctype.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/string.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
soi.o: soi.cpp /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/xlocale.h /usr/include/string.h /usr/include/malloc.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
soi1.o: soi1.cpp /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/xlocale.h /usr/include/string.h /usr/include/malloc.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
stereo.o: stereo.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/helpdefs.h ../headers/fihelp.h \
  /usr/include/malloc.h
testpt.o: testpt.cpp
tgaview.o: tgaview.cpp ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
  /usr/include/bits/types.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/targa_lc.h ../headers/drivers.h
zoom.o: zoom.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/xlocale.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/wordsize.h /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
