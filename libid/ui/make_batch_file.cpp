// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/make_batch_file.h"

#include "3d/3d.h"
#include "3d/line3d.h"
#include "3d/plot3d.h"
#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/convert_center_mag.h"
#include "engine/get_prec_big_float.h"
#include "engine/id_data.h"
#include "engine/log_map.h"
#include "engine/sticky_orbits.h"
#include "engine/type_has_param.h"
#include "engine/perturbation.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/jb.h"
#include "fractals/lorenz.h"
#include "fractals/parser.h"
#include "helpdefs.h"
#include "io/dir_file.h"
#include "io/ends_with_slash.h"
#include "io/file_gets.h"
#include "io/has_ext.h"
#include "io/is_writeable.h"
#include "io/loadfile.h"
#include "io/save_file.h"
#include "math/big.h"
#include "math/biginit.h"
#include "math/sign.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"
#include "ui/ChoiceBuilder.h"
#include "ui/cmdfiles.h"
#include "ui/comments.h"
#include "ui/rotate.h"
#include "ui/sound.h"
#include "ui/spindac.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"
#include "ui/trig_fns.h"
#include "ui/video_mode.h"

#include <config/string_case_compare.h>
#include <config/string_lower.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{

struct WriteBatchData // buffer for parms to break lines nicely
{
    int len;
    char buf[10000];
};

} // namespace

bool g_make_parameter_file{};
bool g_make_parameter_file_map{};
int g_max_line_length{72};

static std::FILE *s_param_file{};

static void put_param(const char *param, ...);
static void put_param_line();
static void put_float(int slash, double value, int prec);
static void put_bf(int slash, BigFloat r, int prec);
static void put_file_name(const char *keyword, const char *fname);
static void strip_zeros(char *buf);
static void write_batch_params(const char *color_inf, bool colors_only, int max_color, int ii, int jj);

static char par_key(int x)
{
    return static_cast<char>(x < 10 ? '0' + x : 'a' - 10 + x);
}

struct MakeParParams
{
    MakeParParams();
    bool prompt();

    bool colors_only{g_make_parameter_file_map}; // makepar map case
    char input_command_file[80];
    char input_command_name[ITEM_NAME_LEN + 1];
    char input_comment[4][MAX_COMMENT_LEN];
    char color_spec[14]{};
    int max_color{g_colors};
    int max_line_length{g_max_line_length};
    int x_multiple{1};
    int y_multiple{1};
    char video_mode_key_name[5];

    int piece_x_dots{g_logical_screen_x_dots};
    int piece_y_dots{g_logical_screen_y_dots};

private:
    bool m_prompt_record_colors{
        g_got_real_dac || (g_is_true_color && g_true_mode == TrueColorMode::DEFAULT_COLOR)};
};

MakeParParams::MakeParParams()
{
    std::strcpy(color_spec, "y");
    if (m_prompt_record_colors)
    {
        --max_color;
        if (g_inside_color > COLOR_BLACK && g_inside_color > max_color)
        {
            max_color = g_inside_color;
        }
        if (g_outside_color > COLOR_BLACK && g_outside_color > max_color)
        {
            max_color = g_outside_color;
        }
        if (g_distance_estimator < COLOR_BLACK && -g_distance_estimator > max_color)
        {
            max_color = (int) (0 - g_distance_estimator);
        }
        if (g_decomp[0] > max_color)
        {
            max_color = g_decomp[0] - 1;
        }
        if (g_potential_flag && g_potential_params[0] >= max_color)
        {
            max_color = (int) g_potential_params[0];
        }
        if (++max_color > 256)
        {
            max_color = 256;
        }

        const char *str_ptr{};
        if (g_color_state == ColorState::DEFAULT)
        {
            // default colors
            if (g_map_specified)
            {
                color_spec[0] = '@';
                str_ptr = g_map_name.c_str();
            }
        }
        else if (g_color_state == ColorState::MAP_FILE)
        {
            // colors match colorfile
            color_spec[0] = '@';
            str_ptr = g_color_file.c_str();
        }
        else // colors match no .map that we know of
        {
            std::strcpy(color_spec, "y");
        }

        if (str_ptr && color_spec[0] == '@')
        {
            const char *str_ptr2 = std::strrchr(str_ptr, SLASH_CH);
            if (str_ptr2 != nullptr)
            {
                str_ptr = str_ptr2 + 1;
            }
            str_ptr2 = std::strrchr(str_ptr, ':');
            if (str_ptr2 != nullptr)
            {
                str_ptr = str_ptr2 + 1;
            }
            std::strncpy(&color_spec[1], str_ptr, 12);
            color_spec[13] = 0;
        }
    }
    std::strcpy(input_command_file, g_command_file.c_str());
    std::strcpy(input_command_name, g_command_name.c_str());
    for (int i = 0; i < 4; i++)
    {
        std::strcpy(input_comment[i], expand_command_comment(i).c_str());
    }

    if (g_command_name.empty())
    {
        std::strcpy(input_command_name, "test");
    }
    vid_mode_key_name(g_video_entry.key, video_mode_key_name);
}

