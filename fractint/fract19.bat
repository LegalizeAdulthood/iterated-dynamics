@echo off

set mode_1280=1280
set mode_1600=1600
set gifdir=?

rem
rem                           +---------------+
rem                           ! Select images !
rem                           +---------------+

:select_images
cls
echo                      FRACT19.PAR generating batch file
echo                      -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
echo.
if not %gifdir%==? goto si1

echo    I don't know the path for your .gif files.  Please, add it to the 5th line
echo  of this file (e.g.  set gifdir=gifs  or:  set gifdir=c:\fract\gifs)
echo.
goto end

:si1
if not %gifdir%=="" set gifdir=%gifdir%\
echo                          Select a set of images
echo                          ----------------------
echo     1 - Images for Fractint 19.3
echo         5 images:  Achute, Aguay, Lumber, Maculated_1, Zorro.
echo     2 - Images for Fractint 19.3
echo         5 images:  30,000-Feet, Ant, Barnsleyj2-manh, Barnsleyj2-manr, Chip.
echo     3 - Images for Fractint 19.3
echo         5 images:  Fractint, Newton-real, Threeply, TileJulia, TileMandel.
echo     4 - Images for Fractint 19.3
echo         4 images:  Caverns_Of_Mongue, Mandel-virus, NutcrackerMonsters,
echo                    Sliced-Tomato.
echo     5 - Images for Fractint 19.4
echo         4 images:  Graphs, G-3-03-M, Spirals, Jdsg410.
echo     6 - Images for Fractint 19.5
echo         2 images:  Ptcmjn01, Ptc4m01.
echo     7 - New Images for Fractint 19.6
echo         3 images:  EJ_01 Oortcld SG8-21-12
echo     8 - Exit
choice /n /c:12345678 "                   Your selection: "
if errorlevel 8 goto END
if errorlevel 7 goto 19_6_1
if errorlevel 6 goto 19_5_1
if errorlevel 5 goto 19_4_1
if errorlevel 4 goto 19_3_4
if errorlevel 3 goto 19_3_3
if errorlevel 2 goto 19_3_2
if errorlevel 1 goto 19_3_1

:19_3_1
set pars=1
goto select_video
:19_3_2
set pars=2
goto select_video
:19_3_3
set pars=3
goto select_video
:19_3_4
set pars=4
goto select_video
:19_4_1
set pars=5
goto select_video
:19_5_1
set pars=6
goto select_video
:19_6_1
set pars=7

rem                           +--------------+
rem                           ! Select video !
rem                           +--------------+

:select_video
cls
if %pars%==1 echo                Total time: 10 minutes at 1024x768 on a P166
if %pars%==2 echo                Total time: 4 mins 30 s at 1024x768 on a P166
if %pars%==3 echo                Total time: 10 minutes at 1024x768 on a P166
if %pars%==4 echo                Total time: 24 minutes at 1024x768 on a P166
if %pars%==5 echo                Total time: 12 minutes at 1024x768 on a P166
if %pars%==6 echo                Total time: 11 minutes at 1024x768 on a P166
if %pars%==7 echo                Total time: 14 minutes at 1024x768 on a P166
echo.
echo     Use this table to get an idea of the calculation time on your computer:
echo +---------+----------+----------+----------+----------+-----------+-----------+
echo !         ! 320x200  ! 640x480  ! 800x600  ! 1024x768 ! 1280x1024 ! 1600x1200 !
echo +---------+----------+----------+----------+----------+-----------+-----------+
echo ! DX2-66  ! t / 2.9  ! t x 1.7  ! t x 2.7  ! t x 4.3  ! t x 7.2   ! t x 10.5  !
echo +---------+----------+----------+----------+----------+-----------+-----------+
echo ! DX4-100 ! t / 3.8  ! t x 1.2  !  t x 2   ! t x 3.2  ! t x 5.3   ! t x 7.8   !
echo +---------+----------+----------+----------+----------+-----------+-----------+
echo !  P-100  ! t / 8.2  ! t / 1.7  ! t / 1.1  ! t x 1.5  ! t x 2.5   ! t x 3.7   !
echo +---------+----------+----------+----------+----------+-----------+-----------+
echo !  P-166  ! t / 12.3 ! t / 2.6  ! t / 1.6  !  t = 1   ! t x 1.7   ! t x 2.5   !
echo +---------+----------+----------+----------+----------+-----------+-----------+
echo                            Select a video mode
echo                            -------------------
echo                            1 - 320 x 200 (F3)
echo                            2 - 640 x 480 (SF5)
echo                            3 - 800 x 600 (SF6)
echo                            4 - 1024 x 768 (SF7)
echo                            5 - 1280 x 1024
echo                            6 - 1600 x 1200
echo                            7 - Exit
choice /n /c:1234567 "                           Your selection: "
if errorlevel 7 goto end
if errorlevel 6 goto 1600x1200
if errorlevel 5 goto 1280x1024
if errorlevel 4 goto 1024x768
if errorlevel 3 goto 800x600
if errorlevel 2 goto 640x480
if errorlevel 1 goto 320x200

