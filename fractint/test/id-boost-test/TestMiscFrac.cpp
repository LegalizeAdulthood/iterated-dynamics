#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "miscfrac.h"
#include "id.h"

BOOST_AUTO_TEST_CASE(miscfrac_precision_format)
{
	std::string format = precision_format("g", 10);
	BOOST_CHECK_EQUAL("%.10g", format);
}
