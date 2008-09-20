@echo off
if exist foo del foo
if exist .\headers\helpdefs.h del .\headers\helpdefs.h
if exist fractint.hlp del fractint.hlp
if exist fractint.map del fractint.map
if exist fractint.exe del fractint.exe
cd common
del *.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos
if exist sound.obj del sound.obj
if exist uclock.obj del uclock.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos_help
if exist hc.exe del hc.exe
if exist hc.obj del hc.obj
if exist f_errs.txt del f_errs.txt
cd ..

