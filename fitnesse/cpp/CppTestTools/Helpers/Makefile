TARGET = libHelpers.a
CPP_TEST_TOOLS = ..
CPP_PLATFORM = Gcc

DOTO = \
 DeterministicHeap.o DeterministicHeapTest.o\
 SimpleStringExtensions.o SimpleStringExtensionsTest.o
 
INCLUDE =\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM)\
 -I$(CPP_TEST_TOOLS)\
 -I.

LIBDIR = $(CPP_TEST_TOOLS)/lib

include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
