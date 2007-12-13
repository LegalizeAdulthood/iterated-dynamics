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

#define dac ((Palettetype *)g_dac_box)

bool validate_luts(const std::string &filename)
{
	return validate_luts(filename.c_str());
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
		dac[index].red   = BYTE((r % 256) >> 2); /* maps default to 8 bits */
		dac[index].green = BYTE((g % 256) >> 2); /* DAC wants 6 bits */
		dac[index].blue  = BYTE((b % 256) >> 2);
	}
	f.close();
	while (index < 256)   /* zap unset entries */
	{
		dac[index].red = 40;
		dac[index].blue = 40;
		dac[index].green = 40;
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
		g_map_dac_box = new BYTE[256*3];
		if (g_map_dac_box == 0)
		{
			stop_message(STOPMSG_NORMAL, "Insufficient memory for color map.");
			return;
		}
	}
	memcpy((char *) g_map_dac_box, (char *) g_dac_box, 768);
}

