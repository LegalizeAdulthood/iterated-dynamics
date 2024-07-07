#include "make_batch_file.h"

#include "port.h"
#include "prototyp.h" // for stricmp

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "comments.h"
#include "convert_center_mag.h"
#include "debug_flags.h"
#include "dir_file.h"
#include "drivers.h"
#include "ends_with_slash.h"
#include "file_gets.h"
#include "find_path.h"
#include "fractalp.h"
#include "fractype.h"
#include "full_screen_prompt.h"
#include "get_calculation_time.h"
#include "get_prec_big_float.h"
#include "has_ext.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"
#include "jb.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "os.h"
#include "parser.h"
#include "plot3d.h"
#include "rotate.h"
#include "sign.h"
#include "sound.h"
#include "spindac.h"
#include "stereo.h"
#include "sticky_orbits.h"
#include "stop_msg.h"
#include "trig_fns.h"
#include "type_has_param.h"
#include "value_saver.h"
#include "version.h"
#include "video_mode.h"

#include <algorithm>
#include <array>
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

static void put_parm(char const *parm, ...);
static void put_parm_line();
static void put_float(int, double, int);
static void put_bf(int slash, bf_t r, int prec);
static void put_filename(char const *keyword, char const *fname);
static void strip_zeros(char *buf);
static void write_batch_parms(char const *colorinf, bool colorsonly, int maxcolor, int ii, int jj);

bool g_make_parameter_file = false;
bool g_make_parameter_file_map = false;
int g_max_line_length = 72;

static std::FILE *parmfile;

inline char par_key(int x)
{
    return static_cast<char>(x < 10 ? '0' + x : 'a' - 10 + x);
}

inline bool is_writable(const std::string &path)
{
    const fs::perms read_write = fs::perms::owner_read | fs::perms::owner_write;
    return (fs::status(path).permissions() & read_write) == read_write;
}

void make_batch_file()
{
    constexpr int MAXPROMPTS = 18;
    bool colorsonly = false;
    // added for pieces feature
    double pdelx = 0.0;
    double pdely = 0.0;
    double pdelx2 = 0.0;
    double pdely2 = 0.0;
    unsigned int pxdots;
    unsigned int pydots;
    unsigned int xm;
    unsigned int ym;
    double pxxmin = 0.0;
    double pyymax = 0.0;
    char vidmde[5];
    int promptnum;
    int piecespromts;
    bool have3rd = false;
    char inpcommandfile[80];
    char inpcommandname[ITEM_NAME_LEN + 1];
    char inpcomment[4][MAX_COMMENT_LEN];
    fullscreenvalues paramvalues[18];
    char const      *choices[MAXPROMPTS];
    fs::path out_name;
    char             buf[256];
    char             buf2[128];
    std::FILE *infile = nullptr;
    std::FILE *fpbat = nullptr;
    char colorspec[14];
    int maxcolor;
    int maxcolorindex = 0;
    char const *sptr = nullptr;
    char const *sptr2;

    if (g_make_parameter_file_map)   // makepar map case
    {
        colorsonly = true;
    }

    driver_stack_screen();
    ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_PARMFILE};

    maxcolor = g_colors;
    std::strcpy(colorspec, "y");
#ifndef XFRACT
    if (g_got_real_dac || (g_is_true_color && g_true_mode == true_color_mode::default_color))
#else
    if (g_got_real_dac || (g_is_true_color && g_true_mode == true_color_mode::default_color) || g_fake_lut)