bool MakeParParams::prompt()
{
    while (true)
    {
        constexpr int MAX_PROMPTS{18};
        ChoiceBuilder<MAX_PROMPTS> builder;
        builder.string_buff("Parameter file", input_command_file, MAX_COMMENT_LEN - 1);
        builder.string_buff("Name", input_command_name, ITEM_NAME_LEN);
        builder.string_buff("Main comment", input_comment[0], MAX_COMMENT_LEN - 1);
        builder.string_buff("Second comment", input_comment[1], MAX_COMMENT_LEN - 1);
        builder.string_buff("Third comment", input_comment[2], MAX_COMMENT_LEN - 1);
        builder.string_buff("Fourth comment", input_comment[3], MAX_COMMENT_LEN - 1);
        if (m_prompt_record_colors)
        {
            builder.string_buff("Record colors?", color_spec, 13);
            builder.comment("    (no | yes | only for full info | @filename to point to a map file)");
            builder.int_number("# of colors", max_color);
            builder.comment("    (if recording full color info)");
        }
        builder.int_number("Maximum line length", g_max_line_length);
        builder.comment("");
        builder.comment("    **** The following is for generating images in pieces ****");
        builder.int_number("X multiple", x_multiple);
        builder.int_number("Y multiple", y_multiple);
        builder.string_buff("Video mode", video_mode_key_name, 4);
        if (builder.prompt("Save Current Parameters") < 0)
        {
            return true;
        }

        g_command_file = builder.read_string_buff(MAX_COMMENT_LEN - 1);
        if (has_ext(g_command_file.c_str()) == nullptr)
        {
            g_command_file += ".par";   // default extension .par
        }
        g_command_name = builder.read_string_buff(ITEM_NAME_LEN);
        for (std::string &comment : g_command_comment)
        {
            comment = builder.read_string_buff(MAX_COMMENT_LEN - 1);
        }
        if (m_prompt_record_colors)
        {
            std::string requested_colors{builder.read_string_buff(13)};
            if (requested_colors[0] == 'o' || g_make_parameter_file_map)
            {
                requested_colors = "y";
                colors_only = true;
            }
            builder.read_comment();
            if (int num_colors{builder.read_int_number()}; num_colors > 0 && num_colors <= 256)
            {
                max_color = num_colors;
            }
            builder.read_comment();
        }
        if (int value = builder.read_int_number();
            g_max_line_length != value && value >= MIN_MAX_LINE_LENGTH && value <= MAX_MAX_LINE_LENGTH)
        {
            g_max_line_length = value;
        }
        builder.read_comment();
        builder.read_comment();
        x_multiple = builder.read_int_number();
        y_multiple = builder.read_int_number();

        // sanity checks
        // get resolution from the video name (which must be valid)
        piece_y_dots = 0;
        piece_x_dots = 0;
        if (int key = check_vid_mode_key_name(builder.read_string_buff(4)); key > 0)
        {
            if (int i = check_vid_mode_key(key); i >= 0)
            {
                // get the resolution of this video mode
                piece_x_dots = g_video_table[i].x_dots;
                piece_y_dots = g_video_table[i].y_dots;
            }
        }
        if (piece_x_dots == 0 && (x_multiple > 1 || y_multiple > 1))
        {
            // no corresponding video mode!
            stop_msg("Invalid video mode entry!");
            continue;
        }

        // bounds range on xm, ym
        if (x_multiple < 1 || x_multiple > 36 || y_multiple < 1 || y_multiple > 36)
        {
            stop_msg("X and Y multiple must be 1 to 36");
            continue;
        }

        // another sanity check: total resolution cannot exceed 65535
        if (x_multiple * piece_x_dots > 65535L || y_multiple * piece_y_dots > 65535L)
        {
            stop_msg("Total resolution (X or Y) cannot exceed 65535");
            continue;
        }

        return false;
    }
}

