# Microsoft Developer Studio Project File - Name="HomeGuardFixturesLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=HomeGuardFixturesLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HomeGuardFixturesLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HomeGuardFixturesLib.mak" CFG="HomeGuardFixturesLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HomeGuardFixturesLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "HomeGuardFixturesLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HomeGuardFixturesLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "HomeGuardFixturesLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\MockHardwareLib" /I "../HomeGuardLib" /I "../include" /I "$(CPP_TEST_TOOLS)" /I "$(CPP_TEST_TOOLS)\Platforms\VisualCpp" /D "_LIB" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "HomeGuardFixturesLib - Win32 Release"
# Name "HomeGuardFixturesLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\EventLogOutput.cpp
# End Source File
# Begin Source File

SOURCE=.\HomeGuardContext.cpp
# End Source File
# Begin Source File

SOURCE=.\HomeGuardFixtureMaker.cpp
# End Source File
# Begin Source File

SOURCE=.\HomeGuardFixtureMakerTest.cpp
# End Source File
# Begin Source File

SOURCE=.\SetUpHomeGuard.cpp
# End Source File
# Begin Source File

SOURCE=.\SetUpTearDownTest.cpp
# End Source File
# Begin Source File

SOURCE=.\TearDownHomeGuard.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CheckFrontPanel.h
# End Source File
# Begin Source File

SOURCE=.\CheckPhoneLine.h
# End Source File
# Begin Source File

SOURCE=.\Control.h
# End Source File
# Begin Source File

SOURCE=.\DefineSensors.h
# End Source File
# Begin Source File

SOURCE=.\EventLogOutput.h
# End Source File
# Begin Source File

SOURCE=.\FrontPanelController.h
# End Source File
# Begin Source File

SOURCE=.\HomeGuardContext.h
# End Source File
# Begin Source File

SOURCE=.\HomeGuardFixtureMaker.h
# End Source File
# Begin Source File

SOURCE=.\PanelStateChanges.h
# End Source File
# Begin Source File

SOURCE=.\SetUpHomeGuard.h
# End Source File
# Begin Source File

SOURCE=.\TearDownHomeGuard.h
# End Source File
# End Group
# End Target
# End Project
