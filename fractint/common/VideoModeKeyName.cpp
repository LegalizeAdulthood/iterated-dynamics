#include <string>
#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "VideoModeKeyName.h"

static std::string video_mode_key_name(int key, int base, const char *prefix)
{
	return str(boost::format("%s%d") % prefix % (key - base + 1));
}

std::string video_mode_key_name(int k)
{
	if (k >= IDK_ALT_F1 && k <= IDK_ALT_F10)
	{
		return video_mode_key_name(k, IDK_ALT_F1, "AF");
	}
	if (k >= IDK_CTL_F1 && k <= IDK_CTL_F10)
	{
		return video_mode_key_name(k, IDK_CTL_F1, "CF");
	}
	if (k >= IDK_SF1 && k <= IDK_SF10)
	{
		return video_mode_key_name(k, IDK_SF1, "SF");
	}
	if (k >= IDK_F1 && k <= IDK_F10)
	{
		return video_mode_key_name(k, IDK_F1, "F");
	}
	return "";
}
