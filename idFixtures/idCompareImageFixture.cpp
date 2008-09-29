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
	_tempFile("tmp.gif")
{
	PUBLISH(idCompareImageFixture, std::string, file);
	PUBLISH(idCompareImageFixture, std::string, parameterSet);
	PUBLISH(idCompareImageFixture, std::string, match);
	PUBLISH(idCompareImageFixture, std::string, matchSize);
	PUBLISH(idCompareImageFixture, std::string, matchColors);
	PUBLISH(idCompareImageFixture, std::string, matchPixels);
}

idCompareImageFixture::~idCompareImageFixture()
{
	delete _reference;
	_reference = 0;
	delete _test;
	_test = 0;
}

std::string ToString(bool value)
{
	return value ? "true" : "false";
}

std::string idCompareImageFixture::match()
{
	return ToString(ResultsMatchReference());
}
std::string idCompareImageFixture::matchSize()
{
	return ToString(ResultsMatchReferenceSize());
}
std::string idCompareImageFixture::matchColors()
{
	return ToString(ResultsMatchReferenceColors());
}
std::string idCompareImageFixture::matchPixels()
{
	return ToString(ResultsMatchReferencePixels());
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

bool idCompareImageFixture::ResultsMatchReference()
{
	return ResultsMatchReferenceSize() && ResultsMatchReferenceColors() && ResultsMatchReferencePixels();
}

bool idCompareImageFixture::ResultsMatchReferenceSize()
{
	Prepare();
	return _reference->SameSize(*_test);
}

bool idCompareImageFixture::ResultsMatchReferenceColors()
{
	Prepare();
	return _reference->SameColors(*_test);
}

bool idCompareImageFixture::ResultsMatchReferencePixels()
{
	Prepare();
	return _reference->SamePixels(*_test);
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

	boost::format command("..\\fractint\\debug\\fractint.exe @%1%/%2% batch=yes savename=%3% video=F7");
	command % file % parameterSet % _tempFile;
	return 0 == FileSystem::ExecuteCommand(command.str());
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