void make_batch_file()
{
    // added for pieces feature
    double piece_delta_x{};
    double piece_delta_y{};
    double piece_delta_x2{};
    double piece_delta_y2{};
    double piece_x_min{};
    double piece_y_min{};
    bool have_3rd{};
    fs::path in_path;
    char buf2[128];
    std::FILE *infile{};
    std::FILE *bat_file{};

    driver_stack_screen();
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_PARAM_FILE};
    MakeParParams params;

    if (g_make_parameter_file)
    {
        goto skip_ui;
    }

    while (true)
    {
prompt_user:
        if (params.prompt())
        {
            break;
        }
skip_ui:
        if (g_make_parameter_file)
        {
            if (g_file_colors > 0)
            {
                std::strcpy(params.color_spec, "y");
            }
            else
            {
                std::strcpy(params.color_spec, "n");
            }
            if (g_make_parameter_file_map)
            {
                params.max_color = 256;
            }
            else
            {
                params.max_color = g_file_colors;
            }
        }
        fs::path out_path = get_save_name(g_command_file);
        if (exists(out_path))
        {
            // file exists
            if (!is_writeable(out_path))
            {
                stop_msg("Can't write " + out_path.string());
                continue;
            }
            in_path = out_path;
            infile = std::fopen(in_path.string().c_str(), "rt");
            out_path.replace_filename("id.tmp");
        }
        s_param_file = std::fopen(out_path.string().c_str(), "wt");
        if (s_param_file == nullptr)
        {
            stop_msg("Can't create " + out_path.string());
            if (infile != nullptr)
            {
                std::fclose(infile);
            }
            continue;
        }

        if (infile != nullptr)
        {
            char line[256];
            while (file_gets(line, 255, infile) >= 0)
            {
                if (std::strchr(line, '{')                     // entry heading?
                    && std::sscanf(line, " %40[^ \t({]", buf2) //
                    && string_case_equal(buf2, g_command_name.c_str()))
                {
                    // entry with same name
                    std::snprintf(buf2, std::size(buf2), "File already has an entry named %s\n%s",
                    g_command_name.c_str(), g_make_parameter_file ?
                    "... Replacing ..." : "Continue to replace it, Cancel to back out");
                    if (stop_msg(StopMsgFlags::CANCEL | StopMsgFlags::INFO_ONLY, buf2))
                    {
                        // cancel
                        std::fclose(infile);
                        infile = nullptr;
                        std::fclose(s_param_file);
                        s_param_file = nullptr;
                        remove(out_path);
                        goto prompt_user;
                    }
                    while (std::strchr(line, '}') == nullptr && file_gets(line, 255, infile) > 0)
                    {
                        // skip to end of set
                    }
                    break;
                }
                std::fputs(line, s_param_file);
                std::fputc('\n', s_param_file);
            }
        }
        //**** start here
        if (params.x_multiple > 1 || params.y_multiple > 1)
        {
            have_3rd = g_x_min != g_x_3rd || g_y_min != g_y_3rd;
            bat_file = open_save_file("makemig.bat", "w");
            if (bat_file == nullptr)
            {
                params.x_multiple = 0;
                params.y_multiple = 0;
            }
            piece_delta_x  = (g_x_max - g_x_3rd) / (params.x_multiple * params.piece_x_dots - 1);   // calculate step sizes
            piece_delta_y  = (g_y_max - g_y_3rd) / (params.y_multiple * params.piece_y_dots - 1);
            piece_delta_x2 = (g_x_3rd - g_x_min) / (params.y_multiple * params.piece_y_dots - 1);
            piece_delta_y2 = (g_y_3rd - g_y_min) / (params.x_multiple * params.piece_x_dots - 1);

            // save corners
            piece_x_min = g_x_min;
            piece_y_min = g_y_max;
        }
        for (int col = 0; col < params.x_multiple; col++)
        {
            for (int row = 0; row < params.y_multiple; row++)
            {
                if (params.x_multiple > 1 || params.y_multiple > 1)
                {
                    char piece_command_name[80];
                    int w{};
                    while (w < (int)g_command_name.length())
                    {
                        const char c = g_command_name[w];
                        if (std::isspace(c) || c == 0)
                        {
                            break;
                        }
                        piece_command_name[w] = c;
                        w++;
                    }
                    piece_command_name[w] = 0;
                    {
                        char tmp_buff[20];
                        std::snprintf(tmp_buff, std::size(tmp_buff), "_%c%c", par_key(col), par_key(row));
                        std::strcat(piece_command_name, tmp_buff);
                    }
                    std::fprintf(s_param_file, "%-19s{", piece_command_name);
                    g_x_min = piece_x_min + piece_delta_x*(col*params.piece_x_dots) + piece_delta_x2*(row*params.piece_y_dots);
                    g_x_max = piece_x_min + piece_delta_x*((col+1)*params.piece_x_dots - 1) + piece_delta_x2*((row+1)*params.piece_y_dots - 1);
                    g_y_min = piece_y_min - piece_delta_y*((row+1)*params.piece_y_dots - 1) - piece_delta_y2*((col+1)*params.piece_x_dots - 1);
                    g_y_max = piece_y_min - piece_delta_y*(row*params.piece_y_dots) - piece_delta_y2*(col*params.piece_x_dots);
                    if (have_3rd)
                    {
                        g_x_3rd = piece_x_min + piece_delta_x*(col*params.piece_x_dots) + piece_delta_x2*((row+1)*params.piece_y_dots - 1);
                        g_y_3rd = piece_y_min - piece_delta_y*((row+1)*params.piece_y_dots - 1) - piece_delta_y2*(col*params.piece_x_dots);
                    }
                    else
                    {
                        g_x_3rd = g_x_min;
                        g_y_3rd = g_y_min;
                    }
                    std::fprintf(bat_file, "start/wait id batch=yes overwrite=yes @%s/%s\n", g_command_file.c_str(), piece_command_name);
                    std::fprintf(bat_file, "if errorlevel 2 goto oops\n");
                }
                else
                {
                    std::fprintf(s_param_file, "%-19s{", g_command_name.c_str());
                }
                {
                    /* guarantee that there are no blank comments above the last
                       non-blank par_comment */
                    int last = -1;
                    for (int k = 0; k < 4; k++)
                    {
                        if (*g_par_comment[k])
                        {
                            last = k;
                        }
                    }
                    for (int k = 0; k < last; k++)
                    {
                        if (g_command_comment[k].empty())
                        {
                            g_command_comment[k] = ";";
                        }
                    }
                }
                if (!g_command_comment[0].empty())
                {
                    std::fprintf(s_param_file, " ; %s", g_command_comment[0].c_str());
                }
                std::fputc('\n', s_param_file);
                {
                    char tmp_buff[25];
                    std::memset(tmp_buff, ' ', 23);
                    tmp_buff[23] = 0;
                    tmp_buff[21] = ';';
                    for (int k = 1; k < 4; k++)
                    {
                        if (!g_command_comment[k].empty())
                        {
                            std::fprintf(s_param_file, "%s%s\n", tmp_buff, g_command_comment[k].c_str());
                        }
                    }
                    if (g_patch_level != 0 && !params.colors_only)
                    {
                        std::fprintf(s_param_file, "%s id Version %d Patchlevel %d\n", tmp_buff, g_release, g_patch_level);
                    }
                }
                write_batch_params(params.color_spec, params.colors_only, params.max_color, col, row);
                if (params.x_multiple > 1 || params.y_multiple > 1)
                {
                    std::fprintf(s_param_file, "  video=%s", params.video_mode_key_name);
                    std::fprintf(s_param_file, " savename=frmig_%c%c\n", par_key(col), par_key(row));
                }
                std::fprintf(s_param_file, "}\n\n");
            }
        }
        if (params.x_multiple > 1 || params.y_multiple > 1)
        {
            std::fprintf(bat_file, "start/wait id makemig=%d/%d\n", params.x_multiple, params.y_multiple);
            std::fprintf(bat_file, ":oops\n");
            std::fclose(bat_file);
        }
        //******end here

        if (infile != nullptr)
        {
            // copy the rest of the file
            char line[256];
            int i;
            do
            {
                i = file_gets(line, 255, infile);
            }
            while (i == 0); // skip blanks
            while (i >= 0)
            {
                std::fputs(line, s_param_file);
                std::fputc('\n', s_param_file);
                i = file_gets(line, 255, infile);
            }
            std::fclose(infile);
        }
        std::fclose(s_param_file);
        if (infile != nullptr)
        {
            // replace the original file with the new
            remove(in_path);           // success assumed on these lines
            rename(out_path, in_path); // since we checked earlier
        }
        break;
    }
    driver_unstack_screen();
}

static WriteBatchData s_wb_data;

static int get_prec(double a, double b, double c)
{
    double high_v = 1.0E20;
    double diff = std::abs(a - b);
    if (diff == 0.0)
    {
        diff = high_v;
    }
    double temp = std::abs(a - c);
    if (temp == 0.0)
    {
        temp = high_v;
    }
    diff = std::min(temp, diff);
    temp = std::abs(b - c);
    if (temp == 0.0)
    {
        temp = high_v;
    }
    diff = std::min(temp, diff);
    int digits = 7;
    if (g_debug_flag >= DebugFlags::FORCE_PRECISION_0_DIGITS &&
        g_debug_flag < DebugFlags::FORCE_PRECISION_20_DIGITS)
    {
        digits = +g_debug_flag - +DebugFlags::FORCE_PRECISION_0_DIGITS;
    }
    while (diff < 1.0 && digits <= DBL_DIG+1)
    {
        diff *= 10;
        ++digits;
    }
    return digits;
}

