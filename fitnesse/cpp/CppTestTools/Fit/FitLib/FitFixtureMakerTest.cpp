#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "FitFixtureMaker.h"
#include "Fit.h"
#include "Summary.h"
#include "Helpers/SimpleStringExtensions.h"
#include <memory>

EXPORT_TEST_GROUP(FitFixtureMaker);

namespace
  {
  FitFixtureMaker* maker;

  void SetUp()
  {
    maker = new FitFixtureMaker();
  }
  void TearDown()
  {
    delete maker;
  }
}

TEST(FitFixtureMaker, ColumnFixture)
{
  auto_ptr<Fixture> fixture(maker->make("ColumnFixture"));
  CHECK(0 != dynamic_cast<ColumnFixture *>(fixture.get()));
}


TEST(FitFixtureMaker, ActionFixture)
{
  auto_ptr<Fixture> fixture(maker->make("ActionFixture"));
  CHECK(0 != dynamic_cast<ActionFixture *>(fixture.get()));
}

TEST(FitFixtureMaker, Fixture)
{
  auto_ptr<Fixture> fixture(maker->make("Fixture"));
  CHECK(0 != fixture.get());
}

TEST(FitFixtureMaker, Summary)
{
  auto_ptr<Fixture> fixture(maker->make("Summary"));
  CHECK(0 != dynamic_cast<Summary *>(fixture.get()));
}

TEST(FitFixtureMaker, PrimitiveFixture)
{
  auto_ptr<Fixture> fixture(maker->make("PrimitiveFixture"));
  CHECK(0 != dynamic_cast<PrimitiveFixture *>(fixture.get()));
}

TEST(FitFixtureMaker, DoesNotExist)
{
  auto_ptr<Fixture> fixture(maker->make("DoesNotExist"));
  CHECK(0 == fixture.get());
}
