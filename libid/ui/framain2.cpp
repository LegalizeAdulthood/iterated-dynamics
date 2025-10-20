// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/framain2.h"

#include "engine/Browse.h"
#include "engine/calc_frac_init.h"
#include "engine/cmdfiles.h"
#include "engine/color_state.h"
#include "engine/ImageRegion.h"
#include "engine/LogicalScreen.h"
#include "engine/Potential.h"
#include "engine/sound.h"
#include "engine/spindac.h"
#include "engine/UserData.h"
#include "engine/video_mode.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "geometry/line3d.h"
#include "io/decoder.h"
#include "io/dir_file.h"
#include "io/gifview.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "io/save_timer.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/evolve.h"
#include "ui/evolver_menu_switch.h"
#include "ui/get_fract_type.h"
#include "ui/goodbye.h"
#include "ui/history.h"
#include "ui/id_keys.h"
#include "ui/main_menu.h"
#include "ui/main_menu_switch.h"
#include "ui/mouse.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/video.h"
#include "ui/zoom.h"

#include <config/port.h>

#include <fmt/format.h>

#include <array> // std::size
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

bool g_compare_gif{};           // compare two gif files flag
bool g_from_text{};             // = true if we're in graphics mode
int g_finish_row{};             // save when this row is finished
EvolutionInfo g_evolve_info{};  //
bool g_have_evolve_info{};      //
void (*g_out_line_cleanup)(){}; //
bool g_virtual_screens{};       //

static int call_line3d(Byte *pixels, int line_len);
static void cmp_line_cleanup();
static int cmp_line(Byte *pixels, int line_len);

static std::FILE *s_cmp_fp{};
static int s_err_count{};

static int iround(const double value)
{
    return static_cast<int>(std::lround(value));
}

namespace
{

class ZoomMouseNotification : public NullMouseNotification
{
public:
    ~ZoomMouseNotification() override = default;
    void primary_down(bool double_click, int x, int y, int key_flags) override;
    void secondary_down(bool double_click, int x, int y, int key_flags) override;
    void middle_down(bool double_click, int x, int y, int key_flags) override;
    void primary_up(int x, int y, int key_flags) override;
    void secondary_up(int x, int y, int key_flags) override;
    void middle_up(int x, int y, int key_flags) override;
    void move(int x, int y, int key_flags) override;

    // return false if no additional action needed;
    // return true if main loop should be continued.
    bool process_zoom(MainContext &context);

private:
    void button_down(int x, int y, bool &flag);
    bool process_zoom_primary(MainContext &context);
    bool process_zoom_secondary(MainContext &context);
    bool process_zoom_middle(MainContext & context);

