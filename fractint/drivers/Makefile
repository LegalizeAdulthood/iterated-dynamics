SHELL=/bin/sh

OBJS = \
d_x11.o d_disk.o

#Need to prevent lex from doing fractint.l -> fractint.c
.SUFFIXES:
.SUFFIXES: .o .c .s .h .asm

all: $(OBJS)

tidy:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

fractint.o: fractint.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c fractint.c

help.o: help.c
	$(CC) $(CFLAGS) -DSRCDIR=\"$(SRCDIR)\" -c help.c

copy: $(FILES)
	mv $(FILES) backup

# DO NOT DELETE THIS LINE -- make depend depends on it.
