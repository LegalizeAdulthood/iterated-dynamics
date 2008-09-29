#include "stdafx.h"
#include "Platform.h"
#include "Fit/FitnesseServer.h"
#include "idFixtureMaker.h"
#include "UnitTestHarness/MemoryLeakWarning.h"

int main(int argc, char *argv[])
{
	MemoryLeakWarning::Enable();
	FixtureMaker* maker = new idFixtureMaker();
	return FitnesseServer::Main(argc, argv, maker);
}
