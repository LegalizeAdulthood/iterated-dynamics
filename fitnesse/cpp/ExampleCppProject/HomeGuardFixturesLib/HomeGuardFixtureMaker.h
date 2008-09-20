#ifndef D_HomeGuardFixtureMaker_H
#define D_HomeGuardFixtureMaker_H

#include "Fit/FixtureMaker.h"
#include <string>

#define PUBLISH_FIXTURE(fixture) if (name == #fixture)	return new fixture;

class Fixture;

class HomeGuardFixtureMaker : public FixtureMaker
{
  public:
    explicit HomeGuardFixtureMaker();

    virtual ~HomeGuardFixtureMaker();

	  virtual Fixture	*make(const std::string& name);

  private:

    HomeGuardFixtureMaker(const HomeGuardFixtureMaker&);
    HomeGuardFixtureMaker& operator=(const HomeGuardFixtureMaker&);

};

#endif  // D_HomeGuardFixtureMaker_H
