#include <boost/test/unit_test.hpp>
#include "CGAWindowControl.h"
#include "CGAWIndowControlMediator.h"

BOOST_AUTO_TEST_CASE(CGAWindowControl_ConstructDefault)
{
	wxId::CGAWindowControl window;
	wxControl *control = static_cast<wxControl *>(&window);
	BOOST_CHECK(control != 0);
}
