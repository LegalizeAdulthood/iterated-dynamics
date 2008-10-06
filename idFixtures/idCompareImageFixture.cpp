#include "stdafx.h"
#include <boost/format.hpp>

#include "idCompareImageFixture.h"
#include "FileSystem.h"
#include "GIFImage.h"

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

idCompareImageFixture::idCompareImageFixture()
	: ColumnFixture(),
	file(),
	parameterSet(),
	_reference(0),
	_test(0),
	_tempFile("fitnesse.gif"),
	_lastMatchingColor(0)
{
	PUBLISH(idCompareImageFixture, std::string, file);
	PUBLISH(idCompareImageFixture, std::string, parameterSet);
	PUBLISH(idCompareImageFixture, bool, match);
	PUBLISH(idCompareImageFixture, bool, matchSize);
	PUBLISH(idCompareImageFixture, bool, matchColors);
	PUBLISH(idCompareImageFixture, int, pixelDifference);
	PUBLISH(idCompareImageFixture, int, pixelDifferenceCount);
	PUBLISH(idCompareImageFixture, int, lastMatchingColor);
	PUBLISH(idCompareImageFixture, std::string, mismatchedColor);
}

idCompareImageFixture::~idCompareImageFixture()
{
	delete _reference;
	_reference = 0;
	delete _test;
	_test = 0;
}

std::string EscapeChars(std::string const &source)
{
	std::string dest = source;
	char const *badChars = "\\/:*?\"<>|";
	for (std::string::size_type pos = dest.find_first_of(badChars);
		pos != std::string::npos;
		pos = dest.find_first_of(badChars))
	{
		std::string escaped = (boost::format("%02x") % int(dest[pos])).str();
		dest.replace(pos, 1, escaped);
	}
	return dest;
}

bool idCompareImageFixture::match()
{
	return matchSize() && matchColors() && 0 == pixelDifference();
}

bool idCompareImageFixture::matchSize()
{
	Prepare();
	return _reference->SameSize(*_test);
}

bool idCompareImageFixture::matchColors()
{
	Prepare();
	return _reference->SameColors(*_test, _lastMatchingColor, _mismatchedColor);
}

int idCompareImageFixture::pixelDifference()
{
	Prepare();
	return _reference->PixelDifference(*_test);
}

int idCompareImageFixture::pixelDifferenceCount()
{
	Prepare();
	return _reference->PixelDifferenceCount(*_test);
}

bool idCompareImageFixture::Prepare()
{
	if (_reference && _test)
	{
		return true;
	}

	if (file.length() == 0 || parameterSet.length() == 0)
	{
		return false;
	}
	if (!RunFractInt())
	{
		return false;
	}
	string referenceImage = (boost::format("FitNesseRoot\\files\\reference\\fractint-20.04p08\\%1%\\%2%.gif")
		% file % EscapeChars(parameterSet)).str();
	delete _reference;
	_reference = new GIFImage(referenceImage);
	delete _test;
	_test = new GIFImage("..\\fractint\\" + _tempFile);

	return true;
}

bool idCompareImageFixture::RunFractInt()
{
	CurrentDirectoryPusher currentDirectory("..\\fractint");

	std::string command =
		(boost::format("..\\fractint\\debug\\fractint.exe @%1%/%2% batch=yes overwrite=yes savename=%3% video=F7")
			% file % parameterSet % _tempFile).str();
	return 0 == FileSystem::ExecuteCommand(command);
}

bool idCompareImageFixture::CompareImages()
{
	return *_reference == *_test;
}

void idCompareImageFixture::reset()
{
	delete _reference;
	_reference = 0;
	delete _test;
	_test = 0;
}