    int m_x{-1};
    int m_y{-1};
    int m_down_x{};
    int m_down_y{};
    bool m_zoom_in_pending{};
    bool m_zoom_out_pending{};
    bool m_primary_down{};
    bool m_secondary_down{};
    bool m_middle_down{};
    bool m_position_updated{};
};

void ZoomMouseNotification::primary_down(const bool double_click, const int x, const int y, int key_flags)
{
    if (g_zoom_box_width != 0.0 && double_click)
    {
        m_zoom_in_pending = true;
    }
    button_down(x, y, m_primary_down);
}

void ZoomMouseNotification::secondary_down(const bool double_click, const int x, const int y, int key_flags)
{
    if (g_zoom_box_width != 0.0 && double_click)
    {
        m_zoom_out_pending = true;
    }
    button_down(x, y, m_secondary_down);
}

void ZoomMouseNotification::middle_down(bool double_click, const int x, const int y, int key_flags)
{
    button_down(x, y, m_middle_down);
}

void ZoomMouseNotification::primary_up(int x, int y, int key_flags)
{
    m_primary_down = false;
}

void ZoomMouseNotification::secondary_up(int x, int y, int key_flags)
{
    m_secondary_down = false;
}

void ZoomMouseNotification::middle_up(int x, int y, int key_flags)
{
    m_middle_down = false;
}

void ZoomMouseNotification::move(const int x, const int y, int /*key_flags*/)
{
    if (m_x != x || m_y != y)
    {
        m_x = x;
        m_y = y;
        m_position_updated = true;
    }
    assert(!m_zoom_in_pending || g_zoom_box_width == 0.0);
    assert(!m_zoom_out_pending || g_zoom_box_width == 0.0);
}

bool ZoomMouseNotification::process_zoom_primary(MainContext &context)
{
    if (g_zoom_box_width == 0.0)
    {
        zoom_box_in(context);
        draw_box(true);
        return true;
    }
    if (m_position_updated)
    {
        const int delta_y = m_y - m_down_y;
        const int amt{static_cast<int>(g_logical_screen.y_size_dots * 0.02)};
        const int steps{delta_y / amt};
        if (steps != 0)
        {
            if (steps > 0 && g_zoom_box_width >= 0.95 && g_zoom_box_height >= 0.95)
            {
                g_zoom_box_width = 0.0;
            }
            else
            {
                m_down_y = m_y;
                resize_box(steps);
            }
            draw_box(true);
        }
        return false;
    }
    return false;
}

bool ZoomMouseNotification::process_zoom_secondary(MainContext &/*context*/)
{
    if (g_zoom_box_width == 0.0)
    {
        return false;
    }
    if (m_position_updated)
    {
        const int delta_x = m_x - m_down_x;
        const int amt{static_cast<int>(g_logical_screen.x_size_dots * 0.02)};
        const int steps{delta_x / amt};
        if (steps != 0)
        {
            m_down_x = m_x;
            g_zoom_box_rotation += steps;
            draw_box(true);
        }
        return false;
    }

    return false;
}

bool ZoomMouseNotification::process_zoom_middle(MainContext &/*context*/)
{
    if (g_zoom_box_width == 0.0)
    {
        return false;
    }
    if (m_position_updated)
    {
        const int delta_y = m_y - m_down_y;
        m_down_y = m_y;
        change_box(0, 2 * delta_y);
        draw_box(true);
        return false;
    }

    return false;
}

bool ZoomMouseNotification::process_zoom(MainContext &context)
{
    if (m_zoom_in_pending)
    {
        request_zoom_in(context);
        m_zoom_in_pending = false;
        return true;
    }
    if (m_zoom_out_pending)
    {
        request_zoom_out(context);
        m_zoom_out_pending = false;
        return true;
    }
    if (m_primary_down)
    {
        return process_zoom_primary(context);
    }
    if (m_secondary_down)
    {
        return process_zoom_secondary(context);
    }
    if (m_middle_down)
    {
        return process_zoom_middle(context);
    }
    if (m_position_updated)
    {
        if (g_box_count)
        {
            g_zoom_box_x = static_cast<double>(m_x) / g_logical_screen.x_size_dots - g_zoom_box_width / 2.0;
            g_zoom_box_y = static_cast<double>(m_y) / g_logical_screen.y_size_dots - g_zoom_box_height / 2.0;
            draw_box(true);
        }
        m_position_updated = false;
        return false;
    }
    return false;
}

void ZoomMouseNotification::button_down(const int x, const int y, bool &flag)
{
    flag = true;
    m_down_x = x;
    m_down_y = y;
}

} // namespace

