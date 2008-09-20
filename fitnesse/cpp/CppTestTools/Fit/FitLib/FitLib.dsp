# Microsoft Developer Studio Project File - Name="FitLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=FitLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FitLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FitLib.mak" CFG="FitLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FitLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "FitLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FitLib - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../Platform" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "FitLib - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\..\Platforms\VisualCpp" /I ".." /I "$(CPP_TEST_TOOLS)" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\*.lib ..\..\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "FitLib - Win32 Release"
# Name "FitLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ActionFixture.cpp
# End Source File
# Begin Source File

SOURCE=.\ActionFixtureTest.cpp
# End Source File
# Begin Source File

SOURCE=.\ColumnFixture.cpp
# End Source File
# Begin Source File

SOURCE=.\FitFixtureMaker.cpp
# End Source File
# Begin Source File

SOURCE=.\FitFixtureMakerTest.cpp
# End Source File
# Begin Source File

SOURCE=.\FitnesseServer.cpp
# End Source File
# Begin Source File

SOURCE=.\FitnesseServerTest.cpp
# End Source File
# Begin Source File

SOURCE=.\Fixture.cpp
# End Source File
# Begin Source File

SOURCE=.\FixtureMaker.cpp
# End Source File
# Begin Source File

SOURCE=.\FixtureMakerTest.cpp
# End Source File
# Begin Source File

SOURCE=.\FixtureTest.cpp
# End Source File
# Begin Source File

SOURCE=.\FrameworkTest.cpp
# End Source File
# Begin Source File

SOURCE=.\Parse.cpp
# End Source File
# Begin Source File

SOURCE=.\PrimitiveFixture.cpp
# End Source File
# Begin Source File

SOURCE=.\RowFixture.cpp
# End Source File
# Begin Source File

SOURCE=.\RowFixtureTest.cpp
# End Source File
# Begin Source File

SOURCE=.\Summary.cpp
# End Source File
# Begin Source File

SOURCE=.\SummaryTest.cpp
# End Source File
# Begin Source File

SOURCE=.\TestFixtureMaker.cpp
# End Source File
# Begin Source File

SOURCE=.\TestRowFixture.cpp
# End Source File
# Begin Source File

SOURCE=.\TypeAdapter.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\ActionFixture.h
# End Source File
# Begin Source File

SOURCE=..\AppFixtureMaker.h
# End Source File
# Begin Source File

SOURCE=..\ColumnFixture.h
# End Source File
# Begin Source File

SOURCE=..\CommandAdapter.h
# End Source File
# Begin Source File

SOURCE=..\DomainObjectWrapper.h
# End Source File
# Begin Source File

SOURCE=..\EnterAdapter.h
# End Source File
# Begin Source File

SOURCE=..\Fit.h
# End Source File
# Begin Source File

SOURCE=.\FitFixtureMaker.h
# End Source File
# Begin Source File

SOURCE=..\FitnesseServer.h
# End Source File
# Begin Source File

SOURCE=..\FitUnitTests.h
# End Source File
# Begin Source File

SOURCE=..\Fixture.h
# End Source File
# Begin Source File

SOURCE=..\FixtureMaker.h
# End Source File
# Begin Source File

SOURCE=..\HurlingPointer.h
# End Source File
# Begin Source File

SOURCE=..\IllegalAccessException.h
# End Source File
# Begin Source File

SOURCE=..\MemberAdapter.h
# End Source File
# Begin Source File

SOURCE=..\Parse.h
# End Source File
# Begin Source File

SOURCE=..\ParseException.h
# End Source File
# Begin Source File

SOURCE=..\PrimitiveFixture.h
# End Source File
# Begin Source File

SOURCE=..\ResolutionException.h
# End Source File
# Begin Source File

SOURCE=..\RowFixture.h
# End Source File
# Begin Source File

SOURCE=..\RowMap.h
# End Source File
# Begin Source File

SOURCE=..\Summary.h
# End Source File
# Begin Source File

SOURCE=..\TypeAdapter.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
