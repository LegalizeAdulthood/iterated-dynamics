TARGET = libFitLib.a
CPP_TEST_TOOLS = ../..
CPP_PLATFORM = Gcc

DOTO = \
 FitFixtureMaker.o FitFixtureMakerTest.o\
 ActionFixture.o\
 ActionFixtureTest.o\
 ColumnFixture.o\
 FitnesseServer.o\
 FitnesseServerTest.o\
 Fixture.o\
 FixtureMaker.o\
 FixtureMakerTest.o\
 FixtureTest.o\
 FrameworkTest.o\
 Parse.o\
 PrimitiveFixture.o\
 RowFixture.o\
 RowFixtureTest.o\
 Summary.o\
 SummaryTest.o\
 TestFixtureMaker.o\
 TestRowFixture.o\
 TypeAdapter.o

INCLUDE =\
 -I..\
 -I$(CPP_TEST_TOOLS)\
 -I$(CPP_TEST_TOOLS)/Platforms/$(CPP_PLATFORM)\
 -I.

LIBDIR = $(CPP_TEST_TOOLS)/lib

include $(CPP_TEST_TOOLS)/MakefileHelpers/StaticLibMakefile
