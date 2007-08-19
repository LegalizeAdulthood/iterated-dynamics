#include "Test.h"
#include "TestResult.h"
#include "TestRegistry.h"

void TestRegistry::addTest(Test *test)
{
	instance().add(test);
}

void TestRegistry::runAllTests(TestResult &result)
{
	instance().run(result);
}

TestRegistry &TestRegistry::instance()
{
	static TestRegistry registry;
	return registry;
}

void TestRegistry::add(Test *test)
{
	if (!m_tests)
	{
		m_tests = test;
		return;
	}

	Test *node = m_tests;
	while (node->getNext())
	{
		node = node->getNext();
	}
	node->setNext(test);
}

void TestRegistry::run(TestResult &result)
{
	result.testsStarted();

	for (Test *test = m_tests; test != 0; test = test->getNext())
	{
		test->run(result);
	}
	result.testsEnded();
}
