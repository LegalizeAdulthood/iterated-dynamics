TARGET = libUnitTestHarness.a
CPP_TEST_TOOLS = ..
CPP_PLATFORM = Gcc

DOTO = \
 NullTest.o NullTestTest.o\
 TestInstaller.o TestInstallerTest.o\
 TestOutput.o TestOutputTest.o\
 MemoryLeakWarningTest.o\
 CommandLineTestRunner.o\
 EqualsFailure.o\
 Failure.o FailureTest.o\
 SimpleString.o SimpleStringTest.o\
 Test.o TestTest.o\
 TestRegistry.o\
 TestResult.o

INCLUDE =\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM)\
 -I$(CPP_TEST_TOOLS)\
 -I.

LIBDIR = $(CPP_TEST_TOOLS)/lib

include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
