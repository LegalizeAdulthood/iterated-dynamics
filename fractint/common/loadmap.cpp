/** loadmap.c **/
#include <string.h>
#include <string>

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "filesystem.h"
#include "loadmap.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"

/***************************************************************************/

bool validate_luts(const std::string &filename)
{
	return validate_luts(filename.c_str());
}

static BYTE map_to_dac(int value)
{
	return BYTE((value % 256) >> 2);
}

bool validate_luts(const char *fn)
{
	char temp[FILE_MAX_PATH + 1];
	strcpy(temp, g_map_name.c_str());
	char temp_fn[FILE_MAX_PATH];
	strcpy(temp_fn, fn);
#ifdef XFRACT
	merge_path_names(temp, temp_fn, false);
#else
	merge_path_names(temp, temp_fn, true);
#endif
	ensure_extension(temp, ".map");
	char name[160];
	find_path(temp, name);        /* search the dos path */
	std::ifstream f(name);
	if (!f)
	{
		stop_message(STOPMSG_NORMAL, "Could not load color map " + std::string(fn));
		return true;
	}
	unsigned index;
	for (index = 0; index < 256; index++)
	{
		unsigned r;
		unsigned g;
		unsigned b;
		f >> r >> g >> b;
		if (!f)
		{
			break;
		}

		/** load global dac values **/
		// TODO: review when COLOR_CHANNEL_MAX != 63
		g_.DAC().Set(index, map_to_dac(r), map_to_dac(g), map_to_dac(b));
	}
	f.close();
	while (index < 256)   /* zap unset entries */
	{
		g_.DAC().Set(index, 40, 40, 40);
		++index;
	}
	g_color_state = COLORSTATE_MAP;
	g_color_file = fn;
	return false;
}


/***************************************************************************/
void set_color_palette_name(const std::string &filename)
{
	if (validate_luts(filename))
	{
		return;
	}
	if (g_map_dac_box == 0)
	{
		g_map_dac_box = new ColormapTable;
		if (g_map_dac_box == 0)
		{
			stop_message(STOPMSG_NORMAL, "Insufficient memory for color map.");
			return;
		}
	}
	*g_map_dac_box = g_.DAC();
}

