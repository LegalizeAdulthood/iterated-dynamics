#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "VideoModeKeyName.h"
#include "id.h"

BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_f1)
{
	std::string name = video_mode_key_name(IDK_F1);
	BOOST_CHECK_EQUAL("F1", name);
}
BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_f10)
{
	std::string name = video_mode_key_name(IDK_F10);
	BOOST_CHECK_EQUAL("F10", name);
}

BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_ctl_f1)
{
	std::string name = video_mode_key_name(IDK_CTL_F1);
	BOOST_CHECK_EQUAL("CF1", name);
}
BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_ctl_f10)
{
	std::string name = video_mode_key_name(IDK_CTL_F10);
	BOOST_CHECK_EQUAL("CF10", name);
}

BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_alt_f1)
{
	std::string name = video_mode_key_name(IDK_ALT_F1);
	BOOST_CHECK_EQUAL("AF1", name);
}
BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_alt_f10)
{
	std::string name = video_mode_key_name(IDK_ALT_F10);
	BOOST_CHECK_EQUAL("AF10", name);
}

BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_shift_f1)
{
	std::string name = video_mode_key_name(IDK_SF1);
	BOOST_CHECK_EQUAL("SF1", name);
}
BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_shift_f10)
{
	std::string name = video_mode_key_name(IDK_SF10);
	BOOST_CHECK_EQUAL("SF10", name);
}

BOOST_AUTO_TEST_CASE(realdos_video_mode_key_name_no_video)
{
	std::string name = video_mode_key_name(IDK_CTL_B);
	BOOST_CHECK_EQUAL("", name);
}
