#ifndef TEST_H
#define TEST_H
///////////////////////////////////////////////////////////////////////////////
//
// TEST.H
//
// This file contains the Test class along with the macros which make effective
// in the harness.
//
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include "SimpleString.h"

class TestResult;

class Test
{
public:
	Test(const SimpleString &testName);

	virtual void run(TestResult &result) = 0;
	void setNext(Test *test);
	Test *getNext() const;

protected:
	bool check(long expected, long actual, TestResult &result,
		const SimpleString &fileName, long lineNumber);
	bool check(const SimpleString &expected, const SimpleString &actual, TestResult &result,
		const SimpleString &fileName, long lineNumber);
	SimpleString name_;
	Test *next_;
};

#define TEST(testName, testGroup)								\
	class testGroup##testName##Test : public Test				\
	{															\
	public:														\
		testGroup##testName##Test() : Test(#testName "Test") {}	\
		void run(TestResult &result_);							\
	}															\
    testGroup##testName##Instance;								\
	void testGroup##testName##Test::run(TestResult &result_)

#define CHECK(condition)														\
	do																			\
	{																			\
		if (!(condition))														\
		{																		\
			result_.addFailure(Failure(name_, __FILE__,__LINE__, #condition));	\
			return;																\
		}																		\
	}																			\
	while (0)

#define CHECK_EQUAL(expected,actual)							\
	do															\
	{															\
		if ((expected) == (actual))								\
		{														\
			return;												\
		}														\
		result_.addFailure(Failure(name_, __FILE__, __LINE__,	\
			StringFrom(expected), StringFrom(actual)));			\
	}															\
	while (0)

#define LONGS_EQUAL(expected,actual)								\
	do																\
	{																\
		long actualTemp = actual;									\
		long expectedTemp = expected;								\
		if ((expectedTemp) != (actualTemp))							\
		{															\
			result_.addFailure(Failure(name_, __FILE__, __LINE__,	\
				StringFrom(expectedTemp), StringFrom(actualTemp)));	\
			return;													\
		}															\
	}																\
	while (0)

#define DOUBLES_EQUAL(expected,actual,threshold)										\
	do																					\
	{																					\
		double actualTemp = actual;														\
		double expectedTemp = expected;													\
		if (fabs((expectedTemp)-(actualTemp)) > threshold)								\
		{																				\
			result_.addFailure(Failure(name_, __FILE__, __LINE__,						\
				StringFrom((double) expectedTemp), StringFrom((double) actualTemp)));	\
			return;																		\
		}																				\
	}																					\
	while (0)

#define FAIL(text)														\
	do																	\
	{																	\
		result_.addFailure(Failure(name_, __FILE__, __LINE__, (text)));	\
		return;															\
	}																	\
	while (0)

#endif
