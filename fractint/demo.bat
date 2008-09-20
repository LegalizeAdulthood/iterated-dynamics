echo off
:top
cls
echo ************************************************************
echo *                                                          *
echo *  The following demonstration of Fractint will only work  *
echo *  with a VGA or higher resolution video system.           *
echo *  The demo should be run in a directory containing all    *
echo *  the Fractint release files. The evolver demo requires   *
echo *  the SF5 640x480 mode, and the sound demo needs a sound  *
echo *  and speakers installed, otherwise, only vanilla VGA     *
echo *  and features are used.                                  *
echo *                                                          *
echo ************************************************************
echo.
echo 0:  Exit
echo 1:  Basic commands demo
echo 2:  New stuff in version 19
echo 3:  New stuff in version 19.4
echo 4:  New stuff in version 19.5
echo 5:  New stuff in version 19.6
echo 6:  Advanced commands demo
echo 7:  New for 20.0 - Evolver
echo 8:  New for 20.0 - Sound
echo 9:  All six demos
sschoice
echo.
if errorlevel 9 goto all
if errorlevel 8 goto sound
if errorlevel 7 goto evolver
if errorlevel 6 goto advanced
if errorlevel 5 goto newin196
if errorlevel 4 goto newin195
if errorlevel 3 goto newin194
if errorlevel 2 goto newin19
if errorlevel 1 goto basic
if errorlevel 0 goto end
goto top

:sound
fractint savename=.\ filename=.\ curdir=yes autokey=play autokeyname=snddemo1.key
goto top


:evolver
fractint savename=.\ filename=.\ curdir=yes  autokey=play autokeyname=explore.key
goto top


:advanced
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=advanced.key
goto top

:newin19
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=new19.key
del new19???.gif
goto top

:newin194
fractint savename=.\ filename=.\ curdir=yes @demo.par/trunc_Demo autokey=play autokeyname=new19-4.key
del new1940?.gif
goto top

:newin195
set r=n195b
goto n195s
:n195b
del demo195.par
goto top

:newin196
fractint savename=.\ filename=.\ curdir=yes autokey=play autokeyname=new19-6.key
del demo196?.gif
goto top

:basic
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=basic.key
del basic001.gif
goto top

:all
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=basic.key
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=new19.key
fractint savename=.\ filename=.\ curdir=yes @demo.par/trunc_Demo autokey=play autokeyname=new19-4.key
set r=allb
goto n195s
:allb
fractint savename=.\ filename=.\ curdir=yes autokey=play autokeyname=new19-6.key
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=advanced.key
del basic001.gif
del new19???.gif
del new1940?.gif
del demo195.par
del demo196?.gif
fractint savename=.\ filename=.\ curdir=yes autokey=play autokeyname=snddemo1.key
fractint savename=.\ filename=.\ curdir=yes autokey=play autokeyname=explore.key
goto top

:n195s
cls
echo.
echo   This autokey demonstrates the new "comment=" and "colors=" commands
echo   ===================================================================
echo.
echo   It will draw the classic Mandel fractal, then:
echo.
echo   - load default.map
echo   - enter "recordcolors=comment"
echo           The compressed palette will be written in the par entry and
echo           the .map file will be in a comment
echo   - enter "comments=$date$/$xdots$_x_$ydots$/On_a_Pentium_166/$calctime$
echo           These comments will be expanded as follow in the par:
echo           Mandel_demo { ; Aug 26, 1996
echo                         ; 160 x 100
echo"                        ; On a Pentium 166
echo                         ;  0:00:00.07
echo   - make a par called "Mandel_Demo"
echo   - load this par and let you see with F2 the complete par entry
echo   - exit
echo.
pause
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=new19-5.key
goto %r%

:end
set r=
echo Have a nice day!
