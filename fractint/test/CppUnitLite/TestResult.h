#ifndef TESTRESULT_H
#define TESTRESULT_H
///////////////////////////////////////////////////////////////////////////////
//
// TESTRESULT.H
//
// A TestResult is a collection of the history of some test runs.  Right now
// it just collects failures.
//
///////////////////////////////////////////////////////////////////////////////
class Failure;

class TestResult
{
public:
	TestResult();
	virtual void testsStarted();
	virtual void addFailure(const Failure &failure);
	virtual void testsEnded();

private:
	int failureCount;
};

#endif
