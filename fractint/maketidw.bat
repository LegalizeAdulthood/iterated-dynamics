@echo off
if exist hc.obj del hc.obj
if exist foo del foo
if exist winfract.map del winfract.map
if exist winfract.ph del winfract.ph
if exist winfract.res del winfract.res
cd common
del *.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd dos
if exist uclock.obj del uclock.obj
if exist f_errs.txt del f_errs.txt
cd ..
cd win
del *.obj
if exist f_errs.txt del f_errs.txt
cd ..

