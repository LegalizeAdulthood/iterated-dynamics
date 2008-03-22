#include "stdafx.h"

#include <sstream>
#include <boost/test/unit_test.hpp>
#include "Endian.h"

BOOST_AUTO_TEST_CASE(Endian_LittleEndianTrue)
{
	LittleEndian<true> endian;
	std::ostringstream stream;
	int value = 0x01020304;
	endian.write(stream, value);
	std::string &output = stream.str();
	BOOST_CHECK_EQUAL(0x04, output[0]);
	BOOST_CHECK_EQUAL(0x03, output[1]);
	BOOST_CHECK_EQUAL(0x02, output[2]);
	BOOST_CHECK_EQUAL(0x01, output[3]);
}
