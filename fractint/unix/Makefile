SHELL=/bin/sh

SRCS = \
    calcmand.cpp \
    calmanfp.cpp \
    d_x11.cpp \
    diskvidu.cpp \
    fpu087.cpp \
    general.cpp \
    os_unix.cpp \
    unix.cpp \
    unixscr.cpp \
    video.cpp

ifeq ($(AS),/usr/bin/nasm)
ASMOBJS = calmanfx.o
else
ASMOBJS =
endif

OBJS = $(SRCS:.cpp=.o) $(ASMOBJS)

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .cpp .s .h .asm

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

depend:
	gccmakedep -- $(CFLAGS) -- $(SRCS)
#       makedepend $(CFLAGS) $(SRCS)

ifeq ($(AS),/usr/bin/nasm)
calmanfx.o: calmanfx.asm
	$(AS) $(AFLAGS) calmanfx.asm
endif

unix.o: unix.cpp
	$(CXX) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c $<

.cpp.o:
	$(CXX) $(CFLAGS) -c $<

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE
calcmand.o: calcmand.cpp /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdlib.h \
  /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
  /usr/include/endian.h /usr/include/bits/endian.h /usr/include/xlocale.h \
  /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h \
  /usr/include/bits/select.h /usr/include/bits/sigset.h \
  /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/alloca.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h
calmanfp.o: calmanfp.cpp ../headers/port.h /usr/include/unistd.h \
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
  ../headers/fractype.h ../headers/externs.h ../headers/calmanfp.h \
  ../headers/fracsubr.h ../headers/MathUtil.h
d_x11.o: d_x11.cpp /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
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
  /usr/include/alloca.h /usr/include/string.h /usr/include/X11/Xlib.h \
  /usr/include/X11/X.h /usr/include/X11/Xfuncproto.h \
  /usr/include/X11/Xosdefs.h /usr/include/X11/keysym.h \
  /usr/include/X11/keysymdef.h /usr/include/X11/Xatom.h \
  /usr/include/X11/Xutil.h /usr/include/signal.h \
  /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
  /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
  /usr/include/asm/sigcontext.h /usr/include/asm-i486/sigcontext.h \
  /usr/include/linux/compiler.h /usr/include/bits/sigstack.h \
  /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h \
  /usr/include/sys/time.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm-i486/ioctls.h /usr/include/asm/ioctl.h \
  /usr/include/asm-i486/ioctl.h /usr/include/asm-generic/ioctl.h \
  /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h \
  /usr/include/sys/wait.h /usr/include/sys/resource.h \
  /usr/include/bits/resource.h ../headers/helpdefs.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/externs.h ../headers/fihelp.h ../headers/calcfrac.h \
  ../headers/diskvid.h ../headers/drivers.h ../headers/editpal.h \
  ../headers/prompts2.h ../headers/zoom.h ../headers/SoundState.h \
  ../headers/CommandParser.h ../headers/cmdfiles.h /usr/include/assert.h
diskvidu.o: diskvidu.cpp /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/string.h /usr/include/xlocale.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdlib.h \
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
  ../headers/diskvid.h ../headers/drivers.h ../headers/realdos.h
fpu087.o: fpu087.cpp ../headers/port.h /usr/include/unistd.h \
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
general.o: general.cpp /usr/include/string.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/xlocale.h /usr/include/stdio.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h /usr/include/bits/typesizes.h \
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
  /usr/include/bits/sched.h /usr/include/alloca.h /usr/include/sys/time.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/fihelp.h ../headers/miscres.h ../headers/slideshw.h
os_unix.o: os_unix.cpp /usr/include/sys/statfs.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/include/bits/statfs.h /usr/include/bits/types.h \
  /usr/include/bits/wordsize.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/typesizes.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/bits/wchar.h /usr/include/gconv.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
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
  ../headers/cmplx.h ../headers/fractint.h ../headers/mpmath.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/big.h \
  ../headers/cmplx.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h
unix.o: unix.cpp /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
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
  /usr/include/alloca.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
  /usr/include/sys/stat.h /usr/include/bits/stat.h \
  /usr/include/sys/time.h /usr/include/sys/file.h /usr/include/string.h \
  /usr/include/ctype.h ../headers/port.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h /usr/include/math.h \
  /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h \
  /usr/include/bits/huge_vall.h /usr/include/bits/inf.h \
  /usr/include/bits/nan.h /usr/include/bits/mathdef.h \
  /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  /usr/include/sys/dir.h /usr/include/dirent.h /usr/include/bits/dirent.h \
  /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
  /usr/include/linux/limits.h
unixscr.o: unixscr.cpp /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
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
  /usr/include/alloca.h /usr/include/string.h /usr/include/curses.h \
  /usr/include/ncurses_dll.h /usr/include/unctrl.h \
  /usr/include/X11/Xlib.h /usr/include/X11/X.h \
  /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
  /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
  /usr/include/X11/Xatom.h /usr/include/X11/Xutil.h /usr/include/signal.h \
  /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
  /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
  /usr/include/asm/sigcontext.h /usr/include/asm-i486/sigcontext.h \
  /usr/include/linux/compiler.h /usr/include/bits/sigstack.h \
  /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h \
  /usr/include/sys/wait.h /usr/include/sys/resource.h \
  /usr/include/bits/resource.h /usr/include/sys/time.h \
  /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h \
  /usr/include/asm/ioctls.h /usr/include/asm-i486/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/asm-i486/ioctl.h \
  /usr/include/asm-generic/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h /usr/include/fcntl.h \
  /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h ../headers/helpdefs.h ../headers/port.h \
  /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/math.h /usr/include/bits/huge_val.h \
  /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h \
  /usr/include/bits/inf.h /usr/include/bits/nan.h \
  /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/float.h ../headers/unix.h \
  ../headers/prototyp.h ../headers/mpmath.h ../headers/cmplx.h \
  ../headers/big.h ../headers/biginit.h ../headers/fractint.h \
  ../headers/helpcom.h ../headers/externs.h ../headers/unixprot.h \
  ../headers/externs.h ../headers/fihelp.h ../headers/drivers.h \
  ../headers/editpal.h ../headers/prompts2.h ../headers/zoom.h \
  /usr/include/assert.h
video.o: video.cpp /usr/include/curses.h /usr/include/ncurses_dll.h \
  /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h /usr/lib/gcc/i486-linux-gnu/4.1.2/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/unctrl.h /usr/include/string.h /usr/include/xlocale.h \
  ../headers/port.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/stdlib.h \
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
  ../headers/externs.h ../headers/calcfrac.h ../headers/diskvid.h \
  ../headers/drivers.h ../headers/realdos.h
