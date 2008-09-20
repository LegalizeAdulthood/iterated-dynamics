TARGET = AllTests
CPP_TEST_TOOLS = ..
CPP_PLATFORM = Gcc

LOADLIBS =\
 -lFitLib\
 -lUnitTestHarness\
 -lGccPlatform\
 -lHelpers\
 -lstdc++ 

LIBDIR = $(CPP_TEST_TOOLS)/lib

LIBDIRS =\
 -L$(LIBDIR) 

INCLUDE =\
 -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP+PLATFORM)\
 -I. 

DOTO =\
  
include $(CPP_TEST_TOOLS)/MakefileHelpers/ExeMakefile

$(TARGET): $(TARGET).o $(DOTO)\
		$(LIBDIR)/libUnitTestHarness.a\
		$(LIBDIR)/libHelpers.a\
		$(LIBDIR)/libGccPlatform.a
	$(CC) -o $(TARGET) $(LIBDIRS) $^ $(LOADLIBS) 
