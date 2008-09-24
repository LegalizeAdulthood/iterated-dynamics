#if !defined(VERIFY_PARAMETER_SET_FIXTURE_H)
#define VERIFY_PARAMETER_SET_FIXTURE_H

#include "Fit/ColumnFixture.h"

class VerifyParameterSetFixture : public ColumnFixture
{
public:
	VerifyParameterSetFixture() : ColumnFixture()
	{
		PUBLISH(VerifyParameterSetFixture, std::string, parameterFile);
		PUBLISH(VerifyParameterSetFixture, std::string, parameterSet);
		PUBLISH(VerifyParameterSetFixture, std::string, referenceImageFile);
		PUBLISH(VerifyParameterSetFixture, std::string, equal);
	}

	std::string equal();

private:
	std::string parameterFile;
	std::string parameterSet;
	std::string referenceImageFile;
};

#endif
