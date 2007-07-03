#include "Failure.h"

#include <stdio.h>
#include <string.h>

Failure::Failure(const SimpleString &theTestName, const SimpleString &theFileName, long theLineNumber,
		          const SimpleString &theCondition)
	: message(theCondition),
	testName(theTestName),
	fileName(theFileName),
	lineNumber(theLineNumber)
{
}

Failure::Failure(const SimpleString &theTestName, const SimpleString &theFileName, long theLineNumber,
				  const SimpleString &expectedValue, const SimpleString &actualValue)
	: testName(theTestName),
	fileName(theFileName),
	lineNumber(theLineNumber)
{
	const char *expected = "expected ";
	const char *butWas = " but was: ";

	char *stage = new char[strlen(expected) + expectedValue.size() +
		strlen(butWas) + actualValue.size() + 1];

	sprintf(stage, "%s%s%s%s",
		expected, expectedValue.asCharString(), butWas, actualValue.asCharString());

	message = SimpleString(stage);

	delete[] stage;
}


