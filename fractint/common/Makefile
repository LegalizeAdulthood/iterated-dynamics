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
	DiffusionScan.cpp

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
