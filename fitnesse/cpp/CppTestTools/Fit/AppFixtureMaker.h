#ifndef D_APPFIXTUREMAKER_H
#define D_APPFIXTUREMAKER_H

#include "FixtureMaker.h"
#include <string>

#define PUBLISH_FIXTURE(fixture) if (name == #fixture)	return new fixture;

class Fixture;

class AppFixtureMaker : public FixtureMaker
  {
  public:
    explicit AppFixtureMaker();

    virtual ~AppFixtureMaker();

    virtual Fixture	*make(const std::string& name);

  private:

    AppFixtureMaker(const AppFixtureMaker&);
    AppFixtureMaker& operator=(const AppFixtureMaker&);

  };

#endif  // D_APPFIXTUREMAKER_H
