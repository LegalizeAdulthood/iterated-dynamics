@echo off
setlocal
rem TODO: Re-enable sound demo when audio library is selected.
:menu
cls
echo ************************************************************
echo *                                                          *
echo *  The demo should be run in a directory containing all    *
echo *  the Iterated Dynamics release files. The evolver demo   *
echo *  requires the SF5 video mode, otherwise only vanilla     *
echo *  features are used.                                      *
echo *                                                          *
echo ************************************************************
echo.
echo 0:  Exit
echo 1:  Basic commands demo
echo 2:  Bailout tests, browser, outside=atan, ants
echo 3:  Rounding functions, showdot turtle
echo 4:  recordcolors=, comment= parameters
echo 5:  fastrestore parameter, text scrolling in ^<Z^> screen
echo 6:  Advanced commands demo
echo 7:  Evolver demo
echo.
echo 9:  All demos
echo.
set /p "choice=Your choice? "

if "%choice%" == "0" goto end
if "%choice%" == "1" call :basic
if "%choice%" == "2" call :demo1
if "%choice%" == "3" call :demo2
if "%choice%" == "4" call :demo3
if "%choice%" == "5" call :demo4
if "%choice%" == "6" call :advanced
if "%choice%" == "7" call :evolver
rem if "%choice%" == "8" call :sound
if "%choice%" == "9" call :all
goto menu

:play-auto-key
start /wait id savename=.\ filename=.\ curdir=yes autokey=play %*
exit /b 0

:basic
call :play-auto-key @demo.par/Mandel_Demo autokeyname=basic.key
del basic001.gif
exit /b 0


:demo1
call :play-auto-key @demo.par/Mandel_Demo autokeyname=demo1.key
del demo1*.gif
exit /b 0


:demo2
call :play-auto-key @demo.par/trunc_Demo autokeyname=demo2.key
del demo2*.gif
exit /b 0


:demo3
cls
echo.
echo   This autokey demonstrates the "comment=" and "colors=" parameters
echo   ===================================================================
echo.
echo   It will draw the classic Mandel fractal, then:
echo.
echo   - load default.map
echo   - enter "recordcolors=comment"
echo           The compressed palette will be written in the par entry and
echo           the .map file will be in a comment
echo   - enter "comments=$date$/$xdots$_x_$ydots$/Calculation_time:_$calctime$
echo           These comments will be expanded as follow in the par:
echo           Mandel_demo { ; May 20, 2024
echo                         ; 800 x 600
echo                         ; Calculation time: 0:00:00.07
echo   - make a par called "Mandel_Demo"
echo   - load this par and let you see with F2 the complete par entry
echo   - exit
echo.
pause
call :play-auto-key @demo.par/Mandel_Demo autokeyname=demo3.key
exit /b 0


:demo4
call :play-auto-key autokeyname=demo4.key
del demo4*.gif
exit /b 0


:advanced
call :play-auto-key @demo.par/Mandel_Demo autokeyname=advanced.key
exit /b 0


:evolver
call :play-auto-key autokeyname=explore.key
exit /b 0


:sound
call :play-auto-key autokeyname=sound_demo.key
exit /b 0


:all
call :basic
call :demo1
call :demo2
call :demo3
call :demo4
call :advanced
call :evolver
rem call :sound
exit /b 0


:end
echo Have a nice day!
