
HFD = .\headers
COMDIR = .\common

HELP = help.src help2.src help3.src help4.src help5.src

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : hc.exe fractint.hlp

# commented out remake of hc.c since it hasn't changed for many moons.
hc.obj : $(COMDIR)\hc.c $(HFD)\helpcom.h $(HFD)\port.h
	$(CC) /AL /W1 /FPi /c /I$(HFD) $(OptT) $(COMDIR)\hc.c

hc.exe : hc.obj
	$(LINKER) /ST:4096 /CP:1 /EXEPACK hc;

fractint.hlp : hc.exe $(HELP)
	 hc /c