MainState big_while_loop(MainContext &context)
{
    int     i = 0;                           // temporary loop counters
    MainState mms_value;
    const auto mouse{std::make_shared<ZoomMouseNotification>()};
    MouseSubscription subscription{mouse};

    driver_check_memory();
    context.from_mandel = false;            // if julia entered from mandel
    if (context.resume)
    {
        goto resumeloop;
    }

    while (true)                    // eternal loop
    {
        driver_check_memory();
        if (g_calc_status != CalcStatus::RESUMABLE || g_show_file == ShowFile::LOAD_IMAGE)
        {
            g_video_entry = g_video_table[g_adapter];
            g_logical_screen.x_dots = g_video_entry.x_dots; // # dots across the screen
            g_logical_screen.y_dots = g_video_entry.y_dots; // # dots down the screen
            g_colors = g_video_entry.colors;                // # colors available
            g_screen_x_dots = g_logical_screen.x_dots;
            g_screen_y_dots = g_logical_screen.y_dots;
            g_logical_screen.y_offset = 0;
            g_logical_screen.x_offset = 0;
            g_color_cycle_range_hi = g_color_cycle_range_hi < g_colors ? g_color_cycle_range_hi : g_colors - 1;

            std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save the DAC

            if (g_overlay_3d && g_init_batch == BatchMode::NONE)
            {
                driver_unstack_screen();            // restore old graphics image
                g_overlay_3d = false;
            }
            else
            {
                driver_set_video_mode(g_video_entry); // switch video modes
                // switching video modes may have changed drivers or disk flag...
                if (!g_good_mode)
                {
                    if (!driver_is_disk())
                    {
                        stop_msg("That video mode is not available with your adapter.");
                    }
                    g_ask_video = true;
                    g_init_mode = -1;
                    driver_set_for_text(); // switch to text mode
                    return MainState::RESTORE_START;
                }

                if (g_virtual_screens && (g_logical_screen.x_dots > g_screen_x_dots || g_logical_screen.y_dots > g_screen_y_dots))
                {
#define MSG_XY1 "Can't set virtual line that long, width cut down."
#define MSG_XY2 "Not enough video memory for that many lines, height cut down."
                    if (g_logical_screen.x_dots > g_screen_x_dots && g_logical_screen.y_dots > g_screen_y_dots)
                    {
                        stop_msg(MSG_XY1 "\n" MSG_XY2);
                    }
                    else if (g_logical_screen.y_dots > g_screen_y_dots)
                    {
                        stop_msg(MSG_XY2);
                    }
                    else
                    {
                        stop_msg(MSG_XY1);
                    }
#undef MSG_XY1
#undef MSG_XY2
                }
                g_logical_screen.x_dots = g_screen_x_dots;
                g_logical_screen.y_dots = g_screen_y_dots;
                g_video_entry.x_dots = g_logical_screen.x_dots;
                g_video_entry.y_dots = g_logical_screen.y_dots;
            }

            if (g_save_dac != SaveDAC::NO || g_colors_preloaded)
            {
                std::memcpy(g_dac_box, g_old_dac_box, 256*3); // restore the DAC
                refresh_dac();
                g_colors_preloaded = false;
            }
            else
            {
                // reset DAC to defaults, which setvideomode has done for us
                if (g_map_specified)
                {
                    // but there's a map=, so load that
                    for (int j = 0; j < 256; ++j)
                    {
                        g_dac_box[j][0] = g_map_clut[j][0];
                        g_dac_box[j][1] = g_map_clut[j][1];
                        g_dac_box[j][2] = g_map_clut[j][2];
                    }
                    refresh_dac();
                }
                else if ((driver_is_disk() && g_colors == 256) || !g_colors)
                {
                    // disk video, setvideomode via bios didn't get it right, so:
                    validate_luts("default"); // read the default palette file
                }
                g_color_state = ColorState::DEFAULT_MAP;
            }
            if (g_viewport.enabled)
            {
                // bypass for VESA virtual screen
                const double f_temp{g_viewport.final_aspect_ratio *
                    (static_cast<double>(g_screen_y_dots) / static_cast<double>(g_screen_x_dots) / g_screen_aspect)};
                g_logical_screen.x_dots = g_viewport.x_dots;
                if (g_logical_screen.x_dots != 0)
                {
                    // xdots specified
                    g_logical_screen.y_dots = g_viewport.y_dots;
                    if (g_logical_screen.y_dots == 0) // calc ydots?
                    {
                        g_logical_screen.y_dots = iround(g_logical_screen.x_dots * f_temp);
                    }
                }
                else if (g_viewport.final_aspect_ratio <= g_screen_aspect)
                {
                    g_logical_screen.x_dots = iround(static_cast<double>(g_screen_x_dots) / g_viewport.reduction);
                    g_logical_screen.y_dots = iround(g_logical_screen.x_dots * f_temp);
                }
                else
                {
                    g_logical_screen.y_dots = iround(static_cast<double>(g_screen_y_dots) / g_viewport.reduction);
                    g_logical_screen.x_dots = iround(g_logical_screen.y_dots / f_temp);
                }
                if (g_logical_screen.x_dots > g_screen_x_dots || g_logical_screen.y_dots > g_screen_y_dots)
                {
                    stop_msg("View window too large; using full screen.");
                    g_viewport.enabled = false;
                    g_viewport.x_dots = g_screen_x_dots;
                    g_logical_screen.x_dots = g_viewport.x_dots;
                    g_viewport.y_dots = g_screen_y_dots;
                    g_logical_screen.y_dots = g_viewport.y_dots;
                }
                // changed test to 1, so a 2x2 window will work with the sound feature
                else if ((g_logical_screen.x_dots <= 1 || g_logical_screen.y_dots <= 1) &&
                    !bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
                {
                    // so ssg works
                    // but no check if in evolve mode to allow lots of small views
                    stop_msg("View window too small; using full screen.");
                    g_viewport.enabled = false;
                    g_logical_screen.x_dots = g_screen_x_dots;
                    g_logical_screen.y_dots = g_screen_y_dots;
                }
                if (bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP) &&
                    bit_set(g_cur_fractal_specific->flags, FractalFlags::INF_CALC))
                {
                    stop_msg("Fractal doesn't terminate! switching off evolution.");
                    g_evolving ^= EvolutionModeFlags::FIELD_MAP;
                    g_viewport.enabled = false;
                    g_logical_screen.x_dots = g_screen_x_dots;
                    g_logical_screen.y_dots = g_screen_y_dots;
                }
                if (bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
                {
                    const int grout = bit_set(g_evolving, EvolutionModeFlags::NO_GROUT) ? 0 : 1;
                    g_logical_screen.x_dots = g_screen_x_dots / g_evolve_image_grid_size - grout;
                    // trim to multiple of 4 for SSG
                    g_logical_screen.x_dots = g_logical_screen.x_dots - g_logical_screen.x_dots % 4;
                    g_logical_screen.y_dots = g_screen_y_dots / g_evolve_image_grid_size - grout;
                    g_logical_screen.y_dots = g_logical_screen.y_dots - g_logical_screen.y_dots % 4;
                }
                else
                {
                    g_logical_screen.x_offset = (g_screen_x_dots - g_logical_screen.x_dots) / 2;
                    g_logical_screen.y_offset = (g_screen_y_dots - g_logical_screen.y_dots) / 3;
                }
            }
            g_logical_screen.x_size_dots = g_logical_screen.x_dots - 1;            // convert just once now
            g_logical_screen.y_size_dots = g_logical_screen.y_dots - 1;
        }
        // assume we save next time (except jb)
        if (g_save_dac == SaveDAC::NO)
        {
            g_save_dac = SaveDAC::NEXT_TIME;
        }
        else
        {
            g_save_dac = SaveDAC::YES;
        }

        if (g_show_file == ShowFile::LOAD_IMAGE)
        {
            // loading an image
            g_out_line_cleanup = nullptr;            // outln routine can set this
            if (g_display_3d != Display3DMode::NONE) // set up 3D decoding
            {
                g_out_line = call_line3d;
            }
            else if (g_compare_gif)            // debug 50
            {
                g_out_line = cmp_line;
            }
            else if (g_potential.store_16bit)
            {
                // .pot format input file
                if (pot_start_disk() < 0)
                {
                    // pot file failed?
                    g_show_file = ShowFile::IMAGE_LOADED;
                    g_potential.flag  = false;
                    g_potential.store_16bit = false;
                    g_init_mode = -1;
                    g_calc_status = CalcStatus::RESUMABLE;         // "resume" without 16-bit
                    driver_set_for_text();
                    get_fract_type();
                    return MainState::IMAGE_START;
                }
                g_out_line = pot_line;
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP &&
                g_evolving == EvolutionModeFlags::NONE) // regular gif/fra input file
            {
                g_out_line = sound_line;      // sound decoding
            }
            else
            {
                g_out_line = out_line;        // regular decoding
            }
            i = funny_glasses_call(gif_view);
            if (g_out_line_cleanup)              // cleanup routine defined?
            {
                g_out_line_cleanup();
            }
            if (i == 0)
            {
                driver_buzzer(Buzzer::COMPLETE);
            }
            else
            {
                g_calc_status = CalcStatus::NO_FRACTAL;
                if (driver_key_pressed())
                {
                    driver_buzzer(Buzzer::INTERRUPT);
                    while (driver_key_pressed())
                    {
                        driver_get_key();
                    }
                    text_temp_msg("*** load incomplete ***");
                }
            }
        }

        // for these cases disable zooming
        g_zoom_enabled = !driver_is_disk() && !bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_ZOOM);
        if (g_evolving == EvolutionModeFlags::NONE)
        {
            calc_frac_init();
        }
        driver_schedule_alarm(1);

        g_save_image_region = g_image_region; // save 3 corners for zoom.c ref points

        if (g_bf_math != BFMathType::NONE)
        {
            copy_bf(g_bf_save_x_min, g_bf_x_min);
            copy_bf(g_bf_save_x_max, g_bf_x_max);
            copy_bf(g_bf_save_y_min, g_bf_y_min);
            copy_bf(g_bf_save_y_max, g_bf_y_max);
            copy_bf(g_bf_save_x_3rd, g_bf_x_3rd);
            copy_bf(g_bf_save_y_3rd, g_bf_y_3rd);
        }
        save_history_info();

        if (g_show_file == ShowFile::LOAD_IMAGE)
        {
            // image has been loaded
            g_show_file = ShowFile::IMAGE_LOADED;
            if (g_init_batch == BatchMode::NORMAL && g_calc_status == CalcStatus::RESUMABLE)
            {
                g_init_batch = BatchMode::FINISH_CALC_BEFORE_SAVE;
            }
            if (g_loaded_3d)      // 'r' of image created with '3'
            {
                g_display_3d = Display3DMode::YES;  // so set flag for 'b' command
            }
        }
        else
        {
            // draw an image
            if (g_save_time_interval != 0 // autosave and resumable?
                && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_RESUME))
            {
                start_save_timer();
                g_finish_row = -1;
            }
            g_browse.browsing = false;       // regenerate image, turn off browsing
            g_browse.stack.clear(); // reset filename stack
            g_browse.name.clear();
            if (g_viewport.enabled && bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP) &&
                g_calc_status != CalcStatus::COMPLETED)
            {
                // generate a set of images with varied parameters on each one
                int count;
                GeneBase gene[NUM_GENES];
                copy_genes_from_bank(gene);
                if (g_have_evolve_info && g_calc_status == CalcStatus::RESUMABLE)
                {
                    g_evolve_x_parameter_range = g_evolve_info.x_parameter_range;
                    g_evolve_y_parameter_range = g_evolve_info.y_parameter_range;
                    g_evolve_new_x_parameter_offset = g_evolve_info.x_parameter_offset;
                    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
                    g_evolve_new_y_parameter_offset = g_evolve_info.y_parameter_offset;
                    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
                    g_evolve_new_discrete_x_parameter_offset = static_cast<char>(g_evolve_info.discrete_x_parameter_offset);
                    g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
                    g_evolve_new_discrete_y_parameter_offset = static_cast<char>(g_evolve_info.discrete_y_parameter_offset);
                    g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset;
                    g_evolve_param_grid_x           = g_evolve_info.px;
                    g_evolve_param_grid_y           = g_evolve_info.py;
                    g_logical_screen.x_offset       = g_evolve_info.screen_x_offset;
                    g_logical_screen.y_offset       = g_evolve_info.screen_y_offset;
                    g_logical_screen.x_dots        = g_evolve_info.x_dots;
                    g_logical_screen.y_dots        = g_evolve_info.y_dots;
                    g_evolve_image_grid_size = g_evolve_info.image_grid_size;
                    g_evolve_this_generation_random_seed = g_evolve_info.this_generation_random_seed;
                    g_evolve_max_random_mutation = g_evolve_info.max_random_mutation;
                    g_evolving = static_cast<EvolutionModeFlags>(g_evolve_info.evolving);
                    g_viewport.enabled = g_evolving != EvolutionModeFlags::NONE;
                    count       = g_evolve_info.count;
                    g_have_evolve_info = false;
                }
                else
                {
                    // not resuming, start from the beginning
                    int mid = g_evolve_image_grid_size / 2;
                    if (g_evolve_param_grid_x != mid || g_evolve_param_grid_y != mid)
                    {
                        g_evolve_this_generation_random_seed = static_cast<unsigned int>(std::clock()); // time for new set
                    }
                    save_param_history();
                    count = 0;
                    g_evolve_max_random_mutation = g_evolve_max_random_mutation * g_evolve_mutation_reduction_factor;
                    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
                    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
                    g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
                    g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset; // evolve_discrete_x_parameter_offset used for discrete params like inside, outside, trigfn etc
                }
                g_evolve_param_box_count = 0;
                g_evolve_dist_per_x = g_evolve_x_parameter_range /(g_evolve_image_grid_size -1);
                g_evolve_dist_per_y = g_evolve_y_parameter_range /(g_evolve_image_grid_size -1);
                const int grout = bit_set(g_evolving, EvolutionModeFlags::NO_GROUT) ? 0 : 1;
                const int tmp_x_dots = g_logical_screen.x_dots + grout;
                const int tmp_y_dots = g_logical_screen.y_dots + grout;
                const int grid_sqr = g_evolve_image_grid_size * g_evolve_image_grid_size;
                while (count < grid_sqr)
                {
                    spiral_map(count); // sets px & py
                    g_logical_screen.x_offset = tmp_x_dots * g_evolve_param_grid_x;
                    g_logical_screen.y_offset = tmp_y_dots * g_evolve_param_grid_y;
                    restore_param_history();
                    fiddle_params(gene, count);
                    calc_frac_init();
                    if (calc_fract() == -1)
                    {
                        goto done;
                    }
                    count ++;
                }
done:
                driver_check_memory();
                if (count == grid_sqr)
                {
                    i = 0;
                    driver_buzzer(Buzzer::COMPLETE); // finished!!
                }
                else
                {
                    g_evolve_info.x_parameter_range = g_evolve_x_parameter_range;
                    g_evolve_info.y_parameter_range = g_evolve_y_parameter_range;
                    g_evolve_info.x_parameter_offset = g_evolve_x_parameter_offset;
                    g_evolve_info.y_parameter_offset = g_evolve_y_parameter_offset;
                    g_evolve_info.discrete_x_parameter_offset = static_cast<short>(g_evolve_discrete_x_parameter_offset);
                    g_evolve_info.discrete_y_parameter_offset = static_cast<short>(g_evolve_discrete_y_parameter_offset);
                    g_evolve_info.px              = static_cast<short>(g_evolve_param_grid_x);
                    g_evolve_info.py              = static_cast<short>(g_evolve_param_grid_y);
                    g_evolve_info.screen_x_offset          = static_cast<short>(g_logical_screen.x_offset);
                    g_evolve_info.screen_y_offset          = static_cast<short>(g_logical_screen.y_offset);
                    g_evolve_info.x_dots           = static_cast<short>(g_logical_screen.x_dots);
                    g_evolve_info.y_dots           = static_cast<short>(g_logical_screen.y_dots);
                    g_evolve_info.image_grid_size = static_cast<short>(g_evolve_image_grid_size);
                    g_evolve_info.this_generation_random_seed = static_cast<short>(g_evolve_this_generation_random_seed);
                    g_evolve_info.max_random_mutation = g_evolve_max_random_mutation;
                    g_evolve_info.evolving        = static_cast<short>(+g_evolving);
                    g_evolve_info.count          = static_cast<short>(count);
                    g_have_evolve_info = true;
                }
                g_logical_screen.y_offset = 0;
                g_logical_screen.x_offset = 0;
                g_logical_screen.x_dots = g_screen_x_dots;
                g_logical_screen.y_dots = g_screen_y_dots; // otherwise save only saves a sub image and boxes get clipped

                // set up for 1st selected image, this reuses px and py
                g_evolve_param_grid_y = g_evolve_image_grid_size /2;
                g_evolve_param_grid_x = g_evolve_param_grid_y;
                unspiral_map();    // first time called, w/above line sets up array
                restore_param_history();
                fiddle_params(gene, 0);
                copy_genes_to_bank(gene);
            }
            // end of evolution loop
            else
            {
                i = calc_fract();       // draw the fractal using "C"
                if (i == 0)
                {
                    driver_buzzer(Buzzer::COMPLETE); // finished!!
                }
            }

            stop_save_timer();
            if (driver_is_disk() && i == 0) // disk-video
            {
                dvid_status(0, "Image has been completed");
            }
        }
        g_box_count = 0;                     // no zoom box yet
        g_zoom_box_width = 0;

        if (g_fractal_type == FractalType::PLASMA)
        {
            g_cycle_limit = 256;              // plasma clouds need quick spins
            g_dac_count = 256;
        }

