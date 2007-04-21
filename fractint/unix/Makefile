SHELL=/bin/sh

ifeq ($(AS),/usr/bin/nasm)

OBJS = \
calcmand.o calmanfp.o diskvidu.o fpu087.o fracsuba.o general.o \
unix.o unixscr.o video.o \
calmanfx.o d_x11.o od_unix.o

else

OBJS = \
calcmand.o calmanfp.o diskvidu.o fpu087.o fracsuba.o general.o \
unix.o unixscr.o video.o d_x11.o os_unix.o

endif

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

depend:
	makedepend $(CFLAGS)

ifeq ($(AS),/usr/bin/nasm)
calmanfx.o: calmanfx.asm
	$(AS) $(AFLAGS) calmanfx.asm
endif

unix.o: unix.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c unix.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.
