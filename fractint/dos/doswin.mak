
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

OBJ = uclock.obj \
bignuma.obj calcmand.obj calmanp5.obj calmanfp.obj fpu087.obj fpu387.obj \
fracsuba.obj lsysa.obj lsysaf.obj \
lyapunov.obj mpmath_a.obj newton.obj parsera.obj

HFD = ..\headers

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : $(OBJ)

.asm.obj:
	$(AS) /W3 $*; >> f_errs.txt
# for Quick Assembler
#       $(AS) $*.asm

.c.obj:
	  $(CC) /I$(HFD) $(OptT) $*.c >> f_errs.txt

Optsize = $(CC) /I$(HFD) $(OptS) $*.c >> f_errs.txt

Optnoalias = $(CC) /I$(HFD) $(OptN) $*.c >> f_errs.txt


uclock.obj  : uclock.c $(HFD)\uclock.h

bignuma.obj : bignuma.asm big.inc
# for MASM
        $(AS) /e /W3 bignuma.asm; >> f_errs.txt
# for QuickAssembler
#   $(AS) /FPi bignuma.asm >> f_errs.txt

calcmand.obj : calcmand.asm

calmanfp.obj : calmanfp.asm
# for MASM
	$(AS) /e /W3 calmanfp; >> f_errs.txt
#	$(AS) /e calmanfp;
# for QuickAssembler
#   $(AS) /FPi calmanfp.asm

calmanp5.obj : calmanp5.asm
	$(AS) /W3 calmanp5; >> f_errs.txt

fpu087.obj : fpu087.asm
	$(AS) /e /W3 fpu087; >> f_errs.txt

fpu387.obj : fpu387.asm

fracsuba.obj : fracsuba.asm

lsysa.obj: lsysa.asm

lsysaf.obj: lsysaf.asm
# for MASM
	$(AS) /e /W3 lsysaf.asm; >> f_errs.txt
# for QuickAssembler
#   $(AS) /FPi lsysaf.asm

lyapunov.obj : lyapunov.asm
	$(AS) /e /W3 lyapunov; >> f_errs.txt

mpmath_a.obj : mpmath_a.asm

newton.obj : newton.asm
	$(AS) /e newton;

parsera.obj: parsera.asm
# for MASM
!ifndef DEBUG
	$(AS) /W3 parsera; >> f_errs.txt
#	$(AS) /e parsera;
!else
	$(AS) /e /Zi parsera;
!endif
# for QuickAssembler
#   $(AS) /FPi parsera.asm

