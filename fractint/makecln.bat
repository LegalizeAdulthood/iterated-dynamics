@echo off
del hc.exe
del hc.obj
del foo
del .\headers\helpdefs.h
del fractint.hlp
del fractint.map
del fractint.exe
cd common
del *.obj
del f_errs.txt
cd ..
cd dos
del sound.obj
del uclock.obj
del f_errs.txt
cd ..