#endif
    {
        --maxcolor;
        if (g_inside_color > COLOR_BLACK && g_inside_color > maxcolor)
        {
            maxcolor = g_inside_color;
        }
        if (g_outside_color > COLOR_BLACK && g_outside_color > maxcolor)
        {
            maxcolor = g_outside_color;
        }
        if (g_distance_estimator < COLOR_BLACK && -g_distance_estimator > maxcolor)
        {
            maxcolor = (int)(0 - g_distance_estimator);
        }
        if (g_decomp[0] > maxcolor)
        {
            maxcolor = g_decomp[0] - 1;
        }
        if (g_potential_flag && g_potential_params[0] >= maxcolor)
        {
            maxcolor = (int)g_potential_params[0];
        }
        if (++maxcolor > 256)
        {
            maxcolor = 256;
        }

        if (g_color_state == color_state::DEFAULT)
        {
            // default colors
            if (g_map_specified)
            {
                colorspec[0] = '@';
                sptr = g_map_name.c_str();
            }
        }
        else if (g_color_state == color_state::MAP_FILE)
        {
            // colors match colorfile
            colorspec[0] = '@';
            sptr = g_color_file.c_str();
        }
        else                        // colors match no .map that we know of
        {
            std::strcpy(colorspec, "y");
        }

        if (sptr && colorspec[0] == '@')
        {
            sptr2 = std::strrchr(sptr, SLASHC);
            if (sptr2 != nullptr)
            {
                sptr = sptr2 + 1;
            }
            sptr2 = std::strrchr(sptr, ':');
            if (sptr2 != nullptr)
            {
                sptr = sptr2 + 1;
            }
            std::strncpy(&colorspec[1], sptr, 12);
            colorspec[13] = 0;
        }
    }
    std::strcpy(inpcommandfile, g_command_file.c_str());
    std::strcpy(inpcommandname, g_command_name.c_str());
    for (int i = 0; i < 4; i++)
    {
        std::strcpy(inpcomment[i], expand_command_comment(i).c_str());
    }

    if (g_command_name.empty())
    {
        std::strcpy(inpcommandname, "test");
    }
    pxdots = g_logical_screen_x_dots;
    pydots = g_logical_screen_y_dots;
    ym = 1;
    xm = ym;
    if (g_make_parameter_file)
    {
        goto skip_UI;
    }

    vidmode_keyname(g_video_entry.keynum, vidmde);
    while (true)
    {
prompt_user:
        promptnum = 0;
        choices[promptnum] = "Parameter file";
        paramvalues[promptnum].type = 0x100 + MAX_COMMENT_LEN - 1;
        paramvalues[promptnum++].uval.sbuf = inpcommandfile;
        choices[promptnum] = "Name";
        paramvalues[promptnum].type = 0x100 + ITEM_NAME_LEN;
        paramvalues[promptnum++].uval.sbuf = inpcommandname;
        choices[promptnum] = "Main comment";
        paramvalues[promptnum].type = 0x100 + MAX_COMMENT_LEN - 1;
        paramvalues[promptnum++].uval.sbuf = inpcomment[0];
        choices[promptnum] = "Second comment";
        paramvalues[promptnum].type = 0x100 + MAX_COMMENT_LEN - 1;
        paramvalues[promptnum++].uval.sbuf = inpcomment[1];
        choices[promptnum] = "Third comment";
        paramvalues[promptnum].type = 0x100 + MAX_COMMENT_LEN - 1;
        paramvalues[promptnum++].uval.sbuf = inpcomment[2];
        choices[promptnum] = "Fourth comment";
        paramvalues[promptnum].type = 0x100 + MAX_COMMENT_LEN - 1;
        paramvalues[promptnum++].uval.sbuf = inpcomment[3];
#ifndef XFRACT
        if (g_got_real_dac || (g_is_true_color && g_true_mode == true_color_mode::default_color))
#else
        if (g_got_real_dac || (g_is_true_color && g_true_mode == true_color_mode::default_color) || g_fake_lut)
#endif
        {
            choices[promptnum] = "Record colors?";
            paramvalues[promptnum].type = 0x100 + 13;
            paramvalues[promptnum++].uval.sbuf = colorspec;
            choices[promptnum] = "    (no | yes | only for full info | @filename to point to a map file)";
            paramvalues[promptnum++].type = '*';
            choices[promptnum] = "# of colors";
            maxcolorindex = promptnum;
            paramvalues[promptnum].type = 'i';
            paramvalues[promptnum++].uval.ival = maxcolor;
            choices[promptnum] = "    (if recording full color info)";
            paramvalues[promptnum++].type = '*';
        }
        choices[promptnum] = "Maximum line length";
        paramvalues[promptnum].type = 'i';
        paramvalues[promptnum++].uval.ival = g_max_line_length;
        choices[promptnum] = "";
        paramvalues[promptnum++].type = '*';
        choices[promptnum] = "    **** The following is for generating images in pieces ****";
        paramvalues[promptnum++].type = '*';
        choices[promptnum] = "X Multiples";
        piecespromts = promptnum;
        paramvalues[promptnum].type = 'i';
        paramvalues[promptnum++].uval.ival = xm;
        choices[promptnum] = "Y Multiples";
        paramvalues[promptnum].type = 'i';
        paramvalues[promptnum++].uval.ival = ym;
        choices[promptnum] = "Video mode";
        paramvalues[promptnum].type = 0x100 + 4;
        paramvalues[promptnum++].uval.sbuf = vidmde;

        if (fullscreen_prompt("Save Current Parameters", promptnum, choices, paramvalues, 0, nullptr) < 0)
        {
            break;
        }

        if (*colorspec == 'o' || g_make_parameter_file_map)
        {
            std::strcpy(colorspec, "y");
            colorsonly = true;
        }

        g_command_file = inpcommandfile;
        if (has_ext(g_command_file.c_str()) == nullptr)
        {
            g_command_file += ".par";   // default extension .par

        }
        g_command_name = inpcommandname;
        for (int i = 0; i < 4; i++)
        {
            g_command_comment[i] = inpcomment[i];
        }
#ifndef XFRACT
        if (g_got_real_dac || (g_is_true_color && g_true_mode == true_color_mode::default_color))
#else
        if (g_got_real_dac || (g_is_true_color && g_true_mode == true_color_mode::default_color) || g_fake_lut)
#endif
        {
            if (paramvalues[maxcolorindex].uval.ival > 0 &&
                    paramvalues[maxcolorindex].uval.ival <= 256)
            {
                maxcolor = paramvalues[maxcolorindex].uval.ival;
            }
        }

        promptnum = piecespromts;
        {
            int newmaxlinelength;
            newmaxlinelength = paramvalues[promptnum-3].uval.ival;
            if (g_max_line_length != newmaxlinelength
                && newmaxlinelength >= MIN_MAX_LINE_LENGTH
                && newmaxlinelength <= MAX_MAX_LINE_LENGTH)
            {
                g_max_line_length = newmaxlinelength;
            }
        }
        xm = paramvalues[promptnum++].uval.ival;
        ym = paramvalues[promptnum++].uval.ival;

        // sanity checks
        {
            long xtotal;
            long ytotal;
            int i;

            // get resolution from the video name (which must be valid)
            pydots = 0;
            pxdots = pydots;
            i = check_vidmode_keyname(vidmde);
            if (i > 0)
            {
                i = check_vidmode_key(0, i);
                if (i >= 0)
                {
                    // get the resolution of this video mode
                    pxdots = g_video_table[i].xdots;
                    pydots = g_video_table[i].ydots;
                }
            }
            if (pxdots == 0 && (xm > 1 || ym > 1))
            {
                // no corresponding video mode!
                stopmsg("Invalid video mode entry!");
                goto prompt_user;
            }

            // bounds range on xm, ym
            if (xm < 1 || xm > 36 || ym < 1 || ym > 36)
            {
                stopmsg("X and Y components must be 1 to 36");
                goto prompt_user;
            }

            // another sanity check: total resolution cannot exceed 65535
            xtotal = xm;
            ytotal = ym;
            xtotal *= pxdots;
            ytotal *= pydots;
            if (xtotal > 65535L || ytotal > 65535L)
            {
                stopmsg("Total resolution (X or Y) cannot exceed 65535");
                goto prompt_user;
            }
        }
skip_UI:
        if (g_make_parameter_file)
        {
            if (g_file_colors > 0)
            {
                std::strcpy(colorspec, "y");
            }
            else
            {
                std::strcpy(colorspec, "n");
            }
            if (g_make_parameter_file_map)
            {
                maxcolor = 256;
            }
            else
            {
                maxcolor = g_file_colors;
            }
        }
        out_name = g_command_file;
        bool gotinfile = false;
        if (fs::exists(g_command_file))
        {
            // file exists
            gotinfile = true;
            if (!is_writable(g_command_file))
            {
                std::snprintf(buf, std::size(buf), "Can't write %s", g_command_file.c_str());
                stopmsg(buf);
                continue;
            }
            out_name.replace_filename("id.tmp");
            infile = std::fopen(g_command_file.c_str(), "rt");
        }
        parmfile = std::fopen(out_name.string().c_str(), "wt");
        if (parmfile == nullptr)
        {
            stopmsg("Can't create " + out_name.string());
            if (gotinfile)
            {
                std::fclose(infile);
            }
            continue;
        }

        if (gotinfile)
        {
            while (file_gets(buf, 255, infile) >= 0)
            {
                if (std::strchr(buf, '{')// entry heading?
                    && std::sscanf(buf, " %40[^ \t({]", buf2)
                    && stricmp(buf2, g_command_name.c_str()) == 0)
                {
                    // entry with same name
                    std::snprintf(buf2, std::size(buf2), "File already has an entry named %s\n%s",
                    g_command_name.c_str(), g_make_parameter_file ?
                    "... Replacing ..." : "Continue to replace it, Cancel to back out");
                    if (stopmsg(stopmsg_flags::CANCEL | stopmsg_flags::INFO_ONLY, buf2))
                    {
                        // cancel
                        std::fclose(infile);
                        std::fclose(parmfile);
                        remove(out_name);
                        goto prompt_user;
                    }
                    while (std::strchr(buf, '}') == nullptr
                        && file_gets(buf, 255, infile) > 0)
                    {
                        ; // skip to end of set
                    }
                    break;
                }
                std::fputs(buf, parmfile);
                std::fputc('\n', parmfile);
            }
        }
        //**** start here
        if (xm > 1 || ym > 1)
        {
            have3rd = g_x_min != g_x_3rd || g_y_min != g_y_3rd;
            fpbat = dir_fopen(g_working_dir.c_str(), "makemig.bat", "w");
            if (fpbat == nullptr)
            {
                ym = 0;
                xm = ym;
            }
            pdelx  = (g_x_max - g_x_3rd) / (xm * pxdots - 1);   // calculate stepsizes
            pdely  = (g_y_max - g_y_3rd) / (ym * pydots - 1);
            pdelx2 = (g_x_3rd - g_x_min) / (ym * pydots - 1);
            pdely2 = (g_y_3rd - g_y_min) / (xm * pxdots - 1);

            // save corners
            pxxmin = g_x_min;
            pyymax = g_y_max;
        }
        for (int i = 0; i < (int)xm; i++)    // columns
        {
            for (int j = 0; j < (int)ym; j++)  // rows
            {
                if (xm > 1 || ym > 1)
                {
                    int w;
                    char c;
                    char PCommandName[80];
                    w = 0;
                    while (w < (int)g_command_name.length())
                    {
                        c = g_command_name[w];
                        if (std::isspace(c) || c == 0)
                        {
                            break;
                        }
                        PCommandName[w] = c;
                        w++;
                    }
                    PCommandName[w] = 0;
                    {
                        char tmpbuff[20];
                        std::snprintf(tmpbuff, std::size(tmpbuff), "_%c%c", par_key(i), par_key(j));
                        std::strcat(PCommandName, tmpbuff);
                    }
                    std::fprintf(parmfile, "%-19s{", PCommandName);
                    g_x_min = pxxmin + pdelx*(i*pxdots) + pdelx2*(j*pydots);
                    g_x_max = pxxmin + pdelx*((i+1)*pxdots - 1) + pdelx2*((j+1)*pydots - 1);
                    g_y_min = pyymax - pdely*((j+1)*pydots - 1) - pdely2*((i+1)*pxdots - 1);
                    g_y_max = pyymax - pdely*(j*pydots) - pdely2*(i*pxdots);
                    if (have3rd)
                    {
                        g_x_3rd = pxxmin + pdelx*(i*pxdots) + pdelx2*((j+1)*pydots - 1);
                        g_y_3rd = pyymax - pdely*((j+1)*pydots - 1) - pdely2*(i*pxdots);
                    }
                    else
                    {
                        g_x_3rd = g_x_min;
                        g_y_3rd = g_y_min;
                    }
                    std::fprintf(fpbat, "id batch=yes overwrite=yes @%s/%s\n", g_command_file.c_str(), PCommandName);
                    std::fprintf(fpbat, "if errorlevel 2 goto oops\n");
                }
                else
                {
                    std::fprintf(parmfile, "%-19s{", g_command_name.c_str());
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
                    std::fprintf(parmfile, " ; %s", g_command_comment[0].c_str());
                }
                std::fputc('\n', parmfile);
                {
                    char tmpbuff[25];
                    std::memset(tmpbuff, ' ', 23);
                    tmpbuff[23] = 0;
                    tmpbuff[21] = ';';
                    for (int k = 1; k < 4; k++)
                    {
                        if (!g_command_comment[k].empty())
                        {
                            std::fprintf(parmfile, "%s%s\n", tmpbuff, g_command_comment[k].c_str());
                        }
                    }
                    if (g_patch_level != 0 && !colorsonly)
                    {
                        std::fprintf(parmfile, "%s id Version %d Patchlevel %d\n", tmpbuff, g_release, g_patch_level);
                    }
                }
                write_batch_parms(colorspec, colorsonly, maxcolor, i, j);
                if (xm > 1 || ym > 1)
                {
                    std::fprintf(parmfile, "  video=%s", vidmde);
                    std::fprintf(parmfile, " savename=frmig_%c%c\n", par_key(i), par_key(j));
                }
                std::fprintf(parmfile, "}\n\n");
            }
        }
        if (xm > 1 || ym > 1)
        {
            std::fprintf(fpbat, "start/wait id makemig=%u/%u\n", xm, ym);
            std::fprintf(fpbat, "rem Simplgif fractmig.gif simplgif.gif  in case you need it\n");
            std::fprintf(fpbat, ":oops\n");
            std::fclose(fpbat);
        }
        //******end here

        if (gotinfile)
        {
            // copy the rest of the file
            int i;
            do
            {
                i = file_gets(buf, 255, infile);
            }
            while (i == 0); // skip blanks
            while (i >= 0)
            {
                std::fputs(buf, parmfile);
                std::fputc('\n', parmfile);
                i = file_gets(buf, 255, infile);
            }
            std::fclose(infile);
        }
        std::fclose(parmfile);
        if (gotinfile)
        {
            // replace the original file with the new
            std::remove(g_command_file.c_str()); // success assumed on these lines
            rename(out_name, g_command_file);     // since we checked earlier with access
        }
        break;
    }
    driver_unstack_screen();
}

struct write_batch_data // buffer for parms to break lines nicely
{
    int len;
    char buf[10000];
};
static write_batch_data s_wbdata;

static int getprec(double a, double b, double c)
{
    double diff;
    double temp;
    int digits;
    double highv = 1.0E20;
    diff = std::fabs(a - b);
    if (diff == 0.0)
    {
        diff = highv;
    }
    temp = std::fabs(a - c);
    if (temp == 0.0)
    {
        temp = highv;
    }
    if (temp < diff)
    {
        diff = temp;
    }
    temp = std::fabs(b - c);
    if (temp == 0.0)
    {
        temp = highv;
    }
    if (temp < diff)
    {
        diff = temp;
    }
    digits = 7;
    if (g_debug_flag >= debug_flags::force_precision_0_digits &&
        g_debug_flag < debug_flags::force_precision_20_digits)
    {
        digits = +g_debug_flag - +debug_flags::force_precision_0_digits;
    }
    while (diff < 1.0 && digits <= DBL_DIG+1)
    {
        diff *= 10;
        ++digits;
    }
    return digits;
}

static void write_batch_parms(char const *colorinf, bool colorsonly, int maxcolor, int ii, int jj)
{
    double Xctr;
    double Yctr;
    LDBL Magnification;
    double Xmagfactor;
    double Rotation;
    double Skew;
    char const *sptr;
    char buf[81];
    bf_t bfXctr = nullptr;
    bf_t bfYctr = nullptr;
    int saved;
    saved = save_stack();
    if (g_bf_math != bf_math_type::NONE)
    {
        bfXctr = alloc_stack(g_bf_length+2);
        bfYctr = alloc_stack(g_bf_length+2);
    }

    s_wbdata.len = 0; // force first parm to start on new line

    // Using near string g_box_x for buffer after saving to extraseg

    if (colorsonly)
    {
        goto docolors;
    }
    if (g_display_3d <= display_3d_modes::NONE)
    {
        // a fractal was generated

        //***** fractal only parameters in this section ******
        put_parm(" reset");
        put_parm("=%d", g_release);

        sptr = g_cur_fractal_specific->name;
        if (*sptr == '*')
        {
            ++sptr;
        }
        put_parm(" %s=%s", "type", sptr);

        if (g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP)
        {
            put_parm(" %s=%.15g/%.15g/%.15g/%.15g",
                     "julibrotfromto", g_julibrot_x_max, g_julibrot_x_min, g_julibrot_y_max, g_julibrot_y_min);
            // these rarely change
            if (g_julibrot_origin_fp != 8
                || g_julibrot_height_fp != 7
                || g_julibrot_width_fp != 10
                || g_julibrot_dist_fp != 24
                || g_julibrot_depth_fp != 8
                || g_julibrot_z_dots != 128)
            {
                put_parm(" %s=%d/%g/%g/%g/%g/%g", "julibrot3d",
                         g_julibrot_z_dots, g_julibrot_origin_fp, g_julibrot_depth_fp, g_julibrot_height_fp, g_julibrot_width_fp, g_julibrot_dist_fp);
            }
            if (g_eyes_fp != 0)
            {
                put_parm(" %s=%g", "julibroteyes", g_eyes_fp);
            }
            if (g_new_orbit_type != fractal_type::JULIA)
            {
                char const *name;
                name = g_fractal_specific[+g_new_orbit_type].name;
                if (*name == '*')
                {
                    name++;
                }
                put_parm(" %s=%s", "orbitname", name);
            }
            if (g_julibrot_3d_mode != julibrot_3d_mode::MONOCULAR)
            {
                put_parm(" %s=%s", "3dmode", to_string(g_julibrot_3d_mode));
            }
        }
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        {
            put_filename("formulafile", g_formula_filename.c_str());
            put_parm(" %s=%s", "formulaname", g_formula_name.c_str());
            if (g_frm_uses_ismand)
            {
                put_parm(" %s=%c", "ismand", g_is_mandelbrot ? 'y' : 'n');
            }
        }
        if (g_fractal_type == fractal_type::LSYSTEM)
        {
            put_filename("lfile", g_l_system_filename.c_str());
            put_parm(" %s=%s", "lname", g_l_system_name.c_str());
        }
        if (g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D)
        {
            put_filename("ifsfile", g_ifs_filename.c_str());
            put_parm(" %s=%s", "ifs", g_ifs_name.c_str());
        }
        if (g_fractal_type == fractal_type::INVERSEJULIA || g_fractal_type == fractal_type::INVERSEJULIAFP)
        {
            put_parm(" %s=%s/%s", "miim", to_string(g_major_method), to_string(g_inverse_julia_minor_method));
        }

        strncpy(buf, showtrig().c_str(), std::size(buf));
        if (buf[0])
        {
            put_parm(buf);
        }

        if (g_user_std_calc_mode != 'g')
        {
            put_parm(" %s=%c", "passes", g_user_std_calc_mode);
        }


        if (g_stop_pass != 0)
        {
            put_parm(" %s=%c%c", "passes", g_user_std_calc_mode, (char)g_stop_pass + '0');
        }

        if (g_use_center_mag)
        {
            if (g_bf_math != bf_math_type::NONE)
            {
                int digits;
                cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
                digits = getprecbf(MAXREZ);
                put_parm(" %s=", "center-mag");
                put_bf(0, bfXctr, digits);
                put_bf(1, bfYctr, digits);
            }
            else // !g_bf_math
            {
                cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
                put_parm(" %s=", "center-mag");
                //          convert 1000 fudged long to double, 1000/1<<24 = 6e-5
                put_parm(g_delta_min > 6e-5 ? "%g/%g" : "%+20.17lf/%+20.17lf", Xctr, Yctr);
            }
            put_parm("/%.7Lg", Magnification); // precision of magnification not critical, but magnitude is
            // Round to avoid ugly decimals, precision here is not critical
            // Don't round Xmagfactor if it's small
            if (std::fabs(Xmagfactor) > 0.5)   // or so, exact value isn't important
            {
                Xmagfactor = (sign(Xmagfactor) * (long)(std::fabs(Xmagfactor) * 1e4 + 0.5)) / 1e4;
            }
            // Just truncate these angles.  Who cares about 1/1000 of a degree
            // Somebody does.  Some rotated and/or skewed images are slightly
            // off when recreated from a PAR using 1/1000.
            if (Xmagfactor != 1 || Rotation != 0 || Skew != 0)
            {
                // Only put what is necessary
                // The difference with Xmagfactor is that it is normally
                // near 1 while the others are normally near 0
                if (std::fabs(Xmagfactor) >= 1)
                {
                    put_float(1, Xmagfactor, 5); // put_float() uses %g
                }
                else     // abs(Xmagfactor) is < 1
                {
                    put_float(1, Xmagfactor, 4); // put_float() uses %g
                }
                if (Rotation != 0 || Skew != 0)
                {
                    // Use precision=6 here.  These angle have already been rounded
                    // to 3 decimal places, but angles like 123.456 degrees need 6
                    // sig figs to get 3 decimal places.  Trailing 0's are dropped anyway.
                    put_float(1, Rotation, 18);
                    if (Skew != 0)
                    {
                        put_float(1, Skew, 18);
                    }
                }
            }
        }
        else // not usemag
        {
            put_parm(" %s=", "corners");
            if (g_bf_math != bf_math_type::NONE)
            {
                int digits;
                digits = getprecbf(MAXREZ);
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
                int xdigits;
                int ydigits;
                xdigits = getprec(g_x_min, g_x_max, g_x_3rd);
                ydigits = getprec(g_y_min, g_y_max, g_y_3rd);
                put_float(0, g_x_min, xdigits);
                put_float(1, g_x_max, xdigits);
                put_float(1, g_y_min, ydigits);
                put_float(1, g_y_max, ydigits);
                if (g_x_3rd != g_x_min || g_y_3rd != g_y_min)
                {
                    put_float(1, g_x_3rd, xdigits);
                    put_float(1, g_y_3rd, ydigits);
                }
            }
        }

        int i;
        for (i = (MAX_PARAMS-1); i >= 0; --i)
        {
            if (typehasparm((g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP)
                            ?g_new_orbit_type:g_fractal_type, i, nullptr))
            {
                break;
            }
        }

        if (i >= 0)
        {
            if (g_fractal_type == fractal_type::CELLULAR || g_fractal_type == fractal_type::ANT)
            {
                put_parm(" %s=%.1f", "params", g_params[0]);
            }
            else
            {
                if (g_debug_flag == debug_flags::force_long_double_param_output)
                {
                    put_parm(" %s=%.17Lg", "params", (long double)g_params[0]);
                }
                else
                {
                    put_parm(" %s=%.17g", "params", g_params[0]);
                }
            }
            for (int j = 1; j <= i; ++j)
            {
                if (g_fractal_type == fractal_type::CELLULAR || g_fractal_type == fractal_type::ANT)
                {
                    put_parm("/%.1f", g_params[j]);
                }
                else
                {
                    if (g_debug_flag == debug_flags::force_long_double_param_output)
                    {
                        put_parm("/%.17Lg", (long double)g_params[j]);
                    }
                    else
                    {
                        put_parm("/%.17g", g_params[j]);
                    }
                }
            }
        }

        if (g_use_init_orbit == init_orbit_mode::pixel)
        {
            put_parm(" %s=pixel", "initorbit");
        }
        else if (g_use_init_orbit == init_orbit_mode::value)
        {
            put_parm(" %s=%.15g/%.15g", "initorbit", g_init_orbit.x, g_init_orbit.y);
        }

        if (g_float_flag)
        {
            put_parm(" %s=y", "float");
        }

        if (g_max_iterations != 150)
        {
            put_parm(" %s=%ld", "maxiter", g_max_iterations);
        }

        if (g_bail_out && (!g_potential_flag || g_potential_params[2] == 0.0))
        {
            put_parm(" %s=%ld", "bailout", g_bail_out);
        }

        if (g_bail_out_test != bailouts::Mod)
        {
            put_parm(" %s=", "bailoutest");
            if (g_bail_out_test == bailouts::Real)
            {
                put_parm("real");
            }
            else if (g_bail_out_test == bailouts::Imag)
            {
                put_parm("imag");
            }
            else if (g_bail_out_test == bailouts::Or)
            {
                put_parm("or");
            }
            else if (g_bail_out_test == bailouts::And)
            {
                put_parm("and");
            }
            else if (g_bail_out_test == bailouts::Manh)
            {
                put_parm("manh");
            }
            else if (g_bail_out_test == bailouts::Manr)
            {
                put_parm("manr");
            }
            else
            {
                put_parm("mod"); // default, just in case
            }
        }
        if (g_fill_color != -1)
        {
            put_parm(" %s=", "fillcolor");
            put_parm("%d", g_fill_color);
        }
        if (g_inside_color != 1)
        {
            put_parm(" %s=", "inside");
            if (g_inside_color == ITER)
            {
                put_parm("maxiter");
            }
            else if (g_inside_color == ZMAG)
            {
                put_parm("zmag");
            }
            else if (g_inside_color == BOF60)
            {
                put_parm("bof60");
            }
            else if (g_inside_color == BOF61)
            {
                put_parm("bof61");
            }
            else if (g_inside_color == EPSCROSS)
            {
                put_parm("epsiloncross");
            }
            else if (g_inside_color == STARTRAIL)
            {
                put_parm("startrail");
            }
            else if (g_inside_color == PERIOD)
            {
                put_parm("period");
            }
            else if (g_inside_color == FMODI)
            {
                put_parm("fmod");
            }
            else if (g_inside_color == ATANI)
            {
                put_parm("atan");
            }
            else
            {
                put_parm("%d", g_inside_color);
            }
        }
        if (g_close_proximity != 0.01
            && (g_inside_color == EPSCROSS || g_inside_color == FMODI || g_outside_color == FMOD))
        {
            put_parm(" %s=%.15g", "proximity", g_close_proximity);
        }
        if (g_outside_color != ITER)
        {
            put_parm(" %s=", "outside");
            if (g_outside_color == REAL)
            {
                put_parm("real");
            }
            else if (g_outside_color == IMAG)
            {
                put_parm("imag");
            }
            else if (g_outside_color == MULT)
            {
                put_parm("mult");
            }
            else if (g_outside_color == SUM)
            {
                put_parm("summ");
            }
            else if (g_outside_color == ATAN)
            {
                put_parm("atan");
            }
            else if (g_outside_color == FMOD)
            {
                put_parm("fmod");
            }
            else if (g_outside_color == TDIS)
            {
                put_parm("tdis");
            }
            else
            {
                put_parm("%d", g_outside_color);
            }
        }

        if (g_log_map_flag && !g_iteration_ranges_len)
        {
            put_parm(" %s=", "logmap");
            if (g_log_map_flag == -1)
            {
                put_parm("old");
            }
            else if (g_log_map_flag == 1)
            {
                put_parm("yes");
            }
            else
            {
                put_parm("%ld", g_log_map_flag);
            }
        }

        if (g_log_map_fly_calculate && g_log_map_flag && !g_iteration_ranges_len)
        {
            put_parm(" %s=", "logmode");
            if (g_log_map_fly_calculate == 1)
            {
                put_parm("fly");
            }
            else if (g_log_map_fly_calculate == 2)
            {
                put_parm("table");
            }
        }

        if (g_potential_flag)
        {
            put_parm(" %s=%d/%g/%d", "potential",
                     (int)g_potential_params[0], g_potential_params[1], (int)g_potential_params[2]);
            if (g_potential_16bit)
            {
                put_parm("/%s", "16bit");
            }
        }
        if (g_invert != 0)
        {
            put_parm(" %s=%-1.15lg/%-1.15lg/%-1.15lg", "invert",
                     g_inversion[0], g_inversion[1], g_inversion[2]);
        }
        if (g_decomp[0])
        {
            put_parm(" %s=%d", "decomp", g_decomp[0]);
        }
        if (g_distance_estimator)
        {
            put_parm(" %s=%ld/%d/%d/%d", "distest", g_distance_estimator, g_distance_estimator_width_factor,
                     g_distance_estimator_x_dots?g_distance_estimator_x_dots:g_logical_screen_x_dots, g_distance_estimator_y_dots?g_distance_estimator_y_dots:g_logical_screen_y_dots);
        }
        if (g_old_demm_colors)
        {
            put_parm(" %s=y", "olddemmcolors");
        }
        if (g_user_biomorph_value != -1)
        {
            put_parm(" %s=%d", "biomorph", g_user_biomorph_value);
        }
        if (g_finite_attractor)
        {
            put_parm(" %s=y", "finattract");
        }

        if (g_force_symmetry != symmetry_type::NOT_FORCED)
        {
            if (g_force_symmetry == static_cast<symmetry_type>(1000) && ii == 1 && jj == 1)
            {
                stopmsg("Regenerate before <b> to get correct symmetry");
            }
            put_parm(" %s=", "symmetry");
            if (g_force_symmetry == symmetry_type::X_AXIS)
            {
                put_parm("xaxis");
            }
            else if (g_force_symmetry == symmetry_type::Y_AXIS)
            {
                put_parm("yaxis");
            }
            else if (g_force_symmetry == symmetry_type::XY_AXIS)
            {
                put_parm("xyaxis");
            }
            else if (g_force_symmetry == symmetry_type::ORIGIN)
            {
                put_parm("origin");
            }
            else if (g_force_symmetry == symmetry_type::PI_SYM)
            {
                put_parm("pi");
            }
            else
            {
                put_parm("none");
            }
        }

        if (g_periodicity_check != 1)
        {
            put_parm(" %s=%d", "periodicity", g_periodicity_check);
        }

        if (g_random_seed_flag)
        {
            put_parm(" %s=%d", "rseed", g_random_seed);
        }

        if (g_iteration_ranges_len)
        {
            put_parm(" %s=", "ranges");
            i = 0;
            while (i < g_iteration_ranges_len)
            {
                if (i)
                {
                    put_parm("/");
                }
                if (g_iteration_ranges[i] == -1)
                {
                    put_parm("-%d/", g_iteration_ranges[++i]);
                    ++i;
                }
                put_parm("%d", g_iteration_ranges[i++]);
            }
        }
    }

    if (g_display_3d >= display_3d_modes::YES)
    {
        //**** 3d transform only parameters in this section ****
        if (g_display_3d == display_3d_modes::B_COMMAND)
        {
            put_parm(" %s=%s", "3d", "overlay");
        }
        else
        {
            put_parm(" %s=%s", "3d", "yes");
        }
        if (!g_loaded_3d)
        {
            put_filename("filename", g_read_filename.c_str());
        }
        if (SPHERE)
        {
            put_parm(" %s=y", "sphere");
            put_parm(" %s=%d/%d", "latitude", THETA1, THETA2);
            put_parm(" %s=%d/%d", "longitude", PHI1, PHI2);
            put_parm(" %s=%d", "radius", RADIUS);
        }
        put_parm(" %s=%d/%d", "scalexyz", XSCALE, YSCALE);
        put_parm(" %s=%d", "roughness", ROUGH);
        put_parm(" %s=%d", "waterline", WATERLINE);
        if (FILLTYPE != +fill_type::POINTS)
        {
            put_parm(" %s=%d", "filltype", FILLTYPE);
        }
        if (g_transparent_color_3d[0] || g_transparent_color_3d[1])
        {
            put_parm(" %s=%d/%d", "transparent", g_transparent_color_3d[0], g_transparent_color_3d[1]);
        }
        if (g_preview)
        {
            put_parm(" %s=%s", "preview", "yes");
            if (g_show_box)
            {
                put_parm(" %s=%s", "showbox", "yes");
            }
            put_parm(" %s=%d", "coarse", g_preview_factor);
        }
        if (g_raytrace_format != raytrace_formats::none)
        {
            put_parm(" %s=%d", "ray", static_cast<int>(g_raytrace_format));
            if (g_brief)
            {
                put_parm(" %s=y", "brief");
            }
        }
        if (FILLTYPE > +fill_type::SOLID_FILL)
        {
            put_parm(" %s=%d/%d/%d", "lightsource", XLIGHT, YLIGHT, ZLIGHT);
            if (LIGHTAVG)
            {
                put_parm(" %s=%d", "smoothing", LIGHTAVG);
            }
        }
        if (g_randomize_3d)
        {
            put_parm(" %s=%d", "randomize", g_randomize_3d);
        }
        if (g_targa_out)
        {
            put_parm(" %s=y", "fullcolor");
        }
        if (g_gray_flag)
        {
            put_parm(" %s=y", "usegrayscale");
        }
        if (g_ambient)
        {
            put_parm(" %s=%d", "ambient", g_ambient);
        }
        if (g_haze)
        {
            put_parm(" %s=%d", "haze", g_haze);
        }
        if (g_background_color[0] != 51 || g_background_color[1] != 153 || g_background_color[2] != 200)
        {
            put_parm(" %s=%d/%d/%d", "background", g_background_color[0], g_background_color[1],
                     g_background_color[2]);
        }
    }

    if (g_display_3d != display_3d_modes::NONE)
    {
        // universal 3d
        //**** common (fractal & transform) 3d parameters in this section ****
        if (!SPHERE || g_display_3d < display_3d_modes::NONE)
        {
            put_parm(" %s=%d/%d/%d", "rotation", XROT, YROT, ZROT);
        }
        put_parm(" %s=%d", "perspective", ZVIEWER);
        put_parm(" %s=%d/%d", "xyshift", XSHIFT, YSHIFT);
        if (g_adjust_3d_x || g_adjust_3d_y)
        {
            put_parm(" %s=%d/%d", "xyadjust", g_adjust_3d_x, g_adjust_3d_y);
        }
        if (g_glasses_type)
        {
            put_parm(" %s=%d", "stereo", g_glasses_type);
            put_parm(" %s=%d", "interocular", g_eye_separation);
            put_parm(" %s=%d", "converge", g_converge_x_adjust);
            put_parm(" %s=%d/%d/%d/%d", "crop",
                     g_red_crop_left, g_red_crop_right, g_blue_crop_left, g_blue_crop_right);
            put_parm(" %s=%d/%d", "bright",
                     g_red_bright, g_blue_bright);
        }
    }

    //**** universal parameters in this section ****

    if (g_view_window)
    {
        put_parm(" %s=%g/%g", "viewwindows", g_view_reduction, g_final_aspect_ratio);
        if (g_view_crop)
        {
            put_parm("/%s", "yes");
        }
        else
        {
            put_parm("/%s", "no");
        }
        put_parm("/%d/%d", g_view_x_dots, g_view_y_dots);
    }

    if (!colorsonly)
    {
        if (g_color_cycle_range_lo != 1 || g_color_cycle_range_hi != 255)
        {
            put_parm(" %s=%d/%d", "cyclerange", g_color_cycle_range_lo, g_color_cycle_range_hi);
        }

        if (g_base_hertz != 440)
        {
            put_parm(" %s=%d", "hertz", g_base_hertz);
        }

        if (g_sound_flag != (SOUNDFLAG_BEEP | SOUNDFLAG_SPEAKER))
        {
            if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_OFF)
            {
                put_parm(" %s=%s", "sound", "off");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_BEEP)
            {
                put_parm(" %s=%s", "sound", "beep");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
            {
                put_parm(" %s=%s", "sound", "x");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
            {
                put_parm(" %s=%s", "sound", "y");
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
            {
                put_parm(" %s=%s", "sound", "z");
            }
            if ((g_sound_flag & SOUNDFLAG_ORBITMASK) && (g_sound_flag & SOUNDFLAG_ORBITMASK) <= SOUNDFLAG_Z)
            {
                if (g_sound_flag & SOUNDFLAG_SPEAKER)
                {
                    put_parm("/pc");
                }
                if (g_sound_flag & SOUNDFLAG_OPL3_FM)
                {
                    put_parm("/fm");
                }
                if (g_sound_flag & SOUNDFLAG_MIDI)
                {
                    put_parm("/midi");
                }
                if (g_sound_flag & SOUNDFLAG_QUANTIZED)
                {
                    put_parm("/quant");
                }
            }
        }

        if (g_fm_volume != 63)
        {
            put_parm(" %s=%d", "volume", g_fm_volume);
        }

        if (g_hi_attenuation != 0)
        {
            if (g_hi_attenuation == 1)
            {
                put_parm(" %s=%s", "attenuate", "low");
            }
            else if (g_hi_attenuation == 2)
            {
                put_parm(" %s=%s", "attenuate", "mid");
            }
            else if (g_hi_attenuation == 3)
            {
                put_parm(" %s=%s", "attenuate", "high");
            }
            else   // just in case
            {
                put_parm(" %s=%s", "attenuate", "none");
            }
        }

        if (g_polyphony != 0)
        {
            put_parm(" %s=%d", "polyphony", g_polyphony+1);
        }

        if (g_fm_wavetype != 0)
        {
            put_parm(" %s=%d", "wavetype", g_fm_wavetype);
        }

        if (g_fm_attack != 5)
        {
            put_parm(" %s=%d", "attack", g_fm_attack);
        }

        if (g_fm_decay != 10)
        {
            put_parm(" %s=%d", "decay", g_fm_decay);
        }

        if (g_fm_sustain != 13)
        {
            put_parm(" %s=%d", "sustain", g_fm_sustain);
        }

        if (g_fm_release != 5)
        {
            put_parm(" %s=%d", "srelease", g_fm_release);
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
                put_parm(" %s=%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", "scalemap", g_scale_map[0], g_scale_map[1], g_scale_map[2], g_scale_map[3]
                         , g_scale_map[4], g_scale_map[5], g_scale_map[6], g_scale_map[7], g_scale_map[8]
                         , g_scale_map[9], g_scale_map[10], g_scale_map[11]);
            }
        }

        if (!g_bof_match_book_images)
        {
            put_parm(" %s=%s", "nobof", "yes");
        }

        if (g_orbit_delay > 0)
        {
            put_parm(" %s=%d", "orbitdelay", g_orbit_delay);
        }

        if (g_orbit_interval != 1)
        {
            put_parm(" %s=%d", "orbitinterval", g_orbit_interval);
        }

        if (g_start_show_orbit)
        {
            put_parm(" %s=%s", "showorbit", "yes");
        }

        if (g_keep_screen_coords)
        {
            put_parm(" %s=%s", "screencoords", "yes");
        }

        if (g_user_std_calc_mode == 'o' && g_set_orbit_corners && g_keep_screen_coords)
        {
            int xdigits;
            int ydigits;
            put_parm(" %s=", "orbitcorners");
            xdigits = getprec(g_orbit_corner_min_x, g_orbit_corner_max_x, g_orbit_corner_3_x);
            ydigits = getprec(g_orbit_corner_min_y, g_orbit_corner_max_y, g_orbit_corner_3_y);
            put_float(0, g_orbit_corner_min_x, xdigits);
            put_float(1, g_orbit_corner_max_x, xdigits);
            put_float(1, g_orbit_corner_min_y, ydigits);
            put_float(1, g_orbit_corner_max_y, ydigits);
            if (g_orbit_corner_3_x != g_orbit_corner_min_x || g_orbit_corner_3_y != g_orbit_corner_min_y)
            {
                put_float(1, g_orbit_corner_3_x, xdigits);
                put_float(1, g_orbit_corner_3_y, ydigits);
            }
        }

        if (g_draw_mode != 'r')
        {
            put_parm(" %s=%c", "orbitdrawmode", g_draw_mode);
        }

        if (g_math_tol[0] != 0.05 || g_math_tol[1] != 0.05)
        {
            put_parm(" %s=%g/%g", "mathtolerance", g_math_tol[0], g_math_tol[1]);
        }

    }

    if (*colorinf != 'n')
    {
        if (g_record_colors == record_colors_mode::comment && *colorinf == '@')
        {
            put_parm_line();
            put_parm("; %s=", "colors");
            put_parm(colorinf);
            put_parm_line();
        }
docolors:
        put_parm(" %s=", "colors");
        if (g_record_colors != record_colors_mode::comment && g_record_colors != record_colors_mode::yes && *colorinf == '@')
        {
            put_parm(colorinf);
        }
        else
        {
            int curc;
            int scanc;
            int force;
            int diffmag = -1;
            int delta;
            int diff1[4][3];
            int diff2[4][3];
            force = 0;
            curc = force;
#ifdef XFRACT
            if (g_fake_lut && g_true_mode == true_color_mode::default_color)
            {
                loaddac(); // stupid kludge
            }
#endif
            while (true)
            {
                // emit color in rgb 3 char encoded form
                for (int j = 0; j < 3; ++j)
                {
                    int k = g_dac_box[curc][j];
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
                put_parm(buf);
                if (++curc >= maxcolor)        // quit if done last color
                {
                    break;
                }
                if (g_debug_flag == debug_flags::force_lossless_colormap)    // lossless compression
                {
                    continue;
                }
                /* Next a P Branderhorst special, a tricky scan for smooth-shaded
                   ranges which can be written as <nn> to compress .par file entry.
                   Method used is to check net change in each color value over
                   spans of 2 to 5 color numbers.  First time for each span size
                   the value change is noted.  After first time the change is
                   checked against noted change.  First time it differs, a
                   a difference of 1 is tolerated and noted as an alternate
                   acceptable change.  When change is not one of the tolerated
                   values, loop exits. */
                if (force)
                {
                    --force;
                    continue;
                }
                scanc = curc;
                int k;
                while (scanc < maxcolor)
                {
                    // scan while same diff to next
                    int i = scanc - curc;
                    if (i > 3)   // check spans up to 4 steps
                    {
                        i = 3;
                    }
                    for (k = 0; k <= i; ++k)
                    {
                        int j;
                        for (j = 0; j < 3; ++j)
                        {
                            // check pattern of chg per color
                            if (g_debug_flag != debug_flags::allow_large_colormap_changes
                                && scanc > (curc+4) && scanc < maxcolor-5)
                            {
                                if (std::abs(2*g_dac_box[scanc][j] - g_dac_box[scanc-5][j]
                                        - g_dac_box[scanc+5][j]) >= 2)
                                {
                                    break;
                                }
                            }
                            delta = (int)g_dac_box[scanc][j] - (int)g_dac_box[scanc-k-1][j];
                            if (k == scanc - curc)
                            {
                                diff2[k][j] = delta;
                                diff1[k][j] = diff2[k][j];
                            }
                            else if (delta != diff1[k][j] && delta != diff2[k][j])
                            {
                                diffmag = std::abs(delta - diff1[k][j]);
                                if (diff1[k][j] != diff2[k][j] || diffmag != 1)
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
                    ++scanc;
                }
                // now scanc-1 is next color which must be written explicitly
                if (scanc - curc > 2)
                {
                    // good, we have a shaded range
                    if (scanc != maxcolor)
                    {
                        if (diffmag < 3)
                        {
                            // not a sharp slope change?
                            force = 2;       // force more between ranges, to stop
                            --scanc;         // "drift" when load/store/load/store/
                        }
                        if (k)
                        {
                            // more of the same
                            force += k;
                            --scanc;
                        }
                    }
                    if (--scanc - curc > 1)
                    {
                        put_parm("<%d>", scanc-curc);
                        curc = scanc;
                    }
                    else                  // changed our mind
                    {
                        force = 0;
                    }
                }
            }
        }
    }

    while (s_wbdata.len)   // flush the buffer
    {
        put_parm_line();
    }

    restore_stack(saved);
}

static void put_filename(char const *keyword, char const *fname)
{
    char const *p;
    if (*fname && !endswithslash(fname))
    {
        p = std::strrchr(fname, SLASHC);
        if (p != nullptr)
        {
            fname = p+1;
            if (*fname == 0)
            {
                return;
            }
        }
        put_parm(" %s=%s", keyword, fname);
    }
}

static void put_parm(char const *parm, ...)
{
    char *bufptr;
    std::va_list args;

    va_start(args, parm);
    if (*parm == ' '             // starting a new parm
        && s_wbdata.len == 0)         // skip leading space
    {
        ++parm;
    }
    bufptr = s_wbdata.buf + s_wbdata.len;
    std::vsprintf(bufptr, parm, args);
    while (*(bufptr++))
    {
        ++s_wbdata.len;
    }
    while (s_wbdata.len > 200)
    {
        put_parm_line();
    }
}

inline int nice_line_length()
{
    return g_max_line_length-4;
}

static void put_parm_line()
{
    int len = s_wbdata.len;
    int c;
    if (len > nice_line_length())
    {
        len = nice_line_length()+1;
        while (--len != 0 && s_wbdata.buf[len] != ' ')
        {
        }
        if (len == 0)
        {
            len = nice_line_length()-1;
            while (++len < g_max_line_length
                && s_wbdata.buf[len]
                && s_wbdata.buf[len] != ' ')
            {
            }
        }
    }
    c = s_wbdata.buf[len];
    s_wbdata.buf[len] = 0;
    std::fputs("  ", parmfile);
    std::fputs(s_wbdata.buf, parmfile);
    if (c && c != ' ')
    {
        std::fputc('\\', parmfile);
    }
    std::fputc('\n', parmfile);
    s_wbdata.buf[len] = (char)c;
    if (c == ' ')
    {
        ++len;
    }
    s_wbdata.len -= len;
    std::strcpy(s_wbdata.buf, s_wbdata.buf+len);
}

/*
   Strips zeros from the non-exponent part of a number. This logic
   was originally in put_bf(), but is split into this routine so it can be
   shared with put_float().
*/

static void strip_zeros(char *buf)
{
    strlwr(buf);
    char *dptr = std::strchr(buf, '.');
    if (dptr != nullptr)
    {
        char *bptr;
        ++dptr;
        char *exptr = std::strchr(buf, 'e');
        if (exptr != nullptr)    // scientific notation with 'e'?
        {
            bptr = exptr;
        }
        else
        {
            bptr = buf + std::strlen(buf);
        }
        while (--bptr > dptr && *bptr == '0')
        {
            *bptr = 0;
        }
        if (exptr && bptr < exptr -1)
        {
            std::strcat(buf, exptr);
        }
    }
}

static void put_float(int slash, double fnum, int prec)
{
    char buf[40];
    char *bptr;
    bptr = buf;
    if (slash)
    {
        *(bptr++) = '/';
    }
    /* Idea of long double cast is to squeeze out another digit or two
       which might be needed (we have found cases where this digit makes
       a difference.) But lets not do this at lower precision */

    if (prec > 15)
    {
        std::sprintf(bptr, "%1.*Lg", prec, (long double)fnum);
    }
    else
    {
        std::sprintf(bptr, "%1.*g", prec, (double)fnum);
    }
    strip_zeros(bptr);
    put_parm(buf);
}

static void put_bf(int slash, bf_t r, int prec)
{
    std::vector<char> buf;              // "/-1.xxxxxxE-1234"
    buf.resize(5000);
    char *bptr = &buf[0];
    if (slash)
    {
        *(bptr++) = '/';

    }
    bftostr(bptr, prec, r);
    strip_zeros(bptr);
    put_parm(&buf[0]);
}
