
HFD = ..\headers

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : winfract.res

.rc.res:
	  rc -r -i$(HFD) $*.rc

winfract.res: winfract.rc mathtool.rc \
	      $(HFD)\mathtool.h $(HFD)\dialog.h $(HFD)\winfract.h \
	      coord.dlg zoom.dlg
	  rc -r -i$(HFD) winfract.rc

