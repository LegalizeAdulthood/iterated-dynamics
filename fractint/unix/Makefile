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
