#include "UnitTestHarness/TestHarness.h"
#include "Helpers/SimpleStringExtensions.h"
#include "HomeGuard.h"
#include "MockFrontPanel.h"

EXPORT_TEST_GROUP(HomeGuard);

namespace
  {
  HomeGuard* homeGuard;
  MockFrontPanel* frontPanel;
  
  void SetUp()
  {
    frontPanel = new MockFrontPanel();
    homeGuard = new HomeGuard(frontPanel);
  }
  void TearDown()
  {
    delete homeGuard;
  }
}

TEST(HomeGuard, PowerUp)
{
  CHECK( true == frontPanel->isArmedIndicatorOn());
}