static void write_batch_params(const char *color_inf, bool colors_only, int max_color, int ii, int jj)
{
    char buf[81];
    BigFloat bf_x_ctr = nullptr;
    BigFloat bf_y_ctr = nullptr;
    int saved = save_stack();
    if (g_bf_math != BFMathType::NONE)
    {
        bf_x_ctr = alloc_stack(g_bf_length+2);
        bf_y_ctr = alloc_stack(g_bf_length+2);
    }

    s_wb_data.len = 0; // force first parm to start on new line

    // Using near string g_box_x for buffer after saving to extraseg

    if (colors_only)
    {
        goto docolors;
    }
    if (g_display_3d <= Display3DMode::NONE)
    {
        // a fractal was generated

        //***** fractal only parameters in this section ******
        put_param(" reset=%s", to_par_string(g_version).c_str());

        put_param(" type=%s", g_cur_fractal_specific->name);

        if (g_fractal_type == FractalType::JULIBROT)
        {
            put_param(" %s=%.15g/%.15g/%.15g/%.15g",
                     "julibrotfromto", g_julibrot_x_max, g_julibrot_x_min, g_julibrot_y_max, g_julibrot_y_min);
            // these rarely change
            if (g_julibrot_origin_fp != 8
                || g_julibrot_height_fp != 7
                || g_julibrot_width_fp != 10
                || g_julibrot_dist_fp != 24
                || g_julibrot_depth_fp != 8
                || g_julibrot_z_dots != 128)
            {
                put_param(" %s=%d/%g/%g/%g/%g/%g", "julibrot3d",
                         g_julibrot_z_dots, g_julibrot_origin_fp, g_julibrot_depth_fp, g_julibrot_height_fp, g_julibrot_width_fp, g_julibrot_dist_fp);
            }
            if (g_eyes_fp != 0)
            {
                put_param(" %s=%g", "julibroteyes", g_eyes_fp);
            }
            if (g_new_orbit_type != FractalType::JULIA)
            {
                put_param(" orbitname=%s", get_fractal_specific(g_new_orbit_type)->name);
            }
            if (g_julibrot_3d_mode != Julibrot3DMode::MONOCULAR)
            {
                put_param(" %s=%s", "3dmode", to_string(g_julibrot_3d_mode));
            }
        }
        if (g_fractal_type == FractalType::FORMULA)
        {
            put_file_name("formulafile", g_formula_filename.c_str());
            put_param(" %s=%s", "formulaname", g_formula_name.c_str());
            if (g_frm_uses_ismand)
            {
                put_param(" %s=%c", "ismand", g_is_mandelbrot ? 'y' : 'n');
            }
        }
        if (g_fractal_type == FractalType::L_SYSTEM)
        {
            put_file_name("lfile", g_l_system_filename.c_str());
            put_param(" %s=%s", "lname", g_l_system_name.c_str());
        }
        if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
        {
            put_file_name("ifsfile", g_ifs_filename.c_str());
            put_param(" %s=%s", "ifs", g_ifs_name.c_str());
        }
        if (g_fractal_type == FractalType::INVERSE_JULIA)
        {
            put_param(" %s=%s/%s", "miim", to_string(g_major_method), to_string(g_inverse_julia_minor_method));
        }

        strncpy(buf, show_trig().c_str(), std::size(buf));
        if (buf[0])
        {
            put_param(buf);
        }

        if (g_user_std_calc_mode != 'g')
        {
            put_param(" %s=%c", "passes", g_user_std_calc_mode);
        }

        if (g_stop_pass != 0)
        {
            put_param(" %s=%c%c", "passes", g_user_std_calc_mode, (char)g_stop_pass + '0');
        }

        if (g_perturbation != PerturbationMode::AUTO)
        {
            put_param(" %s=%s", "perturbation", (g_perturbation == PerturbationMode::YES) ? "yes" : "no");
        }

        if (g_perturbation_tolerance != 1e-6)
        {
            put_param(" %s=%lg", "tolerance", g_perturbation_tolerance);
        }

        if (g_use_center_mag)
        {
            LDouble magnification;
            double x_mag_factor;
            double rotation;
            double skew;
            if (g_bf_math != BFMathType::NONE)
            {
                cvt_center_mag_bf(bf_x_ctr, bf_y_ctr, magnification, x_mag_factor, rotation, skew);
                int digits = get_prec_bf(ResolutionFlag::MAX);
                put_param(" %s=", "center-mag");
                put_bf(0, bf_x_ctr, digits);
                put_bf(1, bf_y_ctr, digits);
            }
            else // !g_bf_math
            {
                double x_ctr;
                double y_ctr;
                cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
                put_param(" %s=", "center-mag");
                //          convert 1000 fudged long to double, 1000/1<<24 = 6e-5
                put_param(g_delta_min > 6e-5 ? "%g/%g" : "%+20.17lf/%+20.17lf", x_ctr, y_ctr);
            }
            put_param("/%.7Lg", magnification); // precision of magnification not critical, but magnitude is
            // Round to avoid ugly decimals, precision here is not critical
            // Don't round x_mag_factor if it's small
            if (std::abs(x_mag_factor) > 0.5)   // or so, exact value isn't important
            {
                x_mag_factor = sign(x_mag_factor) * std::lround(std::abs(x_mag_factor) * 1e4) / 1e4;
            }
            // Just truncate these angles.  Who cares about 1/1000 of a degree
            // Somebody does.  Some rotated and/or skewed images are slightly
            // off when recreated from a PAR using 1/1000.
            if (x_mag_factor != 1 || rotation != 0 || skew != 0)
            {
                // Only put what is necessary
                // The difference with x_mag_factor is that it is normally
                // near 1 while the others are normally near 0
                if (std::abs(x_mag_factor) >= 1)
                {
                    put_float(1, x_mag_factor, 5); // put_float() uses %g
                }
                else     // abs(x_mag_factor) is < 1
                {
                    put_float(1, x_mag_factor, 4); // put_float() uses %g
                }
                if (rotation != 0 || skew != 0)
                {
                    // Use precision=6 here.  These angle have already been rounded
                    // to 3 decimal places, but angles like 123.456 degrees need 6
                    // sig figs to get 3 decimal places.  Trailing 0's are dropped anyway.
                    put_float(1, rotation, 18);
                    if (skew != 0)
                    {
                        put_float(1, skew, 18);
                    }
                }
            }
        }
        else // not usemag
        {
            put_param(" %s=", "corners");
            if (g_bf_math != BFMathType::NONE)
            {
                int digits = get_prec_bf(ResolutionFlag::MAX);
                put_bf(0, g_bf_x_min, digits);
                put_bf(1, g_bf_x_max, digits);
                put_bf(1, g_bf_y_min, digits);
                put_bf(1, g_bf_y_max, digits);
                if (cmp_bf(g_bf_x_3rd, g_bf_x_min) || cmp_bf(g_bf_y_3rd, g_bf_y_min))
                {
                    put_bf(1, g_bf_x_3rd, digits);
                    put_bf(1, g_bf_y_3rd, digits);
                }
            }
            else
            {
                int x_digits = get_prec(g_x_min, g_x_max, g_x_3rd);
                int y_digits = get_prec(g_y_min, g_y_max, g_y_3rd);
                put_float(0, g_x_min, x_digits);
                put_float(1, g_x_max, x_digits);
                put_float(1, g_y_min, y_digits);
                put_float(1, g_y_max, y_digits);
                if (g_x_3rd != g_x_min || g_y_3rd != g_y_min)
                {
                    put_float(1, g_x_3rd, x_digits);
                    put_float(1, g_y_3rd, y_digits);
                }
            }
        }

        int i;
        for (i = (MAX_PARAMS-1); i >= 0; --i)
        {
            if (type_has_param(g_fractal_type == FractalType::JULIBROT ? g_new_orbit_type : g_fractal_type,
                    i, nullptr))
            {
                break;
            }
        }

        if (i >= 0)
        {
            if (g_fractal_type == FractalType::CELLULAR || g_fractal_type == FractalType::ANT)
            {
                put_param(" %s=%.1f", "params", g_params[0]);
            }
            else
            {
                if (g_debug_flag == DebugFlags::FORCE_LONG_DOUBLE_PARAM_OUTPUT)
                {
                    put_param(" %s=%.17Lg", "params", (long double)g_params[0]);
                }
                else
                {
                    put_param(" %s=%.17g", "params", g_params[0]);
                }
            }
            for (int j = 1; j <= i; ++j)
            {
                if (g_fractal_type == FractalType::CELLULAR || g_fractal_type == FractalType::ANT)
                {
                    put_param("/%.1f", g_params[j]);
                }
                else
                {
                    if (g_debug_flag == DebugFlags::FORCE_LONG_DOUBLE_PARAM_OUTPUT)
                    {
                        put_param("/%.17Lg", (long double)g_params[j]);
                    }
                    else
                    {
                        put_param("/%.17g", g_params[j]);
                    }
                }
            }
        }

        if (g_use_init_orbit == InitOrbitMode::PIXEL)
        {
            put_param(" %s=pixel", "initorbit");
        }
        else if (g_use_init_orbit == InitOrbitMode::VALUE)
        {
            put_param(" %s=%.15g/%.15g", "initorbit", g_init_orbit.x, g_init_orbit.y);
        }

        if (g_max_iterations != 150)
        {
            put_param(" %s=%ld", "maxiter", g_max_iterations);
        }

        if (g_bailout && (!g_potential_flag || g_potential_params[2] == 0.0))
        {
            put_param(" %s=%ld", "bailout", g_bailout);
        }

        if (g_bailout_test != Bailout::MOD)
        {
            put_param(" %s=", "bailoutest");
            if (g_bailout_test == Bailout::REAL)
            {
                put_param("real");
            }
            else if (g_bailout_test == Bailout::IMAG)
            {
                put_param("imag");
            }
            else if (g_bailout_test == Bailout::OR)
            {
                put_param("or");
            }
            else if (g_bailout_test == Bailout::AND)
            {
                put_param("and");
            }
            else if (g_bailout_test == Bailout::MANH)
            {
                put_param("manh");
            }
            else if (g_bailout_test == Bailout::MANR)
            {
                put_param("manr");
            }
            else
            {
                put_param("mod"); // default, just in case
            }
        }
        if (g_fill_color != -1)
        {
            put_param(" %s=", "fillcolor");
            put_param("%d", g_fill_color);
        }
        if (g_inside_color != 1)
        {
            put_param(" %s=", "inside");
            if (g_inside_color == ITER)
            {
                put_param("maxiter");
            }
            else if (g_inside_color == ZMAG)
            {
                put_param("zmag");
            }
            else if (g_inside_color == BOF60)
            {
                put_param("bof60");
            }
            else if (g_inside_color == BOF61)
            {
                put_param("bof61");
            }
            else if (g_inside_color == EPS_CROSS)
            {
                put_param("epsiloncross");
            }
            else if (g_inside_color == STAR_TRAIL)
            {
                put_param("startrail");
            }
            else if (g_inside_color == PERIOD)
            {
                put_param("period");
            }
            else if (g_inside_color == FMODI)
            {
                put_param("fmod");
            }
            else if (g_inside_color == ATANI)
            {
                put_param("atan");
            }
            else
            {
                put_param("%d", g_inside_color);
            }
        }
        if (g_close_proximity != 0.01
            && (g_inside_color == EPS_CROSS || g_inside_color == FMODI || g_outside_color == FMOD))
        {
            put_param(" %s=%.15g", "proximity", g_close_proximity);
        }
        if (g_outside_color != ITER)
        {
            put_param(" %s=", "outside");
            if (g_outside_color == REAL)
            {
                put_param("real");
            }
            else if (g_outside_color == IMAG)
            {
                put_param("imag");
            }
            else if (g_outside_color == MULT)
            {
                put_param("mult");
            }
            else if (g_outside_color == SUM)
            {
                put_param("summ");
            }
            else if (g_outside_color == ATAN)
            {
                put_param("atan");
            }
            else if (g_outside_color == FMOD)
            {
                put_param("fmod");
            }
            else if (g_outside_color == TDIS)
            {
                put_param("tdis");
            }
            else
            {
                put_param("%d", g_outside_color);
            }
        }

        if (g_log_map_flag && !g_iteration_ranges_len)
        {
            put_param(" %s=", "logmap");
            if (g_log_map_flag == -1)
            {
                put_param("old");
            }
            else if (g_log_map_flag == 1)
            {
                put_param("yes");
            }
            else
            {
                put_param("%ld", g_log_map_flag);
            }
        }

        if (g_log_map_fly_calculate && g_log_map_flag && !g_iteration_ranges_len)
        {
            put_param(" %s=", "logmode");
            if (g_log_map_fly_calculate == 1)
            {
                put_param("fly");
            }
            else if (g_log_map_fly_calculate == 2)
            {
                put_param("table");
            }
        }

        if (g_potential_flag)
        {
            put_param(" %s=%d/%g/%d", "potential",
                     (int)g_potential_params[0], g_potential_params[1], (int)g_potential_params[2]);
            if (g_potential_16bit)
            {
                put_param("/%s", "16bit");
            }
        }
        if (g_invert != 0)
        {
            put_param(" %s=%-1.15lg/%-1.15lg/%-1.15lg", "invert",
                     g_inversion[0], g_inversion[1], g_inversion[2]);
        }
        if (g_decomp[0])
        {
            put_param(" %s=%d", "decomp", g_decomp[0]);
        }
        if (g_distance_estimator)
        {
            put_param(" %s=%ld/%d/%d/%d", "distest", g_distance_estimator, g_distance_estimator_width_factor,
                     g_distance_estimator_x_dots?g_distance_estimator_x_dots:g_logical_screen_x_dots, g_distance_estimator_y_dots?g_distance_estimator_y_dots:g_logical_screen_y_dots);
        }
        if (g_old_demm_colors)
        {
            put_param(" %s=y", "olddemmcolors");
        }
        if (g_user_biomorph_value != -1)
        {
            put_param(" %s=%d", "biomorph", g_user_biomorph_value);
        }
        if (g_finite_attractor)
        {
            put_param(" %s=y", "finattract");
        }

        if (g_force_symmetry != SymmetryType::NOT_FORCED)
        {
            if (g_force_symmetry == static_cast<SymmetryType>(1000) && ii == 1 && jj == 1)
            {
                stop_msg("Regenerate before <b> to get correct symmetry");
            }
            put_param(" %s=", "symmetry");
            if (g_force_symmetry == SymmetryType::X_AXIS)
            {
                put_param("xaxis");
            }
            else if (g_force_symmetry == SymmetryType::Y_AXIS)
            {
                put_param("yaxis");
            }
            else if (g_force_symmetry == SymmetryType::XY_AXIS)
            {
                put_param("xyaxis");
            }
            else if (g_force_symmetry == SymmetryType::ORIGIN)
            {
                put_param("origin");
            }
            else if (g_force_symmetry == SymmetryType::PI_SYM)
            {
                put_param("pi");
            }
            else
            {
                put_param("none");
            }
        }

        if (g_periodicity_check != 1)
        {
            put_param(" %s=%d", "periodicity", g_periodicity_check);
        }

        if (g_random_seed_flag)
        {
            put_param(" %s=%d", "rseed", g_random_seed);
        }

        if (g_iteration_ranges_len)
        {
            put_param(" %s=", "ranges");
            i = 0;
            while (i < g_iteration_ranges_len)
            {
                if (i)
                {
                    put_param("/");
                }
                if (g_iteration_ranges[i] == -1)
                {
                    put_param("-%d/", g_iteration_ranges[++i]);
                    ++i;
                }
                put_param("%d", g_iteration_ranges[i++]);
            }
        }
    }

    if (g_display_3d >= Display3DMode::YES)
    {
        //**** 3d transform only parameters in this section ****
        if (g_display_3d == Display3DMode::B_COMMAND)
        {
            put_param(" %s=%s", "3d", "overlay");
        }
        else
        {
            put_param(" %s=%s", "3d", "yes");
        }
        if (!g_loaded_3d)
        {
            put_file_name("filename", g_read_filename.c_str());
        }
        if (g_sphere)
        {
            put_param(" %s=y", "sphere");
            put_param(" %s=%d/%d", "latitude", g_sphere_theta_min, g_sphere_theta_max);
            put_param(" %s=%d/%d", "longitude", g_sphere_phi_min, g_sphere_phi_max);
            put_param(" %s=%d", "radius", g_sphere_radius);
        }
        put_param(" %s=%d/%d", "scalexyz", g_x_scale, g_y_scale);
        put_param(" %s=%d", "roughness", g_rough);
        put_param(" %s=%d", "waterline", g_water_line);
        if (g_fill_type != FillType::POINTS)
        {
            put_param(" %s=%d", "filltype", +g_fill_type);
        }
        if (g_transparent_color_3d[0] || g_transparent_color_3d[1])
        {
            put_param(" %s=%d/%d", "transparent", g_transparent_color_3d[0], g_transparent_color_3d[1]);
        }
        if (g_preview)
        {
            put_param(" %s=%s", "preview", "yes");
            if (g_show_box)
            {
                put_param(" %s=%s", "showbox", "yes");
            }
            put_param(" %s=%d", "coarse", g_preview_factor);
        }
        if (g_raytrace_format != RayTraceFormat::NONE)
        {
            put_param(" %s=%d", "ray", static_cast<int>(g_raytrace_format));
            if (g_brief)
            {
                put_param(" %s=y", "brief");
            }
        }
        if (g_fill_type > FillType::SOLID_FILL)
        {
            put_param(" %s=%d/%d/%d", "lightsource", g_light_x, g_light_y, g_light_z);
            if (g_light_avg)
            {
                put_param(" %s=%d", "smoothing", g_light_avg);
            }
        }
        if (g_randomize_3d)
        {
            put_param(" %s=%d", "randomize", g_randomize_3d);
        }
        if (g_targa_out)
        {
            put_param(" %s=y", "fullcolor");
        }
        if (g_gray_flag)
        {
            put_param(" %s=y", "usegrayscale");
        }
        if (g_ambient)
        {
            put_param(" %s=%d", "ambient", g_ambient);
        }
        if (g_haze)
        {
            put_param(" %s=%d", "haze", g_haze);
        }
        if (g_background_color[0] != 51 || g_background_color[1] != 153 || g_background_color[2] != 200)
        {
            put_param(" %s=%d/%d/%d", "background", g_background_color[0], g_background_color[1],
                     g_background_color[2]);
        }
    }

    if (g_display_3d != Display3DMode::NONE)
    {
        // universal 3d
        //**** common (fractal & transform) 3d parameters in this section ****
        if (!g_sphere || g_display_3d < Display3DMode::NONE)
        {
            put_param(" %s=%d/%d/%d", "rotation", g_x_rot, g_y_rot, g_z_rot);
        }
        put_param(" %s=%d", "perspective", g_viewer_z);
        put_param(" %s=%d/%d", "xyshift", g_shift_x, g_shift_y);
        if (g_adjust_3d_x || g_adjust_3d_y)
        {
            put_param(" %s=%d/%d", "xyadjust", g_adjust_3d_x, g_adjust_3d_y);
        }
        if (g_glasses_type != GlassesType::NONE)
        {
            put_param(" %s=%d", "stereo", static_cast<int>(g_glasses_type));
            put_param(" %s=%d", "interocular", g_eye_separation);
            put_param(" %s=%d", "converge", g_converge_x_adjust);
            put_param(" %s=%d/%d/%d/%d", "crop",
                     g_red_crop_left, g_red_crop_right, g_blue_crop_left, g_blue_crop_right);
            put_param(" %s=%d/%d", "bright",
                     g_red_bright, g_blue_bright);
        }
    }

    //**** universal parameters in this section ****

    if (g_view_window)
    {
        put_param(" %s=%g/%g", "viewwindows", g_view_reduction, g_final_aspect_ratio);
        if (g_view_crop)
        {
            put_param("/%s", "yes");
        }
        else
        {
            put_param("/%s", "no");
        }
        put_param("/%d/%d", g_view_x_dots, g_view_y_dots);
    }

    if (!colors_only)
    {
        if (g_color_cycle_range_lo != 1 || g_color_cycle_range_hi != 255)
        {
            put_param(" %s=%d/%d", "cyclerange", g_color_cycle_range_lo, g_color_cycle_range_hi);
        }

        if (g_base_hertz != 440)
        {
            put_param(" %s=%d", "hertz", g_base_hertz);
        }

        if (g_sound_flag != (SOUNDFLAG_BEEP | SOUNDFLAG_SPEAKER))
        {
            if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_OFF)
            {
                put_param(" %s=%s", "sound", "off");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_BEEP)
            {
                put_param(" %s=%s", "sound", "beep");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X)
            {
                put_param(" %s=%s", "sound", "x");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Y)
            {
                put_param(" %s=%s", "sound", "y");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Z)
            {
                put_param(" %s=%s", "sound", "z");
            }
            if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) <= SOUNDFLAG_Z)
            {
                if (g_sound_flag & SOUNDFLAG_SPEAKER)
                {
                    put_param("/pc");
                }
                if (g_sound_flag & SOUNDFLAG_OPL3_FM)
                {
                    put_param("/fm");
                }
                if (g_sound_flag & SOUNDFLAG_MIDI)
                {
                    put_param("/midi");
                }
                if (g_sound_flag & SOUNDFLAG_QUANTIZED)
                {
                    put_param("/quant");
                }
            }
        }

        if (g_fm_volume != 63)
        {
            put_param(" %s=%d", "volume", g_fm_volume);
        }

        if (g_hi_attenuation != 0)
        {
            if (g_hi_attenuation == 1)
            {
                put_param(" %s=%s", "attenuate", "low");
            }
            else if (g_hi_attenuation == 2)
            {
                put_param(" %s=%s", "attenuate", "mid");
            }
            else if (g_hi_attenuation == 3)
            {
                put_param(" %s=%s", "attenuate", "high");
            }
            else   // just in case
            {
                put_param(" %s=%s", "attenuate", "none");
            }
        }

        if (g_polyphony != 0)
        {
            put_param(" %s=%d", "polyphony", g_polyphony+1);
        }

        if (g_fm_wave_type != 0)
        {
            put_param(" %s=%d", "wavetype", g_fm_wave_type);
        }

        if (g_fm_attack != 5)
        {
            put_param(" %s=%d", "attack", g_fm_attack);
        }

        if (g_fm_decay != 10)
        {
            put_param(" %s=%d", "decay", g_fm_decay);
        }

        if (g_fm_sustain != 13)
        {
            put_param(" %s=%d", "sustain", g_fm_sustain);
        }

        if (g_fm_release != 5)
        {
            put_param(" %s=%d", "srelease", g_fm_release);
        }

        if (g_sound_flag & SOUNDFLAG_QUANTIZED)
        {
            // quantize turned on
            int i;
            for (i = 0; i <= 11; i++)
            {
                if (g_scale_map[i] != i+1)
                {
                    i = 15;
                }
            }
            if (i > 12)
            {
                put_param(" %s=%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", "scalemap", g_scale_map[0], g_scale_map[1], g_scale_map[2], g_scale_map[3]
                         , g_scale_map[4], g_scale_map[5], g_scale_map[6], g_scale_map[7], g_scale_map[8]
                         , g_scale_map[9], g_scale_map[10], g_scale_map[11]);
            }
        }

        if (!g_bof_match_book_images)
        {
            put_param(" %s=%s", "nobof", "yes");
        }

        if (g_orbit_delay > 0)
        {
            put_param(" %s=%d", "orbitdelay", g_orbit_delay);
        }

        if (g_orbit_interval != 1)
        {
            put_param(" %s=%d", "orbitinterval", g_orbit_interval);
        }

        if (g_start_show_orbit)
        {
            put_param(" %s=%s", "showorbit", "yes");
        }

        if (g_keep_screen_coords)
        {
            put_param(" %s=%s", "screencoords", "yes");
        }

        if (g_user_std_calc_mode == 'o' && g_set_orbit_corners && g_keep_screen_coords)
        {
            put_param(" %s=", "orbitcorners");
            int x_digits = get_prec(g_orbit_corner_min_x, g_orbit_corner_max_x, g_orbit_corner_3rd_x);
            int y_digits = get_prec(g_orbit_corner_min_y, g_orbit_corner_max_y, g_orbit_corner_3rd_y);
            put_float(0, g_orbit_corner_min_x, x_digits);
            put_float(1, g_orbit_corner_max_x, x_digits);
            put_float(1, g_orbit_corner_min_y, y_digits);
            put_float(1, g_orbit_corner_max_y, y_digits);
            if (g_orbit_corner_3rd_x != g_orbit_corner_min_x || g_orbit_corner_3rd_y != g_orbit_corner_min_y)
            {
                put_float(1, g_orbit_corner_3rd_x, x_digits);
                put_float(1, g_orbit_corner_3rd_y, y_digits);
            }
        }

        if (g_draw_mode != 'r')
        {
            put_param(" %s=%c", "orbitdrawmode", g_draw_mode);
        }

        if (g_math_tol[0] != 0.05 || g_math_tol[1] != 0.05)
        {
            put_param(" %s=%g/%g", "mathtolerance", g_math_tol[0], g_math_tol[1]);
        }
    }

    if (*color_inf != 'n')
    {
        if (g_record_colors == RecordColorsMode::COMMENT && *color_inf == '@')
        {
            put_param_line();
            put_param("; %s=", "colors");
            put_param(color_inf);
            put_param_line();
        }
docolors:
        put_param(" %s=", "colors");
        if (g_record_colors != RecordColorsMode::COMMENT && g_record_colors != RecordColorsMode::YES && *color_inf == '@')
        {
            put_param(color_inf);
        }
        else
        {
            int diff_mag = -1;
            int diff1[4][3];
            int diff2[4][3];
            int force = 0;
            int cur_color = force;
            while (true)
            {
                // emit color in rgb 3 char encoded form
                for (int j = 0; j < 3; ++j)
                {
                    int k = g_dac_box[cur_color][j];
                    if (k < 10)
                    {
                        k += '0';
                    }
                    else if (k < 36)
                    {
                        k += ('A' - 10);
                    }
                    else
                    {
                        k += ('_' - 36);
                    }
                    buf[j] = (char)k;
                }
                buf[3] = 0;
                put_param(buf);
                if (++cur_color >= max_color)        // quit if done last color
                {
                    break;
                }
                if (g_debug_flag == DebugFlags::FORCE_LOSSLESS_COLORMAP)    // lossless compression
                {
                    continue;
                }
                /* Next a P Branderhorst special, a tricky scan for smooth-shaded
                   ranges which can be written as <nn> to compress .par file entry.
                   Method used is to check net change in each color value over
                   spans of 2 to 5 color numbers.  First time for each span size
                   the value change is noted.  After first time the change is
                   checked against noted change.  First time it differs, a
                   difference of 1 is tolerated and noted as an alternate
                   acceptable change.  When change is not one of the tolerated
                   values, loop exits. */
                if (force)
                {
                    --force;
                    continue;
                }
                int scan_color = cur_color;
                int k;
                while (scan_color < max_color)
                {
                    // scan while same diff to next
                    int i = scan_color - cur_color;
                    i = std::min(i, 3); // check spans up to 4 steps
                    for (k = 0; k <= i; ++k)
                    {
                        int j;
                        for (j = 0; j < 3; ++j)
                        {
                            // check pattern of chg per color
                            if (g_debug_flag != DebugFlags::ALLOW_LARGE_COLORMAP_CHANGES
                                && scan_color > (cur_color+4) && scan_color < max_color-5)
                            {
                                if (std::abs(2*g_dac_box[scan_color][j] - g_dac_box[scan_color-5][j]
                                        - g_dac_box[scan_color+5][j]) >= 2)
                                {
                                    break;
                                }
                            }
                            int delta = (int) g_dac_box[scan_color][j] - (int) g_dac_box[scan_color - k - 1][j];
                            if (k == scan_color - cur_color)
                            {
                                diff2[k][j] = delta;
                                diff1[k][j] = diff2[k][j];
                            }
                            else if (delta != diff1[k][j] && delta != diff2[k][j])
                            {
                                diff_mag = std::abs(delta - diff1[k][j]);
                                if (diff1[k][j] != diff2[k][j] || diff_mag != 1)
                                {
                                    break;
                                }
                                diff2[k][j] = delta;
                            }
                        }
                        if (j < 3)
                        {
                            break; // must've exited from inner loop above
                        }
                    }
                    if (k <= i)
                    {
                        break;   // must've exited from inner loop above
                    }
                    ++scan_color;
                }
                // now scanc-1 is next color which must be written explicitly
                if (scan_color - cur_color > 2)
                {
                    // good, we have a shaded range
                    if (scan_color != max_color)
                    {
                        if (diff_mag < 3)
                        {
                            // not a sharp slope change?
                            force = 2;       // force more between ranges, to stop
                            --scan_color;         // "drift" when load/store/load/store/
                        }
                        if (k)
                        {
                            // more of the same
                            force += k;
                            --scan_color;
                        }
                    }
                    if (--scan_color - cur_color > 1)
                    {
                        put_param("<%d>", scan_color-cur_color);
                        cur_color = scan_color;
                    }
                    else                  // changed our mind
                    {
                        force = 0;
                    }
                }
            }
        }
    }

    while (s_wb_data.len)   // flush the buffer
    {
        put_param_line();
    }

    restore_stack(saved);
}

