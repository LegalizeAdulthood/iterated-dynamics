TARGET = libHomeGuardFixturesLib.a
CPP_PLATFORM = Gcc

DEPENDENCY_FILE = .dependencies

DOTO =\
 HomeGuardFixtureMaker.o HomeGuardFixtureMakerTest.o\
 EventLogOutput.o\
 HomeGuardContext.o\
 SetUpHomeGuard.o\
 TearDownHomeGuard.o

INCLUDE =\
 -I../HomeGuardLib\
 -I../include\
 -I../MockHardwareLib\
 -I$(CPP_TEST_TOOLS)/lib\
 -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM)\
 -I.
 
LIBDIR = ../lib
 
include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
