#include "Platform.h"
#include "Fit/FitnesseServer.h"
#include "IteratedDynamicsFixtureMaker.h"
#include "UnitTestHarness/MemoryLeakWarning.h"

int main(int argc, char* argv[])
{
	MemoryLeakWarning::Enable();
	FixtureMaker *maker = new IteratedDynamicsFixtureMaker();
	int status = FitnesseServer::Main(argc, argv, maker);
	return status;
}
