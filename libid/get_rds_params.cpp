// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_rds_params.h"

#include "port.h"
#include "prototyp.h"

#include "drivers.h"
#include "full_screen_prompt.h"
#include "get_a_filename.h"
#include "helpdefs.h"
#include "id_data.h"
#include "stereo.h"
#include "value_saver.h"

#include <cstring>
#include <string>

static char const *s_masks[] = {"*.pot", "*.gif"};

int get_rds_params()
{
    char rds6[60];
    char const *stereobars[] = {"none", "middle", "top"};
    FullScreenValues uvalues[7];
    char const *rds_prompts[7] =
    {
        "Depth Effect (negative reverses front and back)",
        "Image width in inches",
        "Use grayscale value for depth? (if \"no\" uses color number)",
        "Calibration bars",
        "Use image map? (if \"no\" uses random dots)",
        "  If yes, use current image map name? (see below)",
        rds6
    };
    int ret;
    static char reuse = 0;
    driver_stack_screen();
    while (true)
    {
        ret = 0;

        int k = 0;
        uvalues[k].uval.ival = g_auto_stereo_depth;
        uvalues[k++].type = 'i';

        uvalues[k].uval.dval = g_auto_stereo_width;
        uvalues[k++].type = 'f';

        uvalues[k].uval.ch.val = g_gray_flag ? 1 : 0;
        uvalues[k++].type = 'y';

        uvalues[k].type = 'l';
        uvalues[k].uval.ch.list = stereobars;
        uvalues[k].uval.ch.vlen = 6;
        uvalues[k].uval.ch.llen = 3;
        uvalues[k++].uval.ch.val  = g_calibrate;

        uvalues[k].uval.ch.val = g_image_map ? 1 : 0;
        uvalues[k++].type = 'y';

        if (!g_stereo_map_filename.empty() && g_image_map)
        {
            uvalues[k].uval.ch.val = reuse;
            uvalues[k++].type = 'y';

            uvalues[k++].type = '*';
            for (auto & elem : rds6)
            {
                elem = ' ';
            }
            auto p = g_stereo_map_filename.find(SLASHC);
            if (p == std::string::npos ||
                    (int) g_stereo_map_filename.length() < sizeof(rds6)-2)
            {
                p = 0;
            }
            else
            {
                p++;
            }
            // center file name
            rds6[(sizeof(rds6)-(int) (g_stereo_map_filename.length() - p)+2)/2] = 0;
            std::strcat(rds6, "[");
            std::strcat(rds6, &g_stereo_map_filename.c_str()[p]);
            std::strcat(rds6, "]");
        }
        else
        {
            g_stereo_map_filename.clear();
        }
        int choice;
        {
            ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_RDS};
            choice = full_screen_prompt("Random Dot Stereogram Parameters", k, rds_prompts, uvalues, 0, nullptr);
        }
        if (choice < 0)
        {
            ret = -1;
            break;
        }
        else
        {
            k = 0;
            g_auto_stereo_depth = uvalues[k++].uval.ival;
            g_auto_stereo_width = uvalues[k++].uval.dval;
            g_gray_flag         = uvalues[k++].uval.ch.val != 0;
            g_calibrate        = (char)uvalues[k++].uval.ch.val;
            g_image_map        = uvalues[k++].uval.ch.val != 0;
            if (!g_stereo_map_filename.empty() && g_image_map)
            {
                reuse         = (char)uvalues[k++].uval.ch.val;
            }
            else
            {
                reuse = 0;
            }
            if (g_image_map && !reuse)
            {
                if (get_a_file_name("Select an Imagemap File", s_masks[1], g_stereo_map_filename))
                {
                    continue;
                }
            }
        }
        break;
    }
    driver_unstack_screen();
    return ret;
}
