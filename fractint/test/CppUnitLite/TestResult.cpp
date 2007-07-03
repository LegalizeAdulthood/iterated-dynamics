#include "TestResult.h"
#include "Failure.h"

#include <stdio.h>

TestResult::TestResult()
	: failureCount(0)
{
}


void TestResult::testsStarted()
{
}

void TestResult::addFailure(const Failure &failure)
{
	fprintf(stdout, "Failure: \"%s\" line %ld in %s\n",
		failure.message.asCharString(),
		failure.lineNumber,
		failure.fileName.asCharString());

	failureCount++;
}


void TestResult::testsEnded()
{
	if (failureCount > 0)
	{
		fprintf(stdout, "There were %ld failures\n", failureCount);
	}
	else
	{
		fprintf(stdout, "There were no test failures\n");
	}
}
