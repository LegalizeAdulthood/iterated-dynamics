#include "stdafx.h"

#include "realdos.h"
#include "id.h"

TEST(realdos, video_mode_key_name_f1)
{
	std::string name = video_mode_key_name(IDK_F1);
	CHECK_EQUAL("F1", name);
}
TEST(realdos, video_mode_key_name_f10)
{
	std::string name = video_mode_key_name(IDK_F10);
	CHECK_EQUAL("F10", name);
}

TEST(realdos, video_mode_key_name_ctl_f1)
{
	std::string name = video_mode_key_name(IDK_CTL_F1);
	CHECK_EQUAL("CF1", name);
}
TEST(realdos, video_mode_key_name_ctl_f10)
{
	std::string name = video_mode_key_name(IDK_CTL_F10);
	CHECK_EQUAL("CF10", name);
}

TEST(realdos, video_mode_key_name_alt_f1)
{
	std::string name = video_mode_key_name(IDK_ALT_F1);
	CHECK_EQUAL("AF1", name);
}
TEST(realdos, video_mode_key_name_alt_f10)
{
	std::string name = video_mode_key_name(IDK_ALT_F10);
	CHECK_EQUAL("AF10", name);
}

TEST(realdos, video_mode_key_name_shift_f1)
{
	std::string name = video_mode_key_name(IDK_SF1);
	CHECK_EQUAL("SF1", name);
}
TEST(realdos, video_mode_key_name_shift_f10)
{
	std::string name = video_mode_key_name(IDK_SF10);
	CHECK_EQUAL("SF10", name);
}

TEST(realdos, video_mode_key_name_no_video)
{
	std::string name = video_mode_key_name(IDK_CTL_B);
	CHECK_EQUAL("", name);
}
