@echo off
rem ** The SET commands are here to ensure no options are accidentally set
set CL=
set MASM=
set LINK=
if exist f_errs.txt del f_errs.txt

rem ** Microsoft C7.00 or Visual C++ (normal case)
echo Building WinFract using MSC 7 or Visual C++
cd dos_help
nmake "CC=cl /Gs" "AS=masm /ML" "LINKER=link" "OptT=/Oilg" "C7=YES" /F frachelp.mak
if errorlevel 1 goto exit
if exist helpdefs.h move /Y helpdefs.h ..\headers\helpdefs.h
if exist fractint.hlp move /Y fractint.hlp ..\fractint.hlp
cd ..
nmake "WINFRACT=YES" @macros_w /F fractint.mak
cd win_help
if exist winfract.hlp move /Y winfract.hlp ..\winfract.hlp
cd ..
goto exit

:exit
if exist f_errs.txt type f_errs.txt

