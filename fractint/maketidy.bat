@echo off
if exist foo del foo
if exist fractint.map del fractint.map
cd common
del *.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos
if exist sound.obj del sound.obj
if exist tplus.obj del tplus.obj
if exist uclock.obj del uclock.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos_help
if exist hc.obj del hc.obj
cd ..