resumeloop:                             // return here on failed overlays
        driver_check_memory();
        context.more_keys = true;
        while (context.more_keys)
        {
            // loop through command keys
            if (g_timed_save != TimedSave::NONE)
            {
                if (g_timed_save == TimedSave::STARTED)
                {
                    // woke up for timed save
                    driver_get_key();     // eat the dummy char
                    context.key = 's'; // do the save
                    g_resave_flag = TimedSave::STARTED;
                    g_timed_save = TimedSave::FINAL;
                }
                else
                {
                    // save done, resume
                    g_timed_save = TimedSave::NONE;
                    g_resave_flag = TimedSave::FINAL;
                    context.key = ID_KEY_ENTER;
                }
            }
            else if (g_init_batch == BatchMode::NONE)      // not batch mode
            {
                if (g_calc_status == CalcStatus::RESUMABLE && g_zoom_box_width == 0 && !driver_key_pressed())
                {
                    context.key = ID_KEY_ENTER;  // no visible reason to stop, continue
                }
                else      // wait for a real keystroke
                {
                    if (g_browse.auto_browse && g_browse.sub_images)
                    {
                        context.key = 'l';
                    }
                    else
                    {
                        bool changed{};
                        while (!changed)
                        {
                            driver_wait_key_pressed(true);
                            if (driver_key_pressed())
                            {
                                context.key = driver_get_key();
                                break;
                            }
                            changed = mouse->process_zoom(context);
                        }
                        if (changed)
                        {
                            continue;
                        }
                    }
                    if (context.key == ID_KEY_ESC || context.key == 'm' || context.key == 'M')
                    {
                        if (context.key == ID_KEY_ESC && g_escape_exit)
                        {
                            // don't ask, just get out
                            goodbye();
                        }
                        driver_stack_screen();
                        context.key = main_menu(true);
                        if (context.key == '\\' || context.key == ID_KEY_CTL_BACKSLASH
                            || context.key == 'h' || context.key == ID_KEY_CTL_H
                            || check_vid_mode_key(context.key) >= 0)
                        {
                            driver_discard_screen();
                        }
                        else if (context.key == 'x' || context.key == 'y'
                            || context.key == 'z' || context.key == 'g'
                            || context.key == 'v' || context.key == ID_KEY_CTL_B
                            || context.key == ID_KEY_CTL_E || context.key == ID_KEY_CTL_F)
                        {
                            g_from_text = true;
                        }
                        else
                        {
                            driver_unstack_screen();
                        }
                    }
                }
            }
            else          // batch mode, fake next keystroke
            {
                // clang-format off
                // init_batch == FINISH_CALC_BEFORE_SAVE        flag to finish calc before save
                // init_batch == NONE                           not in batch mode
                // init_batch == NORMAL                         normal batch mode
                // init_batch == SAVE                           was NORMAL, now do a save
                // init_batch == BAILOUT_ERROR_NO_SAVE          bailout with errorlevel == 2, error occurred, no save
                // init_batch == BAILOUT_INTERRUPTED_TRY_SAVE   bailout with errorlevel == 1, interrupted, try to save
                // init_batch == BAILOUT_INTERRUPTED_SAVE       was BAILOUT_INTERRUPTED_TRY_SAVE, now do a save
                // clang-format on

                if (g_init_batch == BatchMode::FINISH_CALC_BEFORE_SAVE)
                {
                    context.key = ID_KEY_ENTER;
                    g_init_batch = BatchMode::NORMAL;
                }
                else if (g_init_batch == BatchMode::NORMAL || g_init_batch == BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE)         // save-to-disk
                {
                    context.key = g_debug_flag == DebugFlags::FORCE_DISK_RESTORE_NOT_SAVE ? 'r' : 's';
                    if (g_init_batch == BatchMode::NORMAL)
                    {
                        g_init_batch = BatchMode::SAVE;
                    }
                    if (g_init_batch == BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE)
                    {
                        g_init_batch = BatchMode::BAILOUT_INTERRUPTED_SAVE;
                    }
                }
                else
                {
                    if (g_calc_status != CalcStatus::COMPLETED)
                    {
                        g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE; // bailout with error
                    }
                    goodbye();               // done, exit
                }
            }

            context.key = std::tolower(context.key);
            if (g_evolving != EvolutionModeFlags::NONE)
            {
                mms_value = evolver_menu_switch(context);
            }
            else
            {
                mms_value = main_menu_switch(context);
            }
            if (g_quick_calc
                && (mms_value == MainState::IMAGE_START
                    || mms_value == MainState::RESTORE_START
                    || mms_value == MainState::RESTART))
            {
                g_quick_calc = false;
                g_user.std_calc_mode = g_old_std_calc_mode;
            }
            if (g_quick_calc && g_calc_status != CalcStatus::COMPLETED)
            {
                g_user.std_calc_mode = CalcMode::ONE_PASS;
            }
            if (mms_value == MainState::IMAGE_START || mms_value == MainState::RESTORE_START ||
                mms_value == MainState::RESTART)
            {
                return mms_value;
            }
            if (mms_value == MainState::CONTINUE)
            {
                continue;
            }
            if (g_zoom_enabled && context.more_keys) // draw/clear a zoom box?
            {
                draw_box(true);
            }
            if (driver_resize())
            {
                g_calc_status = CalcStatus::NO_FRACTAL;
            }
        }
    }
}

