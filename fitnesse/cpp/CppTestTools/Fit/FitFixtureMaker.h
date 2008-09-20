#ifndef D_FitFixtureMaker_H
#define D_FitFixtureMaker_H

#include <string>
#include "FixtureMaker.h"

///////////////////////////////////////////////////////////////////////////////
//
//  FitFixtureMaker.h
//
//  FitFixtureMaker is responsible for makeing teh standard FIT fixtures
//
//  Be carful not to mix statically linked FIT fixtures with Dynamically
//  loaded Fit Fixtures
//
///////////////////////////////////////////////////////////////////////////////
class Fixture;

using std::string;

class FitFixtureMaker : public FixtureMaker
  {
  public:
    explicit FitFixtureMaker();
    virtual ~FitFixtureMaker();

    virtual Fixture* make(const string& name);

  private:

    FitFixtureMaker(const FitFixtureMaker&);
    FitFixtureMaker& operator=(const FitFixtureMaker&);

  };

#endif  // D_FitFixtureMaker_H