static void put_file_name(const char *keyword, const char *fname)
{
    if (*fname && !ends_with_slash(fname))
    {
        const char *p = std::strrchr(fname, SLASH_CH);
        if (p != nullptr)
        {
            fname = p+1;
            if (*fname == 0)
            {
                return;
            }
        }
        put_param(" %s=%s", keyword, fname);
    }
}

static void put_param(const char *param, ...)
{
    std::va_list args;

    va_start(args, param);
    if (*param == ' '             // starting a new parm
        && s_wb_data.len == 0)         // skip leading space
    {
        ++param;
    }
    char *buf_ptr = s_wb_data.buf + s_wb_data.len;
    std::vsprintf(buf_ptr, param, args);
    while (*(buf_ptr++))
    {
        ++s_wb_data.len;
    }
    while (s_wb_data.len > 200)
    {
        put_param_line();
    }
}

static int nice_line_length()
{
    return g_max_line_length-4;
}

static void put_param_line()
{
    int len = s_wb_data.len;
    if (len > nice_line_length())
    {
        len = nice_line_length();
        while (len != 0 && s_wb_data.buf[len] != ' ')
        {
            --len;
        }
        if (len == 0)
        {
            len = nice_line_length()-1;
            while (++len < g_max_line_length
                && s_wb_data.buf[len]
                && s_wb_data.buf[len] != ' ')
            {
            }
        }
    }
    int c = s_wb_data.buf[len];
    s_wb_data.buf[len] = 0;
    std::fputs("  ", s_param_file);
    std::fputs(s_wb_data.buf, s_param_file);
    if (c && c != ' ')
    {
        std::fputc('\\', s_param_file);
    }
    std::fputc('\n', s_param_file);
    s_wb_data.buf[len] = (char)c;
    if (c == ' ')
    {
        ++len;
    }
    s_wb_data.len -= len;
    std::strcpy(s_wb_data.buf, s_wb_data.buf+len);
}

