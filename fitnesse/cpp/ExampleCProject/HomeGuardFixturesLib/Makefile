TARGET = libHomeGuardFixturesLib.a

DEPENDENCY_FILE = .dependencies

DOTO =\
 HomeGuardFixtureMaker.o HomeGuardFixtureMakerTest.o\
 EventLogOutput.o\
 HomeGuardInit.o\
 SetUpHomeGuard.o\
 TearDownHomeGuard.o

INCLUDE =\
 -I../HomeGuardLib\
 -I../include\
 -I../MockHardwareLib\
 -I$(CPP_TEST_TOOLS)/lib\
 -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/Gcc\
 -I.
 
LIBDIR = ../lib
 
include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