:320x200
set video=F3
goto credits
:640x480
set video=SF5
goto credits
:800x600
set video=SF6
goto credits
:1024x768
set video=SF7
goto credits
:1280x1024
set video=%mode_1280%
if %mode_1280%==1280 goto viderrmsg
goto credits
:1600x1200
set video=%mode_1600%
if %mode_1600%==1600 goto viderrmsg

:credits
echo.
echo        Fractint batch file for FRACT19.PAR created with PARtoBAT 3.3
echo               from Michael Peters (100041.247@compuserve.com)
echo.
echo     Command file written by Sylvie Gallet (101324.3444@compuserve.com)
pause

rem                      +------------------------+
rem                      ! Images for 19.3 part 1 !
rem                      +------------------------+

:19_3_1_g
if not %pars%==1 goto 19_3_2_g
IF EXIST %gifdir%ACHUTE.GIF GOTO 2
FRACTINT video=%video% @FRACT19.PAR/ACHUTE BATCH=YES SAVENAME=ACHUTE
IF ERRORLEVEL 2 GOTO ABORT
:2
IF EXIST %gifdir%AGUAY.GIF GOTO 3
FRACTINT video=%video% @FRACT19.PAR/AGUAY BATCH=YES SAVENAME=AGUAY
IF ERRORLEVEL 2 GOTO ABORT
:3
IF EXIST %gifdir%LUMBER.GIF GOTO 4
FRACTINT video=%video% @FRACT19.PAR/LUMBER BATCH=YES SAVENAME=LUMBER
IF ERRORLEVEL 2 GOTO ABORT
:4
IF EXIST %gifdir%MACULATE.GIF GOTO 5
FRACTINT video=%video% @FRACT19.PAR/MACULATED_1 BATCH=YES SAVENAME=MACULATE
IF ERRORLEVEL 2 GOTO ABORT
:5
IF EXIST %gifdir%ZORRO.GIF GOTO 6
FRACTINT video=%video% @FRACT19.PAR/ZORRO BATCH=YES SAVENAME=ZORRO
IF ERRORLEVEL 2 GOTO ABORT
:6
goto SUCCESS

rem                      +------------------------+
rem                      ! Images for 19.3 part 2 !
rem                      +------------------------+

:19_3_2_g
if not %pars%==2 goto 19_3_3_g
IF EXIST %gifdir%30_000_F.GIF GOTO 7
FRACTINT video=%video% @FRACT19.PAR/30,000-FEET BATCH=YES SAVENAME=30_000_F
IF ERRORLEVEL 2 GOTO ABORT
:7
IF EXIST %gifdir%ANT.GIF GOTO 8
FRACTINT video=%video% @FRACT19.PAR/ANT BATCH=YES SAVENAME=ANT
IF ERRORLEVEL 2 GOTO ABORT
:8
IF EXIST %gifdir%BARNSL-H.GIF GOTO 9
FRACTINT video=%video% @FRACT19.PAR/BARNSLEYJ2-MANH BATCH=YES SAVENAME=BARNSL-H
IF ERRORLEVEL 2 GOTO ABORT
:9
IF EXIST %gifdir%BARNSL-R.GIF GOTO 10
FRACTINT video=%video% @FRACT19.PAR/BARNSLEYJ2-MANR BATCH=YES SAVENAME=BARNSL-R
IF ERRORLEVEL 2 GOTO ABORT
:10
IF EXIST %gifdir%CHIP.GIF GOTO 11
FRACTINT video=%video% @FRACT19.PAR/CHIP BATCH=YES SAVENAME=CHIP
IF ERRORLEVEL 2 GOTO ABORT
:11
goto SUCCESS

rem                      +------------------------+
rem                      ! Images for 19.3 part 3 !
rem                      +------------------------+

:19_3_3_g
if not %pars%==3 goto 19_3_4_g
IF EXIST %gifdir%FRACTINT.GIF GOTO 12
FRACTINT video=%video% @FRACT19.PAR/FRACTINT BATCH=YES SAVENAME=FRACTINT
IF ERRORLEVEL 2 GOTO ABORT
:12
IF EXIST %gifdir%NEWTON_R.GIF GOTO 13
FRACTINT video=%video% @FRACT19.PAR/NEWTON-REAL BATCH=YES SAVENAME=NEWTON_R
IF ERRORLEVEL 2 GOTO ABORT
:13
IF EXIST %gifdir%THREEPLY.GIF GOTO 14
FRACTINT video=%video% @FRACT19.PAR/THREEPLY BATCH=YES SAVENAME=THREEPLY
IF ERRORLEVEL 2 GOTO ABORT
:14
IF EXIST %gifdir%TILEJULI.GIF GOTO 15
FRACTINT video=%video% @FRACT19.PAR/TILEJULIA BATCH=YES SAVENAME=TILEJULI
IF ERRORLEVEL 2 GOTO ABORT
:15
IF EXIST %gifdir%TILEMAND.GIF GOTO 16
FRACTINT video=%video% @FRACT19.PAR/TILEMANDEL BATCH=YES SAVENAME=TILEMAND
IF ERRORLEVEL 2 GOTO ABORT
:16
goto SUCCESS

