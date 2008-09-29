#if !defined(ID_COMPARE_IMAGE_FIXTURE_H)
#define ID_COMPARE_IMAGE_FIXTURE_H

#include <string>
#include "Fit/ColumnFixture.h"

class GIFImage;

class idCompareImageFixture : public ColumnFixture
{
public:
	idCompareImageFixture();
	virtual ~idCompareImageFixture();
	virtual void reset();

private:
	std::string file;
	std::string parameterSet;
	std::string match();
	std::string matchSize();
	std::string matchColors();
	std::string matchPixels();
	GIFImage *_reference;
	GIFImage *_test;
	std::string _tempFile;

	bool Prepare();
	bool ResultsMatchReference();
	bool ResultsMatchReferenceSize();
	bool ResultsMatchReferenceColors();
	bool ResultsMatchReferencePixels();
	bool RunFractInt();
	bool CompareImages();
};

#endif