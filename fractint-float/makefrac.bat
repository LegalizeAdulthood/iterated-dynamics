@echo off
rem ** The SET commands are here to ensure no options are accidentally set
set CL=
set MASM=
set LINK=
if exist f_errs.txt del f_errs.txt

rem ** Microsoft C7.00 or Visual C++ (normal case)
echo Building Fractint using MSC 7 or Visual C++
nmake "CC=cl /Gs" "AS=masm /ML" "LINKER=link" "OptT=/Oilg" "C7=YES" frachelp.mak
if errorlevel 1 goto exit
nmake "CC=cl /Gs /DC6 /DFLOATONLY" "AS=masm /ML" "LINKER=link" "OptT=/Oeciltaz" "OptS=/Osleazc" "OptN=/Oilc" "C7=YES" fractint.mak
goto exit

:exit
