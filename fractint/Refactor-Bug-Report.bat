@echo off
cd c:\data\id
set dest=Bugs\%1
mkdir "%dest%"
mkdir "%dest%\src"
mkdir "%dest%\src\common"
mkdir "%dest%\src\headers"
mkdir "%dest%\src\help"
mkdir "%dest%\maps"
mkdir "%dest%\src\test\CppUnitLite"
mkdir "%dest%\src\test\idTests"
mkdir "%dest%\src\win32"

copy "src\ID.HLP" "%dest%\src" > nul:
copy "id.sln" "%dest%" > nul:
copy "src\id.vcproj" "%dest%\src" > nul:
copy "src\sstools.ini" "%dest%\src" > nul:
copy "src\unused.vcproj" "%dest%\src" > nul:
copy "src\common\*.cpp" "%dest%\src\common\" > nul:
copy "src\headers\*.h" "%dest%\src\headers\" > nul:
copy "src\help\*.cpp" "%dest%\src\help\" > nul:
copy "src\help\hc.vcproj" "%dest%\src\help\" > nul:
copy "src\help\*.src" "%dest%\src\help\" > nul:
copy "src\help\HELPDEFS.H" "%dest%\src\help\" > nul:
copy "src\help\ID.HLP" "%dest%\src\help\" > nul:
copy "src\maps\default.map" "%dest%\src\maps\" > nul:
copy "src\test\CppUnitLite\CppUnitLite.vcproj" "%dest%\src\test\CppUnitLite\" > nul:
copy "src\test\CppUnitLite\*.cpp" "%dest%\src\test\CppUnitLite\" > nul:
copy "src\test\CppUnitLite\*.h" "%dest%\src\test\CppUnitLite\" > nul:
copy "src\test\idTests\*.cpp" "%dest%\src\test\idTests\" > nul:
copy "src\test\idTests\id-tests.vcproj" "%dest%\src\test\idTests\" > nul:
copy "src\test\idTests\*.h" "%dest%\src\test\idTests\" > nul:
copy "src\win32\Banner.bmp" "%dest%\src\win32\" > nul:
copy "src\win32\*.cpp" "%dest%\src\win32\" > nul:
copy "src\win32\*.h" "%dest%\src\win32\" > nul:
copy "src\win32\FractInt.ico" "%dest%\src\win32\" > nul:
copy "src\win32\fractint.rc" "%dest%\src\win32\" > nul:
copy "src\win32\IteratedDynamicsSetup.vdproj" "%dest%\src\win32\" > nul:
