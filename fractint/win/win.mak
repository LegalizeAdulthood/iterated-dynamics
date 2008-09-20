
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

OBJ = dialog.obj mainfrac.obj mathtool.obj profile.obj select.obj \
windos.obj windos2.obj winfract.obj winstubs.obj wintext.obj \
wgeneral.obj

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


dialog.obj: dialog.c $(HFD)\winfract.h $(HFD)\dialog.h $(HFD)\fractint.h

mainfrac.obj: mainfrac.c $(HFD)\fractint.h

mathtool.obj: mathtool.c $(HFD)\winfract.h $(HFD)\mathtool.h

profile.obj: profile.c $(HFD)\winfract.h

select.obj: select.c $(HFD)\select.h

windos.obj: windos.c $(HFD)\winfract.h $(HFD)\fractint.h

windos2.obj: windos2.c $(HFD)\fractint.h

winfract.obj: winfract.c $(HFD)\winfract.h

winstubs.obj: winstubs.c $(HFD)\fractint.h

wintext.obj: wintext.c

wgeneral.obj: wgeneral.asm

