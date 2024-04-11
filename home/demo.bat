@echo off
rem TODO: Re-enable sound demo when audio library is selected.
:top
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
echo 9:  All demos
set /p "choice=Your choice? "
echo.
if "%choice%" == "9" goto all
rem if "%choice%" == "8" goto sound
if "%choice%" == "7" goto evolver
if "%choice%" == "6" goto advanced
if "%choice%" == "5" goto demo4
if "%choice%" == "4" goto demo3
if "%choice%" == "3" goto demo2
if "%choice%" == "2" goto demo1
if "%choice%" == "1" goto basic
if "%choice%" == "0" goto end
goto top

:sound
id savename=.\ filename=.\ curdir=yes autokey=play autokeyname=sound_demo.key
goto top


:evolver
id savename=.\ filename=.\ curdir=yes  autokey=play autokeyname=explore.key
goto top


:advanced
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=advanced.key
goto top


:demo1
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=demo1.key
del demo1*.gif
goto top


:demo2
id savename=.\ filename=.\ curdir=yes @demo.par/trunc_Demo autokey=play autokeyname=demo2.key
del demo2*.gif
goto top


:demo3
set r=demo3b
goto demo3s
:demo3b
del demo3.par
goto top


:demo4
id savename=.\ filename=.\ curdir=yes autokey=play autokeyname=demo4.key
del demo4*.gif
goto top


:basic
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=basic.key
del basic001.gif
goto top


:all
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=basic.key
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=demo1.key
id savename=.\ filename=.\ curdir=yes @demo.par/trunc_Demo autokey=play autokeyname=demo2.key
set r=allb
goto demo3s
:allb
id savename=.\ filename=.\ curdir=yes autokey=play autokeyname=demo4.key
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=advanced.key
del basic001.gif
del demo1*.gif
del demo2*.gif
del demo3.par
del demo4*.gif
rem id savename=.\ filename=.\ curdir=yes autokey=play autokeyname=snddemo1.key
id savename=.\ filename=.\ curdir=yes autokey=play autokeyname=explore.key
goto top


:demo3s
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
id savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=demo3.key
goto %r%

:end
set r=
echo Have a nice day!
