#if !defined(ID_FIXTURE_MAKER_H)
#define ID_FIXTURE_MAKER_H

#include "Fit/FixtureMaker.h"

class idFixtureMaker : public FixtureMaker
{
public:
	idFixtureMaker() : FixtureMaker() { }
	virtual ~idFixtureMaker() { }

	virtual Fixture *make(const std::string &name);
};

#endif
