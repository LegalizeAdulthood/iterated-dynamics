#if !defined(ITERATED_DYNAMICS_FIXTURE_MAKER_H)
#define ITERATED_DYNAMICS_FIXTURE_MAKER_H

#include "Fit/FixtureMaker.h"

class IteratedDynamicsFixtureMaker : public FixtureMaker
{
public:
	explicit IteratedDynamicsFixtureMaker();
	virtual ~IteratedDynamicsFixtureMaker();

	virtual Fixture	*make(std::string const &name);

private:
	// hide copy c'tors
    IteratedDynamicsFixtureMaker(IteratedDynamicsFixtureMaker const &);
    IteratedDynamicsFixtureMaker& operator=(IteratedDynamicsFixtureMaker const &);
};

#endif
