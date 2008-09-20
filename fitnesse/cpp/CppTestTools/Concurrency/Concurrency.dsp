# Microsoft Developer Studio Project File - Name="Concurrency" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Concurrency - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Concurrency.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Concurrency.mak" CFG="Concurrency - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Concurrency - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Concurrency - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Concurrency - Win32 Release"

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

!ELSEIF  "$(CFG)" == "Concurrency - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "$(CPP_TEST_TOOLS)" /I "$(CPP_TEST_TOOLS)/Platforms/VisualCpp" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fr /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\*.lib ..\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Concurrency - Win32 Release"
# Name "Concurrency - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Action.cpp
# End Source File
# Begin Source File

SOURCE=.\ActionExecutingRunnable.h
# End Source File
# Begin Source File

SOURCE=.\ActiveObject.cpp
# End Source File
# Begin Source File

SOURCE=.\ActiveObjectTest.cpp
# End Source File
# Begin Source File

SOURCE=.\ClientSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\DelayTest.cpp
# End Source File
# Begin Source File

SOURCE=.\EventSettingActionTest.cpp
# End Source File
# Begin Source File

SOURCE=.\EventTest.cpp
# End Source File
# Begin Source File

SOURCE=.\FlagSettingActionTest.cpp
# End Source File
# Begin Source File

SOURCE=.\MutexTest.cpp
# End Source File
# Begin Source File

SOURCE=.\NullAction.cpp
# End Source File
# Begin Source File

SOURCE=.\Runnable.cpp
# End Source File
# Begin Source File

SOURCE=.\ScopeLockTest.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketInnards.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketTest.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadTest.cpp
# End Source File
# Begin Source File

SOURCE=.\Timer.cpp
# End Source File
# Begin Source File

SOURCE=.\TimerTest.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Action.h
# End Source File
# Begin Source File

SOURCE=.\ActiveObject.h
# End Source File
# Begin Source File

SOURCE=.\ClientSocket.h
# End Source File
# Begin Source File

SOURCE=.\ConcurrencyTests.h
# End Source File
# Begin Source File

SOURCE=.\Delay.h
# End Source File
# Begin Source File

SOURCE=.\Event.h
# End Source File
# Begin Source File

SOURCE=.\EventSettingAction.h
# End Source File
# Begin Source File

SOURCE=.\FlagSettingAction.h
# End Source File
# Begin Source File

SOURCE=.\Mutex.h
# End Source File
# Begin Source File

SOURCE=.\NullAction.h
# End Source File
# Begin Source File

SOURCE=.\Runnable.h
# End Source File
# Begin Source File

SOURCE=.\ScopeLock.h
# End Source File
# Begin Source File

SOURCE=.\ServerSocket.h
# End Source File
# Begin Source File

SOURCE=.\Socket.h
# End Source File
# Begin Source File

SOURCE=.\SocketException.h
# End Source File
# Begin Source File

SOURCE=.\SocketInnards.h
# End Source File
# Begin Source File

SOURCE=.\Thread.h
# End Source File
# Begin Source File

SOURCE=.\Timer.h
# End Source File
# Begin Source File

SOURCE=.\WaitingAction.h
# End Source File
# End Group
# End Target
# End Project
