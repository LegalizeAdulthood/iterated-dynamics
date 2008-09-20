TARGET = AllTests

DEPENDENCY_FILE = .dependencies

LOADLIBS =\
 -lHomeGuardLib\
 -lMockHardwareLib\
 -lHomeGuardLib\
 -lHomeGuardFixturesLib\
 -lFitLib\
 -lUnitTestHarness\
 -lHelpers\
 -lPlatform\
 -lstdc++\
 -lpthread

LIBDIR = ../lib

LIBDIRS =\
 -L$(CPP_TEST_TOOLS)/lib\
 -L$(LIBDIR)

INCLUDE = -I$(CPP_TEST_TOOLS)\
 -I.

DOTO = 

include $(CPP_TEST_TOOLS)/MakefileHelpers/TestExeMakefile

$(TARGET): $(TARGET).o $(DOTO)\
		$(LIBDIR)/libHomeGuardLib.a\
		$(LIBDIR)/libHomeGuardFixturesLib.a\
		$(CPP_TEST_TOOLS)/lib/libConcurrency.a\
		$(CPP_TEST_TOOLS)/lib/libFitLib.a\
		$(CPP_TEST_TOOLS)/lib/libPlatform.a
	$(CXX) -o $(TARGET) $(LIBDIRS) $^ $(LOADLIBS)

	