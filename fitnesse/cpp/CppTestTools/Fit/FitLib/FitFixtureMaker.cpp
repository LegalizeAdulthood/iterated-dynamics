#include "Platform.h"
#include "FitFixtureMaker.h"
#include "Fit.h"
#include "Summary.h"

FitFixtureMaker::FitFixtureMaker()
{}

FitFixtureMaker::~FitFixtureMaker()
{}

Fixture* FitFixtureMaker::make(const string& name)
{
  /* static and dynamic fixtures don't mix well
   * their statics variables get confused
   */

  if (name == "Fixture")
    return new Fixture;
  if (name == "ColumnFixture")
    return new ColumnFixture;
  if (name == "PrimitiveFixture")
    return new PrimitiveFixture;
  if (name == "ActionFixture")
    return new ActionFixture;
  if (name == "Summary")
    return new Summary;

  return 0;
}
