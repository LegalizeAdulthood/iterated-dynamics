#if !defined(ID_COMPARE_IMAGE_FIXTURE_H)
#define ID_COMPARE_IMAGE_FIXTURE_H

#include <string>
#include "Fit/ColumnFixture.h"

class GIFImage;

namespace
{
	inline void Convert(const bool &in, std::string &out)
	{
		out = in ? "true" : "false";
	}
}

class idCompareImageFixture : public ColumnFixture
{
public:
	idCompareImageFixture();
	virtual ~idCompareImageFixture();
	virtual void reset();

private:
	std::string file;
	std::string parameterSet;
	bool match();
	bool matchSize();
	bool matchColors();
	int pixelDifference();
	int pixelDifferenceCount();
	GIFImage *_reference;
	GIFImage *_test;
	std::string _tempFile;
	int lastMatchingColor() { return _lastMatchingColor; }
	int _lastMatchingColor;
	std::string mismatchedColor() { return _mismatchedColor; }
	std::string _mismatchedColor;

	bool Prepare();
	bool RunFractInt();
	bool CompareImages();
};

#endif