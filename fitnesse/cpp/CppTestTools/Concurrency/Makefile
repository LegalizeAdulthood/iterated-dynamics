TARGET = libConcurrency.a
CPP_TEST_TOOLS = ..
CPP_PLATFORM = Gcc

DOTO = \
 Action.o\
 ActiveObject.o\
 ActiveObjectTest.o\
 DelayTest.o\
 EventTest.o\
 FlagSettingActionTest.o\
 EventSettingActionTest.o\
 MutexTest.o\
 NullAction.o\
 Runnable.o\
 ScopeLockTest.o\
 SocketTest.o\
 ThreadTest.o\
 Timer.o\
 TimerTest.o\
 ServerSocket.o\
 ClientSocket.o\
 SocketInnards.o


INCLUDE = -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM)\
 -I.

LIBDIR = $(CPP_TEST_TOOLS)/lib

include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