static int call_line3d(Byte *pixels, const int line_len)
{
    // this routine exists because line3d might be in an overlay
    return line3d(pixels, line_len);
}

// displays differences between current image file and new image
static int cmp_line(Byte *pixels, const int line_len)
{
    int row = g_row_count++;
    if (row == 0)
    {
        s_err_count = 0;
        s_cmp_fp = dir_fopen(g_working_dir, "cmperr", g_init_batch != BatchMode::NONE ? "a" : "w");
        g_out_line_cleanup = cmp_line_cleanup;
    }
    if (g_potential.store_16bit)
    {
        // 16 bit info, ignore odd numbered rows
        if ((row & 1) != 0)
        {
            return 0;
        }
        row >>= 1;
    }
    for (int col = 0; col < line_len; col++)
    {
        int old_color = get_color(col, row);
        if (old_color == static_cast<int>(pixels[col]))
        {
            g_put_color(col, row, 0);
        }
        else
        {
            if (old_color == 0)
            {
                g_put_color(col, row, 1);
            }
            ++s_err_count;
            if (g_init_batch == BatchMode::NONE)
            {
                fmt::print(s_cmp_fp, "#{:5d} col {:3d} row {:3d} old {:3d} new {:3d}\n", //
                    s_err_count, col, row, old_color, pixels[col]);
            }
        }
    }
    return 0;
}

static void cmp_line_cleanup()
{
    if (g_init_batch != BatchMode::NONE)
    {
        time_t now;
        std::time(&now);
        char *times_text = std::ctime(&now);
        times_text[24] = 0; //clobber newline in time string
        fmt::print(s_cmp_fp, "{:s} compare to {:s} has {:5d} errs\n", //
            times_text, g_read_filename.string(), s_err_count);
    }
    std::fclose(s_cmp_fp);
}

// read keystrokes while = specified key, return 1+count;
// used to catch up when moving zoombox is slower than keyboard
int key_count(const int key)
{
    int ctr = 1;
    while (driver_key_pressed() == key)
    {
        driver_get_key();
        ++ctr;
    }
    return ctr;
}

} // namespace id::ui
