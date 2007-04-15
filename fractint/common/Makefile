SHELL=/bin/sh

OBJS = \
3d.o ant.o bigflt.o  biginit.o bignum.o bignumc.o calcfrac.o \
cmdfiles.o decoder.o drivers.o editpal.o encoder.o \
evolve.o f16.o fracsubr.o fractalb.o fractalp.o fractals.o \
fractint.o framain2.o frasetup.o gifview.o hcmplx.o help.o history.o \
intro.o jb.o jiim.o line3d.o loadfdos.o loadfile.o loadmap.o lorenz.o \
lsys.o lsysf.o memory.o miscfrac.o miscovl.o miscres.o mpmath_c.o parser.o \
parserfp.o plot3d.o prompts1.o prompts2.o \
realdos.o rotate.o slideshw.o soi.o soi1.o stereo.o testpt.o tgaview.o \
zoom.o

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

depend:
	gccmakedep -- $(CFLAGS) -- $(OBJS:.o=.c)
#	makedepend $(CFLAGS) $(OBJS:.o=.c)

fractint.o: fractint.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c fractint.c

help.o: help.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c help.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE
3d.o: 3d.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
ant.o: ant.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h
bigflt.o: bigflt.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h
biginit.o: biginit.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/malloc.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h
bignum.o: bignum.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
bignumc.o: bignumc.c /usr/include/memory.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/big.h ../headers/cmplx.h ../headers/biginit.h
calcfrac.o: calcfrac.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/targa_lc.h ../headers/drivers.h
cmdfiles.o: cmdfiles.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h \
  ../headers/unix.h ../headers/prototyp.h ../headers/mpmath.h \
  ../headers/cmplx.h ../headers/big.h ../headers/biginit.h \
  ../headers/fractint.h ../headers/helpcom.h ../headers/externs.h \
  ../headers/unixprot.h ../headers/fractype.h ../headers/drivers.h
decoder.o: decoder.c ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
drivers.o: drivers.c ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h /usr/include/string.h
editpal.o: editpal.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
encoder.o: encoder.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h
evolve.o: evolve.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h
f16.o: f16.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/targa_lc.h
fracsubr.o: fracsubr.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/memory.h \
  /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h
fractalb.o: fractalb.c /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/malloc.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
fractalp.o: fractalp.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
fractals.o: fractals.c /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/malloc.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
fractint.o: fractint.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/signal.h /usr/include/bits/sigset.h \
  /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
  /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
  /usr/include/asm/sigcontext.h /usr/include/linux/compiler.h \
  /usr/include/bits/sigstack.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/bits/sigthread.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h /usr/include/ctype.h \
  /usr/include/endian.h /usr/include/bits/endian.h ../headers/port.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/sys/sysmacros.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
framain2.o: framain2.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h /usr/include/ctype.h \
  /usr/include/endian.h /usr/include/bits/endian.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
frasetup.o: frasetup.c /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/malloc.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h
gifview.o: gifview.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
hcmplx.o: hcmplx.c ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
help.o: help.c /usr/include/fcntl.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/fcntl.h /usr/include/sys/types.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/time.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/string.h /usr/include/assert.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h
history.o: history.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h
intro.o: intro.c /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h
jb.o: jb.c ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/drivers.h
jiim.o: jiim.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h /usr/include/stdlib.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/fractype.h ../headers/drivers.h
line3d.o: line3d.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
loadfdos.o: loadfdos.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h
loadfile.o: loadfile.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/targa_lc.h \
  ../headers/drivers.h
loadmap.o: loadmap.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
lorenz.o: lorenz.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/drivers.h
lsys.o: lsys.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/malloc.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/lsys.h ../headers/drivers.h
lsysf.o: lsysf.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/malloc.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/lsys.h ../headers/drivers.h
memory.o: memory.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h /usr/include/malloc.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/errno.h \
  /usr/include/bits/errno.h /usr/include/linux/errno.h \
  /usr/include/asm/errno.h /usr/include/asm-generic/errno.h \
  /usr/include/asm-generic/errno-base.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
miscfrac.o: miscfrac.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/limits.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/syslimits.h \
  /usr/include/limits.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/bits/posix2_lim.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/targa_lc.h ../headers/drivers.h
miscovl.o: miscovl.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/string.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
miscres.o: miscres.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/malloc.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
mpmath_c.o: mpmath_c.c ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
parser.o: parser.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/time.h /usr/include/bits/time.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
parserfp.o: parserfp.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/time.h /usr/include/bits/time.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
plot3d.o: plot3d.c /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h
prompts1.o: prompts1.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
prompts2.o: prompts2.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/ctype.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/dir.h /usr/include/dirent.h \
  /usr/include/bits/dirent.h /usr/include/bits/posix1_lim.h \
  /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
  /usr/include/malloc.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
realdos.o: realdos.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
  /usr/include/ctype.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fractype.h ../headers/helpdefs.h ../headers/drivers.h
rotate.o: rotate.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/helpdefs.h ../headers/drivers.h
slideshw.o: slideshw.c /usr/include/ctype.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/time.h /usr/include/bits/time.h \
  /usr/include/string.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
soi.o: soi.c /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/string.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
soi1.o: soi1.c /usr/include/time.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  /usr/include/string.h /usr/include/malloc.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
stereo.o: stereo.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h /usr/include/time.h \
  /usr/include/bits/time.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
  /usr/include/bits/endian.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h ../headers/helpdefs.h
testpt.o: testpt.c
tgaview.o: tgaview.c ../headers/port.h /usr/include/unistd.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/targa_lc.h ../headers/drivers.h
zoom.o: zoom.c /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/stddef.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdio.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc-lib/i486-linux/3.3.5/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc-lib/i486-linux/3.3.5/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/drivers.h
