#include "Fit/Fit.h"
#include "Fit/Summary.h"
#include "Fit/ResolutionException.h"
#include "IteratedDynamicsFixtureMaker.h"
#include "VerifyParameterSetFixture.h"

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

IteratedDynamicsFixtureMaker::IteratedDynamicsFixtureMaker()
{
}

IteratedDynamicsFixtureMaker::~IteratedDynamicsFixtureMaker()
{
}

template <typename T>
Fixture *CreateFixture()
{
	return new T;
}

typedef Fixture *CreateProc();
struct NamedCreator
{
	char const *name;
	CreateProc *creator;
};

Fixture *IteratedDynamicsFixtureMaker::make(std::string const &fullName)
{
	std::string name = fullName;
	std::string libraryName = splitName(fullName).first;
	if (libraryName != fullName)
	{
		name = splitName(fullName).second;
	}

#define NAMED_CREATOR(name_) { #name_, CreateFixture<name_> }
	static NamedCreator creators[] =
	{
		NAMED_CREATOR(Fixture),
		NAMED_CREATOR(ColumnFixture),
		NAMED_CREATOR(ActionFixture),
		NAMED_CREATOR(PrimitiveFixture),
		NAMED_CREATOR(Summary),
		NAMED_CREATOR(VerifyParameterSetFixture)
	};

	for (int i = 0; i < NUM_OF(creators); i++)
	{
		if (creators[i].name == name)
		{
			return creators[i].creator();
		}
	}

	throw ResolutionException(name);

	return 0;
}
