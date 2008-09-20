TARGET = HomeGuardFitnesseServer
CPP_PLATFORM = Gcc

DEPENDENCY_FILE = .dependencies

LOADLIBS =\
 -lHomeGuardFixturesLib\
 -lHomeGuardLib\
 -lMockHardwareLib\
 -lHomeGuardLib\
 -lUtilLib\
 -lFitLib\
 -lConcurrency\
 -lPlatform\
 -lstdc++\
 -lpthread
 

LIBDIR = ../lib
TEST_DIR=../../test

LIBDIRS =\
 -L$(CPP_TEST_TOOLS)/lib\
 -L$(LIBDIR)

INCLUDE =\
 -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM) \
 -I.

DOTO = $(TARGET).o

include $(CPP_TEST_TOOLS)/MakefileHelpers/ExeMakefile

$(TARGET) :  $(DOTO)\
		$(LIBDIR)/libHomeGuardLib.a\
		$(LIBDIR)/libHomeGuardFixturesLib.a\
		$(CPP_TEST_TOOLS)/lib/libConcurrency.a\
		$(CPP_TEST_TOOLS)/lib/libFitLib.a\
		$(CPP_TEST_TOOLS)/lib/libPlatform.a
	$(CXX) -o $(TARGET) $(LIBDIRS) $^ $(LOADLIBS) 
	test -f $(TEST_DIR) || mkdir -p $(TEST_DIR)
	cp -f $(TARGET) $(TEST_DIR)

