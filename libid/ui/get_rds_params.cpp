// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_rds_params.h"

#include "helpdefs.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/full_screen_prompt.h"
#include "ui/help.h"
#include "ui/stereo.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <string>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{

static const char *s_masks[] = {"*.pot", "*.gif"};

int get_rds_params()
{
    char rds6[60];
    const char *stereo_bars[] = {"none", "middle", "top"};
    std::array<const char *, 7> rds_prompts{
        "Depth Effect (negative reverses front and back)",              //
        "Image width in inches",                                        //
        "Use grayscale value for depth? (if \"no\" uses color number)", //
        "Calibration bars",                                             //
        "Use texture map? (if \"no\" uses random dots)",                //
        "  If yes, use current texture map name? (see below)",          //
        rds6                                                            //
    };
    int ret;
    driver_stack_screen();
    while (true)
    {
        std::array<FullScreenValues, 7> values;
        ret = 0;

        int k = 0;
        values[k].uval.ival = g_auto_stereo_depth;
        values[k++].type = 'i';

        values[k].uval.dval = g_auto_stereo_width;
        values[k++].type = 'f';

        values[k].uval.ch.val = g_gray_flag ? 1 : 0;
        values[k++].type = 'y';

        values[k].type = 'l';
        values[k].uval.ch.list = stereo_bars;
        values[k].uval.ch.vlen = 6;
        values[k].uval.ch.list_len = 3;
        values[k++].uval.ch.val  = +g_calibrate;

        values[k].uval.ch.val = g_use_stereo_texture ? 1 : 0;
        values[k++].type = 'y';

        if (!g_stereo_texture_filename.empty() && g_use_stereo_texture)
        {
            values[k].uval.ch.val = g_stereo_texture_reuse ? 1 : 0;
            values[k++].type = 'y';

            values[k++].type = '*';
            std::fill(std::begin(rds6), std::end(rds6), ' ');
            auto p = g_stereo_texture_filename.find(SLASH_CH);
            if (p == std::string::npos ||
                    static_cast<int>(g_stereo_texture_filename.length()) < sizeof(rds6)-2)
            {
                p = 0;
            }
            else
            {
                p++;
            }
            // center file name
            rds6[(sizeof(rds6)- static_cast<int>(g_stereo_texture_filename.length() - p) +2)/2] = 0;
            std::strcat(rds6, "[");
            std::strcat(rds6, &g_stereo_texture_filename.c_str()[p]);
            std::strcat(rds6, "]");
        }
        else
        {
            g_stereo_texture_filename.clear();
            g_stereo_texture_reuse = false;
        }
        int choice;
        {
            ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_RDS};
            choice = full_screen_prompt(
                "Random Dot Stereogram Parameters", k, rds_prompts.data(), values.data(), 0, nullptr);
        }
        if (choice < 0)
        {
            ret = -1;
            break;
        }
        k = 0;
        g_auto_stereo_depth = values[k++].uval.ival;
        g_auto_stereo_width = values[k++].uval.dval;
        g_gray_flag = values[k++].uval.ch.val != 0;
        g_calibrate = static_cast<CalibrationBars>(values[k++].uval.ch.val);
        g_use_stereo_texture = values[k++].uval.ch.val != 0;
        if (!g_stereo_texture_filename.empty() && g_use_stereo_texture)
        {
            g_stereo_texture_reuse = values[k++].uval.ch.val != 0;
        }
        else
        {
            g_stereo_texture_reuse = false;
        }
        if (g_use_stereo_texture && !g_stereo_texture_reuse)
        {
            if (driver_get_filename(
                    "Select a Texture Map File", "Texture Map", s_masks[1], g_stereo_texture_filename))
            {
                continue;
            }
        }
        break;
    }
    driver_unstack_screen();
    return ret;
}

} // namespace id::ui
