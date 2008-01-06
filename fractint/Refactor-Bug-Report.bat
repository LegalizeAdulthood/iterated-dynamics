@echo off
cd c:\data\id
set dest=Bugs\%1
mkdir "%dest%\common"
mkdir "%dest%\headers"
mkdir "%dest%\help"
mkdir "%dest%\maps"
mkdir "%dest%\test\CppUnitLite"
mkdir "%dest%\test\idTests"
mkdir "%dest%\win32"

copy "ID.HLP" "%dest%" > nul:
copy "id.sln" "%dest%" > nul:
copy "id.vcproj" "%dest%" > nul:
copy "sstools.ini" "%dest%" > nul:
copy "unused.vcproj" "%dest%" > nul:
copy "common\*.cpp" "%dest%\common\" > nul:
copy "headers\*.h" "%dest%\headers\" > nul:
copy "help\*.cpp" "%dest%\help\" > nul:
copy "help\hc.vcproj" "%dest%\help\" > nul:
copy "help\*.src" "%dest%\help\" > nul:
copy "help\HELPDEFS.H" "%dest%\help\" > nul:
copy "help\ID.HLP" "%dest%\help\" > nul:
copy "maps\default.map" "%dest%\maps\" > nul:
copy "test\CppUnitLite\CppUnitLite.vcproj" "%dest%\test\CppUnitLite\" > nul:
copy "test\CppUnitLite\*.cpp" "%dest%\test\CppUnitLite\" > nul:
copy "test\CppUnitLite\*.h" "%dest%\test\CppUnitLite\" > nul:
copy "test\idTests\*.cpp" "%dest%\test\idTests\" > nul:
copy "test\idTests\id-tests.vcproj" "%dest%\test\idTests\" > nul:
copy "test\idTests\*.h" "%dest%\test\idTests\" > nul:
copy "win32\Banner.bmp" "%dest%\win32\" > nul:
copy "win32\*.cpp" "%dest%\win32\" > nul:
copy "win32\*.h" "%dest%\win32\" > nul:
copy "win32\FractInt.ico" "%dest%\win32\" > nul:
copy "win32\fractint.rc" "%dest%\win32\" > nul:
copy "win32\IteratedDynamicsSetup.vdproj" "%dest%\win32\" > nul:
