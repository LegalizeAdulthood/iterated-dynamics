@echo off
rem ** The SET commands are here to ensure no options are accidentally set
set CL=
set MASM=
set LINK=
if exist f_errs.txt del f_errs.txt

rem ** Microsoft C7.00 or Visual C++ (normal case)
echo Building WinFract using MSC 7 or Visual C++
nmake "CC=cl /Gs" "AS=masm /ML" "LINKER=link" "OptT=/Oilg" "C7=YES" /F frachelp.mak
if errorlevel 1 goto exit
rem delete hc.exe since it conflicts with the Windows help compiler
if exist hc.exe del hc.exe
if exist helpdefs.h move /Y helpdefs.h .\headers\helpdefs.h
nmake "WINFRACT=YES" @macros_w /F fractint.mak
goto exit

:exit
if exist f_errs.txt type f_errs.txt

