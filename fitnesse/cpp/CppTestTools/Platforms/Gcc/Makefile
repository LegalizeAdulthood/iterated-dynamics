TARGET = libPlatform.a
CPP_TEST_TOOLS = ../..
CPP_PLATFORM = Gcc

DOTO = \
 Delay.o\
 Event.o\
 GccMemoryLeakWarning.o\
 Mutex.o\
 Thread.o\

INCLUDE =\
 -I$(CPP_TEST_TOOLS)\
 -I.

LIBDIR = $(CPP_TEST_TOOLS)/lib

include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
