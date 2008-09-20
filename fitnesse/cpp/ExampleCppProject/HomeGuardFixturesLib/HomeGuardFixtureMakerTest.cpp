#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "HomeGuardFixtureMaker.h"
#include "Fit/Fit.h"
#include "Fit/ResolutionException.h"
#include "Fit/Summary.h"
#include "Helpers/SimpleStringExtensions.h"
#include "Control.h"
#include "SetUpHomeGuard.h"
#include "TearDownHomeGuard.h"
#include "CheckFrontPanel.h"
#include "CheckPhoneLine.h"
#include "DefineSensors.h"
#include "PortControl.h"
#include "EventLogOutput.h"
#include "FrontPanelController.h"



#include <memory>

EXPORT_TEST_GROUP(HomeGuardFixtureMaker);

namespace 
{
  HomeGuardFixtureMaker* maker;

  void SetUp()
  {
    maker = new HomeGuardFixtureMaker();
  }
  void TearDown()
  {
    delete maker;
  }
}

TEST(HomeGuardFixtureMaker, ColumnFixture)
{
   auto_ptr<Fixture> fixture(maker->make("ColumnFixture"));
   CHECK(0 != dynamic_cast<ColumnFixture *>(fixture.get()));
}


TEST(HomeGuardFixtureMaker, ActionFixture)
{
   auto_ptr<Fixture> fixture(maker->make("ActionFixture"));
   CHECK(0 != dynamic_cast<ActionFixture *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, Fixture)
{
   auto_ptr<Fixture> fixture(maker->make("Fixture"));
   CHECK(0 != fixture.get());
}

TEST(HomeGuardFixtureMaker, Summary)
{
   auto_ptr<Fixture> fixture(maker->make("Summary"));
   CHECK(0 != dynamic_cast<Summary *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, PrimitiveFixture)
{
   auto_ptr<Fixture> fixture(maker->make("PrimitiveFixture"));
   CHECK(0 != dynamic_cast<PrimitiveFixture *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, DoesNotExist)
{
  try 
  {
   auto_ptr<Fixture> fixture(maker->make("DoesNotExist"));
   FAIL("Should throw exception");
  } 
  catch (ResolutionException& e) 
  {
      e = e;
  }
}

TEST(HomeGuardFixtureMaker, Control)
{
   auto_ptr<Fixture> fixture(maker->make("Control"));
   CHECK(0 != dynamic_cast<Control *>(fixture.get()));
}

IGNORE_TEST(HomeGuardFixtureMaker, SetUpHomeGuard)
{
   auto_ptr<Fixture> fixture(maker->make("SetUpHomeGuard"));
   CHECK(0 != dynamic_cast<SetUpHomeGuard *>(fixture.get()));
}

IGNORE_TEST(HomeGuardFixtureMaker, TearDownHomeGuard)
{
   auto_ptr<Fixture> fixture(maker->make("TearDownHomeGuard"));
   CHECK(0 != dynamic_cast<TearDownHomeGuard *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, CheckFrontPanel)
{
   auto_ptr<Fixture> fixture(maker->make("CheckFrontPanel"));
   CHECK(0 != dynamic_cast<CheckFrontPanel *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, CheckPhoneLine)
{
   auto_ptr<Fixture> fixture(maker->make("CheckPhoneLine"));
   CHECK(0 != dynamic_cast<CheckPhoneLine *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, DefineSensors)
{
   auto_ptr<Fixture> fixture(maker->make("DefineSensors"));
   CHECK(0 != dynamic_cast<DefineSensors *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, PortControl)
{
   auto_ptr<Fixture> fixture(maker->make("PortControl"));
   CHECK(0 != dynamic_cast<PortControl *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, EventLogOutput)
{
   auto_ptr<Fixture> fixture(maker->make("EventLogOutput"));
   CHECK(0 != dynamic_cast<EventLogOutput *>(fixture.get()));
}

TEST(HomeGuardFixtureMaker, FrontPanelController)
{
   auto_ptr<Fixture> fixture(maker->make("FrontPanelController"));
   CHECK(0 != dynamic_cast<FrontPanelController *>(fixture.get()));
}



