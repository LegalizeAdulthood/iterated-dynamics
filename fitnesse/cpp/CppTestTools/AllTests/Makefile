TARGET = AllTests
CPP_TEST_TOOLS = ..
CPP_PLATFORM = Gcc

LOADLIBS =\
 -lFitLib\
 -lUnitTestHarness\
 -lConcurrency\
 -lPlatform\
 -lHelpers\
 -lstdc++\
 -lpthread

# -lConcurrency\

LIBDIR = $(CPP_TEST_TOOLS)/lib

LIBDIRS =\
 -L$(LIBDIR) 

INCLUDE =\
 -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM)\
 -I. 

DOTO =\
   
include $(CPP_TEST_TOOLS)/MakefileHelpers/TestExeMakefile

$(TARGET): $(TARGET).o $(DOTO)\
		$(LIBDIR)/libConcurrency.a\
		$(LIBDIR)/libUnitTestHarness.a\
		$(LIBDIR)/libFitLib.a\
		$(LIBDIR)/libHelpers.a\
		$(LIBDIR)/libPlatform.a
	$(CXX) -o $(TARGET) $(LIBDIRS) $^ $(LOADLIBS) 
