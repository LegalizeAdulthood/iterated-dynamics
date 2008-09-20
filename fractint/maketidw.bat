@echo off
if exist foo del foo
if exist winfract.map del winfract.map
cd common
del *.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos
if exist uclock.obj del uclock.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos_help
if exist hc.obj del hc.obj
cd ..
cd win
del *.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd win_menu
if exist winfract.res del winfract.res
cd ..
cd win_help
if exist winfract.ph del winfract.ph
cd ..