rem                      +------------------------+
rem                      ! Images for 19.3 part 4 !
rem                      +------------------------+

:19_3_4_g
if not %pars%==4 goto 19_4_1_g
IF EXIST %gifdir%CAVERNS.GIF GOTO 17
FRACTINT video=%video% @FRACT19.PAR/CAVERNS_OF_MONGUE BATCH=YES SAVENAME=CAVERNS
IF ERRORLEVEL 2 GOTO ABORT
:17
IF EXIST %gifdir%MANDEL_V.GIF GOTO 18
FRACTINT video=%video% @FRACT19.PAR/MANDEL-VIRUS BATCH=YES SAVENAME=MANDEL_V
IF ERRORLEVEL 2 GOTO ABORT
:18
IF EXIST %gifdir%NUTCRACK.GIF GOTO 19
FRACTINT video=%video% @FRACT19.PAR/NUTCRACKERMONSTERS BATCH=YES SAVENAME=NUTCRACK
IF ERRORLEVEL 2 GOTO ABORT
:19
IF EXIST %gifdir%SLICED_T.GIF GOTO 20
FRACTINT video=%video% @FRACT19.PAR/SLICED-TOMATO BATCH=YES SAVENAME=SLICED_T
IF ERRORLEVEL 2 GOTO ABORT
:20
GOTO SUCCESS

rem                         +-----------------+
rem                         ! Images for 19.4 !
rem                         +-----------------+

:19_4_1_g
if not %pars%==5 goto 19_5_1_g
IF EXIST %gifdir%GRAPHS.GIF GOTO 22
FRACTINT video=%video% @FRACT19.PAR/GRAPHS BATCH=YES SAVENAME=GRAPHS
IF ERRORLEVEL 2 GOTO ABORT
:22
IF EXIST %gifdir%SPIRALS.GIF GOTO 23
FRACTINT video=%video% @FRACT19.PAR/SPIRALS BATCH=YES SAVENAME=SPIRALS
IF ERRORLEVEL 2 GOTO ABORT
:23
IF EXIST %gifdir%G-3-03-M.GIF GOTO 24
FRACTINT video=%video% @FRACT19.PAR/G-3-03-M BATCH=YES SAVENAME=G-3-03-M
IF ERRORLEVEL 2 GOTO ABORT
:24
IF EXIST %gifdir%jdsg410.GIF GOTO 25
FRACTINT video=%video% @FRACT19.PAR/jdsg4101 BATCH=YES SAVENAME=jdsg410
IF ERRORLEVEL 2 GOTO ABORT
:25
goto SUCCESS

rem                         +-----------------+
rem                         ! Images for 19.5 !
rem                         +-----------------+

:19_5_1_g
if not %pars%==6 goto 19_6_1_g
IF EXIST %gifdir%PTCMJN01.GIF GOTO 26
FRACTINT video=%video% @FRACT19.PAR/PTCMJN01 BATCH=YES SAVENAME=PTCMJN01
IF ERRORLEVEL 2 GOTO ABORT
:26
IF EXIST %gifdir%PTC4M01.GIF GOTO 27
FRACTINT video=%video% @FRACT19.PAR/PTC4M01 BATCH=YES SAVENAME=PTC4M01
IF ERRORLEVEL 2 GOTO ABORT
:27
goto SUCCESS

rem                         +-----------------+
rem                         ! Images for 19.6 !
rem                         +-----------------+

:19_6_1_g
if not %pars%==7 goto end
IF EXIST %gifdir%EJ_01.GIF GOTO 28
FRACTINT video=%video% @FRACT19.PAR/EJ_01 BATCH=YES SAVENAME=EJ_01
IF ERRORLEVEL 2 GOTO ABORT
:28
IF EXIST %gifdir%oortcld.GIF GOTO 29
FRACTINT video=%video% @FRACT19.PAR/oortcld BATCH=YES SAVENAME=oortcld
IF ERRORLEVEL 2 GOTO ABORT
:29
IF EXIST %gifdir%sg82112.GIF GOTO 30
FRACTINT video=%video% @FRACT19.PAR/sg8-21-12 BATCH=YES SAVENAME=sg82112
IF ERRORLEVEL 2 GOTO ABORT
:30
goto SUCCESS

rem                              +------+
rem                              ! Exit !
rem                              +------+

:viderrmsg
echo.
echo  Oops! I don't know the key assigned to your %video% video mode!
echo.
if %video%==1280 echo  Please, add it to the 3rd line of this file (e.g. set mode_%video%=cf9)
if %video%==1600 echo  Please, add it to the 4th line of this file (e.g. set mode_%video%=cf9)
echo.
pause
goto end

:SUCCESS
cls
echo.
ECHO  FRACT19.BAT successfully completed.
GOTO END

:ABORT
rem cls
echo.
ECHO  FRACT19.BAT aborted!

:END
echo.
echo  Have a nice day!
echo.
pause
