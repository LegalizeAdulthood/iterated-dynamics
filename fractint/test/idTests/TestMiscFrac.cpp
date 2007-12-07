#include "stdafx.h"

#include "miscfrac.h"
#include "id.h"

TEST(precision_format, miscfrac)
{
	std::string format = precision_format("g", 10);
	CHECK_EQUAL("%.10g", format);
}
