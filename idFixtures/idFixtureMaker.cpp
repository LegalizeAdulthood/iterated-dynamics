#include "stdafx.h"
#include "Fit/Fit.h"
#include "Fit/Summary.h"
#include "Fit/ResolutionException.h"
#include "idFixtureMaker.h"
#include "idCompareImageFixture.h"

Fixture *idFixtureMaker::make(const std::string &fullName)
{
    string name = fullName;
	  string libraryName = splitName(fullName).first;
    if (libraryName != fullName)
        name = splitName(fullName).second;

    PUBLISH_FIXTURE(Fixture);
    PUBLISH_FIXTURE(ColumnFixture);
    PUBLISH_FIXTURE(ActionFixture)
    PUBLISH_FIXTURE(PrimitiveFixture);
    PUBLISH_FIXTURE(Summary);

	if (name == "CompareImage")
	{
		return new idCompareImageFixture;
	}

	throw ResolutionException(name);

    return 0;
}