/*
   Strips zeros from the non-exponent part of a number. This logic
   was originally in put_bf(), but is split into this routine so it can be
   shared with put_float().
*/

static void strip_zeros(char *buf)
{
    string_lower(buf);
    char *dot_ptr = std::strchr(buf, '.');
    if (dot_ptr != nullptr)
    {
        char *b_ptr;
        ++dot_ptr;
        char *exp_ptr = std::strchr(buf, 'e');
        if (exp_ptr != nullptr)    // scientific notation with 'e'?
        {
            b_ptr = exp_ptr;
        }
        else
        {
            b_ptr = buf + std::strlen(buf);
        }
        while (--b_ptr > dot_ptr && *b_ptr == '0')
        {
            *b_ptr = 0;
        }
        if (exp_ptr && b_ptr < exp_ptr -1)
        {
            std::strcat(buf, exp_ptr);
        }
    }
}

static void put_float(int slash, double value, int prec)
{
    char buf[40];
    char *buff_ptr = buf;
    if (slash)
    {
        *(buff_ptr++) = '/';
    }
    /* Idea of long double cast is to squeeze out another digit or two
       which might be needed (we have found cases where this digit makes
       a difference.) But let's not do this at lower precision */

    if (prec > 15)
    {
        std::sprintf(buff_ptr, "%1.*Lg", prec, (long double)value);
    }
    else
    {
        std::sprintf(buff_ptr, "%1.*g", prec, value);
    }
    strip_zeros(buff_ptr);
    put_param(buf);
}

static void put_bf(int slash, BigFloat r, int prec)
{
    std::vector<char> buf;              // "/-1.xxxxxxE-1234"
    buf.resize(5000);
    char *buff_ptr = buf.data();
    if (slash)
    {
        *(buff_ptr++) = '/';
    }
    bf_to_str(buff_ptr, prec, r);
    strip_zeros(buff_ptr);
    put_param(buf.data());
}
