#include "stdafx.h"

#include <sstream>

#include "Endian.h"

TEST(Endian, LittleEndianTrue)
{
	LittleEndian<true> endian;
	std::ostringstream stream;
	int value = 0x01020304;
	endian.write(stream, value);
	std::string &output = stream.str();
	CHECK_EQUAL(0x04, output[0]);
	CHECK_EQUAL(0x03, output[1]);
	CHECK_EQUAL(0x02, output[2]);
	CHECK_EQUAL(0x01, output[3]);
}
