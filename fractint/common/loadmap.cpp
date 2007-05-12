/** loadmap.c **/
#include <string.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"

/***************************************************************************/

#define dac ((Palettetype *)g_dac_box)

int validate_luts(const char *fn)
{
	char temp[FILE_MAX_PATH + 1];
	strcpy(temp, g_map_name);
	char temp_fn[FILE_MAX_PATH];
	strcpy(temp_fn, fn);
#ifdef XFRACT
	merge_path_names(temp, temp_fn, 3);
#else
	merge_path_names(temp, temp_fn, 0);
#endif
	if (has_extension(temp) == NULL) /* Did name have an extension? */
	{
		strcat(temp, ".map");  /* No? Then add .map */
	}
	char line[160];
	findpath(temp, line);        /* search the dos path */
	FILE *f = fopen(line, "r");
	if (f == NULL)
	{
		sprintf(line, "Could not load color map %s", fn);
		stop_message(0, line);
		return 1;
	}
	unsigned index;
	for (index = 0; index < 256; index++)
	{
		if (fgets(line, 100, f) == NULL)
		{
			break;
		}
		unsigned r;
		unsigned g;
		unsigned b;
		sscanf(line, "%u %u %u", &r, &g, &b);
		/** load global dac values **/
		// TODO: review when COLOR_CHANNEL_MAX != 63
		dac[index].red   = (BYTE)((r % 256) >> 2); /* maps default to 8 bits */
		dac[index].green = (BYTE)((g % 256) >> 2); /* DAC wants 6 bits */
		dac[index].blue  = (BYTE)((b % 256) >> 2);
	}
	fclose(f);
	while (index < 256)   /* zap unset entries */
	{
		dac[index].red = 40;
		dac[index].blue = 40;
		dac[index].green = 40;
		++index;
	}
	g_color_state = COLORSTATE_MAP;
	strcpy(g_color_file, fn);
	return 0;
}


/***************************************************************************/

int set_color_palette_name(char *fn)
{
	if (validate_luts(fn) != 0)
	{
		return 1;
	}
	if (g_map_dac_box == NULL)
	{
		g_map_dac_box = (BYTE *) malloc(256*3*sizeof(BYTE));
		if (g_map_dac_box == NULL)
		{
			stop_message(0, "Insufficient memory for color map.");
			return 1;
		}
	}
	memcpy((char *) g_map_dac_box, (char *) g_dac_box, 768);
	return 0;
}

