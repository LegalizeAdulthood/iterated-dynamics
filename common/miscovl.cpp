/*
        Overlayed odds and ends that don't fit anywhere else.
*/
#include "port.h"
#include "prototyp.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "fractype.h"
#include "framain2.h"
#include "helpdefs.h"
#include "id_data.h"
#include "jb.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "miscovl.h"
#include "miscres.h"
#include "os.h"
#include "parser.h"
#include "plot3d.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"

#include <stdio.h>
#if defined(XFRACT)
#include <unistd.h>
#else
#include <process.h>
#endif

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

static void put_parm(char const *parm, ...);

static void put_parm_line();
static int getprec(double, double, double);
int getprecbf(int);
static void put_float(int, double, int);
static void put_bf(int slash, bf_t r, int prec);
static void put_filename(char const *keyword, char const *fname);
static int check_modekey(int curkey, int choice);
static int entcompare(const void *p1, const void *p2);
static void update_fractint_cfg();
static void strip_zeros(char *buf);

bool g_is_true_color = false;
bool g_make_parameter_file = false;
bool g_make_parameter_file_map = false;

static char par_comment[4][MAX_COMMENT_LEN];

// JIIM

static FILE *parmfile;

#define PAR_KEY(x)  ( x < 10 ? '0' + x : 'a' - 10 + x)

void make_batch_file()
{
#define MAXPROMPTS 18
    bool colorsonly = false;
    // added for pieces feature
    double pdelx = 0.0;
    double pdely = 0.0;
    double pdelx2 = 0.0;
    double pdely2 = 0.0;
    unsigned int pxdots, pydots, xm, ym;
    double pxxmin = 0.0, pyymax = 0.0;
    char vidmde[5];
    int promptnum;
    int piecespromts;
    bool have3rd = false;
    char inpcommandfile[80], inpcommandname[ITEM_NAME_LEN+1];
    char inpcomment[4][MAX_COMMENT_LEN];
    fullscreenvalues paramvalues[18];
    char const *choices[MAXPROMPTS];
    char outname[FILE_MAX_PATH+1], buf[256], buf2[128];
    FILE *infile = nullptr;
    FILE *fpbat = nullptr;
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
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPPARMFILE;

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

        if (g_color_state == 0)
        {
            // default colors
            if (g_map_specified)
            {
                colorspec[0] = '@';
                sptr = g_map_name.c_str();
            }
        }
        else if (g_color_state == 2)
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
        g_command_comment[i] = expand_comments(par_comment[i]);
        std::strcpy(inpcomment[i], g_command_comment[i].c_str());
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
#ifndef XFRACT
        choices[promptnum] = "Video mode";
        paramvalues[promptnum].type = 0x100 + 4;
        paramvalues[promptnum++].uval.sbuf = vidmde;
#endif

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
            long xtotal, ytotal;
#ifndef XFRACT
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
                stopmsg(STOPMSG_NONE, "Invalid video mode entry!");
                goto prompt_user;
            }
#endif

            // bounds range on xm, ym
            if (xm < 1 || xm > 36 || ym < 1 || ym > 36)
            {
                stopmsg(STOPMSG_NONE, "X and Y components must be 1 to 36");
                goto prompt_user;
            }

            // another sanity check: total resolution cannot exceed 65535
            xtotal = xm;
            ytotal = ym;
            xtotal *= pxdots;
            ytotal *= pydots;
            if (xtotal > 65535L || ytotal > 65535L)
            {
                stopmsg(STOPMSG_NONE, "Total resolution (X or Y) cannot exceed 65535");
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
        std::strcpy(outname, g_command_file.c_str());
        bool gotinfile = false;
        if (access(g_command_file.c_str(), 0) == 0)
        {
            // file exists
            gotinfile = true;
            if (access(g_command_file.c_str(), 6))
            {
                sprintf(buf, "Can't write %s", g_command_file.c_str());
                stopmsg(STOPMSG_NONE, buf);
                continue;
            }
            int i = (int) std::strlen(outname);
            while (--i >= 0 && outname[i] != SLASHC)
            {
                outname[i] = 0;
            }
            std::strcat(outname, "fractint.tmp");
            infile = fopen(g_command_file.c_str(), "rt");
        }
        parmfile = fopen(outname, "wt");
        if (parmfile == nullptr)
        {
            sprintf(buf, "Can't create %s", outname);
            stopmsg(STOPMSG_NONE, buf);
            if (gotinfile)
            {
                fclose(infile);
            }
            continue;
        }

        if (gotinfile)
        {
            while (file_gets(buf, 255, infile) >= 0)
            {
                if (std::strchr(buf, '{')// entry heading?
                    && sscanf(buf, " %40[^ \t({]", buf2)
                    && stricmp(buf2, g_command_name.c_str()) == 0)
                {
                    // entry with same name
                    _snprintf(buf2, NUM_OF(buf2), "File already has an entry named %s\n%s",
                              g_command_name.c_str(), g_make_parameter_file ?
                              "... Replacing ..." : "Continue to replace it, Cancel to back out");
                    if (stopmsg(STOPMSG_CANCEL | STOPMSG_INFO_ONLY, buf2))
                    {
                        // cancel
                        fclose(infile);
                        fclose(parmfile);
                        unlink(outname);
                        goto prompt_user;
                    }
                    while (std::strchr(buf, '}') == nullptr
                        && file_gets(buf, 255, infile) > 0)
                    {
                        ; // skip to end of set
                    }
                    break;
                }
                fputs(buf, parmfile);
                fputc('\n', parmfile);
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
                        char buf[20];
                        sprintf(buf, "_%c%c", PAR_KEY(i), PAR_KEY(j));
                        std::strcat(PCommandName, buf);
                    }
                    fprintf(parmfile, "%-19s{", PCommandName);
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
                    fprintf(fpbat, "Fractint batch=yes overwrite=yes @%s/%s\n", g_command_file.c_str(), PCommandName);
                    fprintf(fpbat, "If Errorlevel 2 goto oops\n");
                }
                else
                {
                    fprintf(parmfile, "%-19s{", g_command_name.c_str());
                }
                {
                    /* guarantee that there are no blank comments above the last
                       non-blank par_comment */
                    int last = -1;
                    for (int i = 0; i < 4; i++)
                    {
                        if (*par_comment[i])
                        {
                            last = i;
                        }
                    }
                    for (int i = 0; i < last; i++)
                    {
                        if (g_command_comment[i].empty())
                        {
                            g_command_comment[i] = ";";
                        }
                    }
                }
                if (!g_command_comment[0].empty())
                {
                    fprintf(parmfile, " ; %s", g_command_comment[0].c_str());
                }
                fputc('\n', parmfile);
                {
                    char buf[25];
                    std::memset(buf, ' ', 23);
                    buf[23] = 0;
                    buf[21] = ';';
                    for (int k = 1; k < 4; k++)
                    {
                        if (!g_command_comment[k].empty())
                        {
                            fprintf(parmfile, "%s%s\n", buf, g_command_comment[k].c_str());
                        }
                    }
                    if (g_patch_level != 0 && !colorsonly)
                    {
                        fprintf(parmfile, "%s %s Version %d Patchlevel %d\n", buf,
                                Fractint, g_release, g_patch_level);
                    }
                }
                write_batch_parms(colorspec, colorsonly, maxcolor, i, j);
                if (xm > 1 || ym > 1)
                {
                    fprintf(parmfile, "  video=%s", vidmde);
                    fprintf(parmfile, " savename=frmig_%c%c\n", PAR_KEY(i), PAR_KEY(j));
                }
                fprintf(parmfile, "  }\n\n");
            }
        }
        if (xm > 1 || ym > 1)
        {
            fprintf(fpbat, "Fractint makemig=%u/%u\n", xm, ym);
            fprintf(fpbat, "Rem Simplgif fractmig.gif simplgif.gif  in case you need it\n");
            fprintf(fpbat, ":oops\n");
            fclose(fpbat);
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
                fputs(buf, parmfile);
                fputc('\n', parmfile);
                i = file_gets(buf, 255, infile);
            }
            fclose(infile);
        }
        fclose(parmfile);
        if (gotinfile)
        {
            // replace the original file with the new
            unlink(g_command_file.c_str());   // success assumed on these lines
            rename(outname, g_command_file.c_str());  // since we checked earlier with access
        }
        break;
    }
    g_help_mode = old_help_mode;
    driver_unstack_screen();
}

struct write_batch_data // buffer for parms to break lines nicely
{
    int len;
    char buf[10000];
};
static write_batch_data s_wbdata;

void write_batch_parms(char const *colorinf, bool colorsonly, int maxcolor, int ii, int jj)
{
    double Xctr, Yctr;
    LDBL Magnification;
    double Xmagfactor, Rotation, Skew;
    char const *sptr;
    char buf[81];
    bf_t bfXctr = nullptr, bfYctr = nullptr;
    int saved;
    saved = save_stack();
    if (bf_math != bf_math_type::NONE)
    {
        bfXctr = alloc_stack(bflength+2);
        bfYctr = alloc_stack(bflength+2);
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
                name = g_fractal_specific[static_cast<int>(g_new_orbit_type)].name;
                if (*name == '*')
                {
                    name++;
                }
                put_parm(" %s=%s", "orbitname", name);
            }
            if (g_julibrot_3d_mode != 0)
            {
                put_parm(" %s=%s", "3dmode", g_julibrot_3d_options[g_julibrot_3d_mode].c_str());
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
            put_parm(" %s=%s/%s", "miim",
                     g_jiim_method[static_cast<int>(g_major_method)].c_str(),
                     g_jiim_left_right[static_cast<int>(g_inverse_julia_minor_method)].c_str());
        }

        showtrig(buf); // this function is in miscres.c
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
            if (bf_math != bf_math_type::NONE)
            {
                int digits;
                cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
                digits = getprecbf(MAXREZ);
                put_parm(" %s=", "center-mag");
                put_bf(0, bfXctr, digits);
                put_bf(1, bfYctr, digits);
            }
            else // !bf_math
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
            if (bf_math != bf_math_type::NONE)
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
                int xdigits, ydigits;
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
                stopmsg(STOPMSG_NONE, "Regenerate before <b> to get correct symmetry");
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
        if (FILLTYPE)
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
        if (FILLTYPE > 4)
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
#ifndef XFRACT
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
#endif
        }

#ifndef XFRACT
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
#endif

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
            int xdigits, ydigits;
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
            int curc, scanc, force, diffmag = -1;
            int delta, diff1[4][3], diff2[4][3];
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
    vsprintf(bufptr, parm, args);
    while (*(bufptr++))
    {
        ++s_wbdata.len;
    }
    while (s_wbdata.len > 200)
    {
        put_parm_line();
    }
}

int g_max_line_length = 72;

inline int nice_line_length()
{
    return g_max_line_length-4;
}

static void put_parm_line()
{
    int len = s_wbdata.len, c;
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
    fputs("  ", parmfile);
    fputs(s_wbdata.buf, parmfile);
    if (c && c != ' ')
    {
        fputc('\\', parmfile);
    }
    fputc('\n', parmfile);
    s_wbdata.buf[len] = (char)c;
    if (c == ' ')
    {
        ++len;
    }
    s_wbdata.len -= len;
    std::strcpy(s_wbdata.buf, s_wbdata.buf+len);
}

int getprecbf_mag()
{
    double Xmagfactor, Rotation, Skew;
    LDBL Magnification;
    bf_t bXctr, bYctr;
    int saved, dec;

    saved = save_stack();
    bXctr            = alloc_stack(bflength+2);
    bYctr            = alloc_stack(bflength+2);
    // this is just to find Magnification
    cvtcentermagbf(bXctr, bYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
    restore_stack(saved);

    // I don't know if this is portable, but something needs to
    // be used in case compiler's LDBL_MAX is not big enough
    if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
    {
        return -1;
    }

    dec = getpower10(Magnification) + 4; // 4 digits of padding sounds good
    return dec;
}

static int getprec(double a, double b, double c)
{
    double diff, temp;
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
    if (g_debug_flag >= debug_flags::force_precision_0_digits && g_debug_flag < debug_flags::force_precision_20_digits)
    {
        digits = g_debug_flag - debug_flags::force_precision_0_digits;
    }
    while (diff < 1.0 && digits <= DBL_DIG+1)
    {
        diff *= 10;
        ++digits;
    }
    return digits;
}

/* This function calculates the precision needed to distiguish adjacent
   pixels at Fractint's maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if rez==MAXREZ) or at current resolution (if rez==CURRENTREZ)    */
int getprecbf(int rezflag)
{
    bf_t del1, del2, one, bfxxdel, bfxxdel2, bfyydel, bfyydel2;
    int digits, dec;
    int saved;
    int rez;
    saved    = save_stack();
    del1     = alloc_stack(bflength+2);
    del2     = alloc_stack(bflength+2);
    one      = alloc_stack(bflength+2);
    bfxxdel   = alloc_stack(bflength+2);
    bfxxdel2  = alloc_stack(bflength+2);
    bfyydel   = alloc_stack(bflength+2);
    bfyydel2  = alloc_stack(bflength+2);
    floattobf(one, 1.0);
    if (rezflag == MAXREZ)
    {
        rez = OLD_MAX_PIXELS -1;
    }
    else
    {
        rez = g_logical_screen_x_dots-1;
    }

    // bfxxdel = (bfxmax - bfx3rd)/(xdots-1)
    sub_bf(bfxxdel, g_bf_x_max, g_bf_x_3rd);
    div_a_bf_int(bfxxdel, (U16)rez);

    // bfyydel2 = (bfy3rd - bfymin)/(xdots-1)
    sub_bf(bfyydel2, g_bf_y_3rd, g_bf_y_min);
    div_a_bf_int(bfyydel2, (U16)rez);

    if (rezflag == CURRENTREZ)
    {
        rez = g_logical_screen_y_dots-1;
    }

    // bfyydel = (bfymax - bfy3rd)/(ydots-1)
    sub_bf(bfyydel, g_bf_y_max, g_bf_y_3rd);
    div_a_bf_int(bfyydel, (U16)rez);

    // bfxxdel2 = (bfx3rd - bfxmin)/(ydots-1)
    sub_bf(bfxxdel2, g_bf_x_3rd, g_bf_x_min);
    div_a_bf_int(bfxxdel2, (U16)rez);

    abs_a_bf(add_bf(del1, bfxxdel, bfxxdel2));
    abs_a_bf(add_bf(del2, bfyydel, bfyydel2));
    if (cmp_bf(del2, del1) < 0)
    {
        copy_bf(del1, del2);
    }
    if (cmp_bf(del1, clear_bf(del2)) == 0)
    {
        restore_stack(saved);
        return -1;
    }
    digits = 1;
    while (cmp_bf(del1, one) < 0)
    {
        digits++;
        mult_a_bf_int(del1, 10);
    }
    digits = std::max(digits, 3);
    restore_stack(saved);
    dec = getprecbf_mag();
    return std::max(digits, dec);
}

/* This function calculates the precision needed to distinguish adjacent
   pixels at Fractint's maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if rez==MAXREZ) or at current resolution (if rez==CURRENTREZ)    */
int getprecdbl(int rezflag)
{
    LDBL del1, del2, xdel, xdel2, ydel, ydel2;
    int digits;
    LDBL rez;
    if (rezflag == MAXREZ)
    {
        rez = OLD_MAX_PIXELS -1;
    }
    else
    {
        rez = g_logical_screen_x_dots-1;
    }

    xdel = ((LDBL)g_x_max - (LDBL)g_x_3rd)/rez;
    ydel2 = ((LDBL)g_y_3rd - (LDBL)g_y_min)/rez;

    if (rezflag == CURRENTREZ)
    {
        rez = g_logical_screen_y_dots-1;
    }

    ydel = ((LDBL)g_y_max - (LDBL)g_y_3rd)/rez;
    xdel2 = ((LDBL)g_x_3rd - (LDBL)g_x_min)/rez;

    del1 = fabsl(xdel) + fabsl(xdel2);
    del2 = fabsl(ydel) + fabsl(ydel2);
    if (del2 < del1)
    {
        del1 = del2;
    }
    if (del1 == 0)
    {
#ifdef DEBUG
        showcornersdbl("getprecdbl");
#endif
        return -1;
    }
    digits = 1;
    while (del1 < 1.0)
    {
        digits++;
        del1 *= 10;
    }
    digits = std::max(digits, 3);
    return digits;
}

/*
   Strips zeros from the non-exponent part of a number. This logic
   was originally in put_bf(), but is split into this routine so it can be
   shared with put_float(), which had a bug in Fractint 19.2 (used to strip
   zeros from the exponent as well.)
*/

static void strip_zeros(char *buf)
{
    char *dptr, *bptr, *exptr;
    strlwr(buf);
    dptr = std::strchr(buf, '.');
    if (dptr != nullptr)
    {
        ++dptr;
        exptr = std::strchr(buf, 'e');
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
        sprintf(bptr, "%1.*Lg", prec, (long double)fnum);
    }
    else
    {
        sprintf(bptr, "%1.*g", prec, (double)fnum);
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

static int entnums[MAX_VIDEO_MODES];
static bool modes_changed = false;

int select_video_mode(int curmode)
{
    int attributes[MAX_VIDEO_MODES];
    int ret;

    for (int i = 0; i < g_video_table_len; ++i)  // init tables
    {
        entnums[i] = i;
        attributes[i] = 1;
    }

    qsort(entnums, g_video_table_len, sizeof(entnums[0]), entcompare); // sort modes

    // pick default mode
    if (curmode < 0)
    {
        g_video_entry.videomodeax = 19;  // vga
        g_video_entry.colors = 256;
    }
    else
    {
        std::memcpy((char *) &g_video_entry, (char *) &g_video_table[curmode], sizeof(g_video_entry));
    }
    int i;
    for (i = 0; i < g_video_table_len; ++i)  // find default mode
    {
        if (g_video_entry.videomodeax == g_video_table[entnums[i]].videomodeax
            && g_video_entry.colors == g_video_table[entnums[i]].colors
            && (curmode < 0
                || std::memcmp((char *) &g_video_entry, (char *) &g_video_table[entnums[i]], sizeof(g_video_entry)) == 0))
        {
            break;
        }
    }
    if (i >= g_video_table_len) // no match, default to first entry
    {
        i = 0;
    }

    bool const old_tab_mode = g_tab_mode;
    help_labels const old_help_mode = g_help_mode;
    modes_changed = false;
    g_tab_mode = false;
    g_help_mode = help_labels::HELPVIDSEL;
    i = fullscreen_choice(CHOICE_HELP,
                          "Select Video Mode",
                          "key...name.......................xdot..ydot.colr.driver......comment......",
                          nullptr, g_video_table_len, nullptr, attributes,
                          1, 16, 74, i, format_vid_table, nullptr, nullptr, check_modekey);
    g_tab_mode = old_tab_mode;
    g_help_mode = old_help_mode;
    if (i == -1)
    {
        // update fractint.cfg for new key assignments
        if (modes_changed
            && g_bad_config == 0
            && stopmsg(STOPMSG_CANCEL | STOPMSG_NO_BUZZER | STOPMSG_INFO_ONLY,
                "Save new function key assignments or cancel changes?") == 0)
        {
            update_fractint_cfg();
        }
        return -1;
    }
    // picked by function key or ENTER key
    i = (i < 0) ? (-1 - i) : entnums[i];
    // the selected entry now in g_video_entry
    std::memcpy((char *) &g_video_entry, (char *) &g_video_table[i], sizeof(g_video_entry));

    // copy fractint.cfg table to resident table, note selected entry
    int k = 0;
    for (i = 0; i < g_video_table_len; ++i)
    {
        if (g_video_table[i].keynum > 0)
        {
            if (std::memcmp((char *)&g_video_entry, (char *)&g_video_table[i], sizeof(g_video_entry)) == 0)
            {
                k = g_video_table[i].keynum;
            }
        }
    }
    ret = k;
    if (k == 0)  // selected entry not a copied (assigned to key) one
    {
        std::memcpy((char *)&g_video_table[MAX_VIDEO_MODES-1],
               (char *)&g_video_entry, sizeof(*g_video_table));
        ret = 1400; // special value for check_vidmode_key
    }

    // update fractint.cfg for new key assignments
    if (modes_changed && g_bad_config == 0)
    {
        update_fractint_cfg();
    }

    return ret;
}

void format_vid_table(int choice, char *buf)
{
    char local_buf[81];
    char kname[5];
    int truecolorbits;
    std::memcpy((char *)&g_video_entry, (char *)&g_video_table[entnums[choice]],
           sizeof(g_video_entry));
    vidmode_keyname(g_video_entry.keynum, kname);
    sprintf(buf, "%-5s %-25s %5d %5d ",  // 44 chars
            kname, g_video_entry.name, g_video_entry.xdots, g_video_entry.ydots);
    truecolorbits = g_video_entry.dotmode/1000;
    if (truecolorbits == 0)
    {
        sprintf(local_buf, "%s%3d",  // 47 chars
                buf, g_video_entry.colors);
    }
    else
    {
        sprintf(local_buf, "%s%3s",  // 47 chars
                buf, (truecolorbits == 4)?" 4g":
                (truecolorbits == 3)?"16m":
                (truecolorbits == 2)?"64k":
                (truecolorbits == 1)?"32k":"???");
    }
    sprintf(buf, "%s %.12s %.12s",  // 74 chars
            local_buf, g_video_entry.driver->name, g_video_entry.comment);
}

static int check_modekey(int curkey, int choice)
{
    int i = check_vidmode_key(1, curkey);
    if (i >= 0)
    {
        return -1-i;
    }
    i = entnums[choice];
    int ret = 0;
    if ((curkey == '-' || curkey == '+')
        && (g_video_table[i].keynum == 0 || g_video_table[i].keynum >= 1084))
    {
        if (g_bad_config)
        {
            stopmsg(STOPMSG_NONE, "Missing or bad FRACTINT.CFG file. Can't reassign keys.");
        }
        else
        {
            if (curkey == '-')
            {
                // deassign key?
                if (g_video_table[i].keynum >= 1084)
                {
                    g_video_table[i].keynum = 0;
                    modes_changed = true;
                }
            }
            else
            {
                // assign key?
                int j = getakeynohelp();
                if (j >= 1084 && j <= 1113)
                {
                    for (int k = 0; k < g_video_table_len; ++k)
                    {
                        if (g_video_table[k].keynum == j)
                        {
                            g_video_table[k].keynum = 0;
                            ret = -1; // force redisplay
                        }
                    }
                    g_video_table[i].keynum = j;
                    modes_changed = true;
                }
            }
        }
    }
    return ret;
}

static int entcompare(const void *p1, const void *p2)
{
    int i, j;
    i = g_video_table[*((int *)p1)].keynum;
    if (i == 0)
    {
        i = 9999;
    }
    j = g_video_table[*((int *)p2)].keynum;
    if (j == 0)
    {
        j = 9999;
    }
    if (i < j || (i == j && *((int *)p1) < *((int *)p2)))
    {
        return -1;
    }
    return 1;
}

static void update_fractint_cfg()
{
#ifndef XFRACT
    char cfgname[100], outname[100], buf[121], kname[5];
    FILE *cfgfile, *outfile;
    int i, j, linenum, nextlinenum, nextmode;
    VIDEOINFO vident;

    findpath("fractint.cfg", cfgname);

    if (access(cfgname, 6))
    {
        sprintf(buf, "Can't write %s", cfgname);
        stopmsg(STOPMSG_NONE, buf);
        return;
    }
    std::strcpy(outname, cfgname);
    i = (int) std::strlen(outname);
    while (--i >= 0 && outname[i] != SLASHC)
    {
        outname[i] = 0;
    }
    std::strcat(outname, "fractint.tmp");
    outfile = fopen(outname, "w");
    if (outfile == nullptr)
    {
        sprintf(buf, "Can't create %s", outname);
        stopmsg(STOPMSG_NONE, buf);
        return;
    }
    cfgfile = fopen(cfgname, "r");

    nextmode = 0;
    linenum = nextmode;
    nextlinenum = g_cfg_line_nums[0];
    while (fgets(buf, 120, cfgfile))
    {
        char colorsbuf[10];
        ++linenum;
        if (linenum == nextlinenum)
        {
            // replace this line
            std::memcpy((char *)&vident, (char *)&g_video_table[nextmode],
                   sizeof(g_video_entry));
            vidmode_keyname(vident.keynum, kname);
            std::strcpy(buf, vident.name);
            i = (int) std::strlen(buf);
            while (i && buf[i-1] == ' ') // strip trailing spaces to compress
            {
                --i;
            }
            j = i + 5;
            while (j < 32)
            {
                // tab to column 33
                buf[i++] = '\t';
                j += 8;
            }
            buf[i] = 0;
            int truecolorbits = vident.dotmode/1000;
            if (truecolorbits == 0)
            {
                sprintf(colorsbuf, "%3d", vident.colors);
            }
            else
            {
                sprintf(colorsbuf, "%3s",
                        (truecolorbits == 4)?" 4g":
                        (truecolorbits == 3)?"16m":
                        (truecolorbits == 2)?"64k":
                        (truecolorbits == 1)?"32k":"???");
            }
            fprintf(outfile, "%-4s,%s,%4x,%4x,%4x,%4x,%4d,%5d,%5d,%s,%s\n",
                    kname,
                    buf,
                    vident.videomodeax,
                    vident.videomodebx,
                    vident.videomodecx,
                    vident.videomodedx,
                    vident.dotmode%1000, // remove true-color flag, keep g_text_safe
                    vident.xdots,
                    vident.ydots,
                    colorsbuf,
                    vident.comment);
            if (++nextmode >= g_video_table_len)
            {
                nextlinenum = 32767;
            }
            else
            {
                nextlinenum = g_cfg_line_nums[nextmode];
            }
        }
        else
        {
            fputs(buf, outfile);
        }
    }

    fclose(cfgfile);
    fclose(outfile);
    unlink(cfgname);         // success assumed on these lines
    rename(outname, cfgname); // since we checked earlier with access
#endif
}

/* make_mig() takes a collection of individual GIF images (all
   presumably the same resolution and all presumably generated
   by Fractint and its "divide and conquer" algorithm) and builds
   a single multiple-image GIF out of them.  This routine is
   invoked by the "batch=stitchmode/x/y" option, and is called
   with the 'x' and 'y' parameters
*/

void make_mig(unsigned int xmult, unsigned int ymult)
{
    unsigned int xres, yres;
    unsigned int allxres, allyres, xtot, ytot;
    unsigned int xloc, yloc;
    unsigned char ichar;
    unsigned int allitbl, itbl;
    unsigned int i;
    char gifin[15], gifout[15];
    int errorflag, inputerrorflag;
    unsigned char *temp;
    FILE *out, *in;

    errorflag = 0;                          // no errors so
    inputerrorflag = 0;
    allitbl = 0;
    allyres = allitbl;
    allxres = allyres;
    in = nullptr;
    out = in;

    std::strcpy(gifout, "fractmig.gif");

    temp = &g_old_dac_box[0][0];                 // a safe place for our temp data

    g_gif87a_flag = true;                     // for now, force this

    // process each input image, one at a time
    for (unsigned ystep = 0U; ystep < ymult; ystep++)
    {
        for (unsigned xstep = 0U; xstep < xmult; xstep++)
        {
            if (xstep == 0 && ystep == 0)          // first time through?
            {
                printf(" \n Generating multi-image GIF file %s using", gifout);
                printf(" %u X and %u Y components\n\n", xmult, ymult);
                // attempt to create the output file
                out = fopen(gifout, "wb");
                if (out == nullptr)
                {
                    printf("Cannot create output file %s!\n", gifout);
                    exit(1);
                }
            }

            sprintf(gifin, "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));

            in = fopen(gifin, "rb");
            if (in == nullptr)
            {
                printf("Can't open file %s!\n", gifin);
                exit(1);
            }

            // (read, but only copy this if it's the first time through)
            if (fread(temp, 13, 1, in) != 1)   // read the header and LDS
            {
                inputerrorflag = 1;
            }
            std::memcpy(&xres, &temp[6], 2);     // X-resolution
            std::memcpy(&yres, &temp[8], 2);     // Y-resolution

            if (xstep == 0 && ystep == 0)  // first time through?
            {
                allxres = xres;             // save the "master" resolution
                allyres = yres;
                xtot = xres * xmult;        // adjust the image size
                ytot = yres * ymult;
                std::memcpy(&temp[6], &xtot, 2);
                std::memcpy(&temp[8], &ytot, 2);
                if (g_gif87a_flag)
                {
                    temp[3] = '8';
                    temp[4] = '7';
                    temp[5] = 'a';
                }
                temp[12] = 0; // reserved
                if (fwrite(temp, 13, 1, out) != 1)     // write out the header
                {
                    errorflag = 1;
                }
            }                           // end of first-time-through

            ichar = (char)(temp[10] & 0x07);        // find the color table size
            itbl = 1 << (++ichar);
            ichar = (char)(temp[10] & 0x80);        // is there a global color table?
            if (xstep == 0 && ystep == 0)   // first time through?
            {
                allitbl = itbl;             // save the color table size
            }
            if (ichar != 0)                // yup
            {
                // (read, but only copy this if it's the first time through)
                if (fread(temp, 3*itbl, 1, in) != 1)    // read the global color table
                {
                    inputerrorflag = 2;
                }
                if (xstep == 0 && ystep == 0)       // first time through?
                {
                    if (fwrite(temp, 3*itbl, 1, out) != 1)     // write out the GCT
                    {
                        errorflag = 2;
                    }
                }
            }

            if (xres != allxres || yres != allyres || itbl != allitbl)
            {
                // Oops - our pieces don't match
                printf("File %s doesn't have the same resolution as its predecessors!\n", gifin);
                exit(1);
            }

            while (true)                       // process each information block
            {
                std::memset(temp, 0, 10);
                if (fread(temp, 1, 1, in) != 1)    // read the block identifier
                {
                    inputerrorflag = 3;
                }

                if (temp[0] == 0x2c)           // image descriptor block
                {
                    if (fread(&temp[1], 9, 1, in) != 1)    // read the Image Descriptor
                    {
                        inputerrorflag = 4;
                    }
                    std::memcpy(&xloc, &temp[1], 2); // X-location
                    std::memcpy(&yloc, &temp[3], 2); // Y-location
                    xloc += (xstep * xres);     // adjust the locations
                    yloc += (ystep * yres);
                    std::memcpy(&temp[1], &xloc, 2);
                    std::memcpy(&temp[3], &yloc, 2);
                    if (fwrite(temp, 10, 1, out) != 1)     // write out the Image Descriptor
                    {
                        errorflag = 4;
                    }

                    ichar = (char)(temp[9] & 0x80);     // is there a local color table?
                    if (ichar != 0)            // yup
                    {
                        if (fread(temp, 3*itbl, 1, in) != 1)       // read the local color table
                        {
                            inputerrorflag = 5;
                        }
                        if (fwrite(temp, 3*itbl, 1, out) != 1)     // write out the LCT
                        {
                            errorflag = 5;
                        }
                    }

                    if (fread(temp, 1, 1, in) != 1)        // LZH table size
                    {
                        inputerrorflag = 6;
                    }
                    if (fwrite(temp, 1, 1, out) != 1)
                    {
                        errorflag = 6;
                    }
                    while (true)
                    {
                        if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (fread(temp, 1, 1, in) != 1)    // block size
                        {
                            inputerrorflag = 7;
                        }
                        if (fwrite(temp, 1, 1, out) != 1)
                        {
                            errorflag = 7;
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (fread(temp, i, 1, in) != 1)    // LZH data block
                        {
                            inputerrorflag = 8;
                        }
                        if (fwrite(temp, i, 1, out) != 1)
                        {
                            errorflag = 8;
                        }
                    }
                }

                if (temp[0] == 0x21)           // extension block
                {
                    // (read, but only copy this if it's the last time through)
                    if (fread(&temp[2], 1, 1, in) != 1)    // read the block type
                    {
                        inputerrorflag = 9;
                    }
                    if (!g_gif87a_flag && xstep == xmult-1 && ystep == ymult-1)
                    {
                        if (fwrite(temp, 2, 1, out) != 1)
                        {
                            errorflag = 9;
                        }
                    }
                    while (true)
                    {
                        if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (fread(temp, 1, 1, in) != 1)    // block size
                        {
                            inputerrorflag = 10;
                        }
                        if (!g_gif87a_flag && xstep == xmult-1 && ystep == ymult-1)
                        {
                            if (fwrite(temp, 1, 1, out) != 1)
                            {
                                errorflag = 10;
                            }
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (fread(temp, i, 1, in) != 1)    // data block
                        {
                            inputerrorflag = 11;
                        }
                        if (!g_gif87a_flag && xstep == xmult-1 && ystep == ymult-1)
                        {
                            if (fwrite(temp, i, 1, out) != 1)
                            {
                                errorflag = 11;
                            }
                        }
                    }
                }

                if (temp[0] == 0x3b)           // end-of-stream indicator
                {
                    break;                      // done with this file
                }

                if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                {
                    break;
                }
            }
            fclose(in);                     // done with an input GIF

            if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
            {
                break;
            }
        }

        if (errorflag != 0 || inputerrorflag != 0)  // oops - did something go wrong?
        {
            break;
        }
    }

    temp[0] = 0x3b;                 // end-of-stream indicator
    if (fwrite(temp, 1, 1, out) != 1)
    {
        errorflag = 12;
    }
    fclose(out);                    // done with the output GIF

    if (inputerrorflag != 0)       // uh-oh - something failed
    {
        printf("\007 Process failed = early EOF on input file %s\n", gifin);
        /* following line was for debugging
            printf("inputerrorflag = %d\n", inputerrorflag);
        */
    }

    if (errorflag != 0)            // uh-oh - something failed
    {
        printf("\007 Process failed = out of disk space?\n");
        /* following line was for debugging
            printf("errorflag = %d\n", errorflag);
        */
    }

    // now delete each input image, one at a time
    if (errorflag == 0 && inputerrorflag == 0)
    {
        for (unsigned ystep = 0U; ystep < ymult; ystep++)
        {
            for (unsigned xstep = 0U; xstep < xmult; xstep++)
            {
                sprintf(gifin, "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));
                id_fs_remove(gifin);
            }
        }
    }

    // tell the world we're done
    if (errorflag == 0 && inputerrorflag == 0)
    {
        printf("File %s has been created (and its component files deleted)\n", gifout);
    }
}

/* This routine copies the current screen to by flipping x-axis, y-axis,
   or both. Refuses to work if calculation in progress or if fractal
   non-resumable. Clears zoombox if any. Resets corners so resulting fractal
   is still valid. */
void flip_image(int key)
{
    int ixhalf, iyhalf, tempdot;

    // fractal must be rotate-able and be finished
    if ((g_cur_fractal_specific->flags&NOROTATE) != 0
        || g_calc_status == calc_status_value::IN_PROGRESS
        || g_calc_status == calc_status_value::RESUMABLE)
    {
        return;
    }
    if (bf_math != bf_math_type::NONE)
    {
        clear_zoombox(); // clear, don't copy, the zoombox
    }
    ixhalf = g_logical_screen_x_dots / 2;
    iyhalf = g_logical_screen_y_dots / 2;
    switch (key)
    {
    case 24:            // control-X - reverse X-axis
        for (int i = 0; i < ixhalf; i++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int j = 0; j < g_logical_screen_y_dots; j++)
            {
                tempdot = getcolor(i, j);
                g_put_color(i, j, getcolor(g_logical_screen_x_dots-1-i, j));
                g_put_color(g_logical_screen_x_dots-1-i, j, tempdot);
            }
        }
        g_save_x_min = g_x_max + g_x_min - g_x_3rd;
        g_save_y_max = g_y_max + g_y_min - g_y_3rd;
        g_save_x_max = g_x_3rd;
        g_save_y_min = g_y_3rd;
        g_save_x_3rd = g_x_max;
        g_save_y_3rd = g_y_min;
        if (bf_math != bf_math_type::NONE)
        {
            add_bf(g_bf_save_x_min, g_bf_x_max, g_bf_x_min); // sxmin = xxmax + xxmin - xx3rd;
            sub_a_bf(g_bf_save_x_min, g_bf_x_3rd);
            add_bf(g_bf_save_y_max, g_bf_y_max, g_bf_y_min); // symax = yymax + yymin - yy3rd;
            sub_a_bf(g_bf_save_y_max, g_bf_y_3rd);
            copy_bf(g_bf_save_x_max, g_bf_x_3rd);        // sxmax = xx3rd;
            copy_bf(g_bf_save_y_min, g_bf_y_3rd);        // symin = yy3rd;
            copy_bf(g_bf_save_x_3rd, g_bf_x_max);        // sx3rd = xxmax;
            copy_bf(g_bf_save_y_3rd, g_bf_y_min);        // sy3rd = yymin;
        }
        break;
    case 25:            // control-Y - reverse Y-aXis
        for (int j = 0; j < iyhalf; j++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                tempdot = getcolor(i, j);
                g_put_color(i, j, getcolor(i, g_logical_screen_y_dots-1-j));
                g_put_color(i, g_logical_screen_y_dots-1-j, tempdot);
            }
        }
        g_save_x_min = g_x_3rd;
        g_save_y_max = g_y_3rd;
        g_save_x_max = g_x_max + g_x_min - g_x_3rd;
        g_save_y_min = g_y_max + g_y_min - g_y_3rd;
        g_save_x_3rd = g_x_min;
        g_save_y_3rd = g_y_max;
        if (bf_math != bf_math_type::NONE)
        {
            copy_bf(g_bf_save_x_min, g_bf_x_3rd);        // sxmin = xx3rd;
            copy_bf(g_bf_save_y_max, g_bf_y_3rd);        // symax = yy3rd;
            add_bf(g_bf_save_x_max, g_bf_x_max, g_bf_x_min); // sxmax = xxmax + xxmin - xx3rd;
            sub_a_bf(g_bf_save_x_max, g_bf_x_3rd);
            add_bf(g_bf_save_y_min, g_bf_y_max, g_bf_y_min); // symin = yymax + yymin - yy3rd;
            sub_a_bf(g_bf_save_y_min, g_bf_y_3rd);
            copy_bf(g_bf_save_x_3rd, g_bf_x_min);        // sx3rd = xxmin;
            copy_bf(g_bf_save_y_3rd, g_bf_y_max);        // sy3rd = yymax;
        }
        break;
    case 26:            // control-Z - reverse X and Y aXis
        for (int i = 0; i < ixhalf; i++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int j = 0; j < g_logical_screen_y_dots; j++)
            {
                tempdot = getcolor(i, j);
                g_put_color(i, j, getcolor(g_logical_screen_x_dots-1-i, g_logical_screen_y_dots-1-j));
                g_put_color(g_logical_screen_x_dots-1-i, g_logical_screen_y_dots-1-j, tempdot);
            }
        }
        g_save_x_min = g_x_max;
        g_save_y_max = g_y_min;
        g_save_x_max = g_x_min;
        g_save_y_min = g_y_max;
        g_save_x_3rd = g_x_max + g_x_min - g_x_3rd;
        g_save_y_3rd = g_y_max + g_y_min - g_y_3rd;
        if (bf_math != bf_math_type::NONE)
        {
            copy_bf(g_bf_save_x_min, g_bf_x_max);        // sxmin = xxmax;
            copy_bf(g_bf_save_y_max, g_bf_y_min);        // symax = yymin;
            copy_bf(g_bf_save_x_max, g_bf_x_min);        // sxmax = xxmin;
            copy_bf(g_bf_save_y_min, g_bf_y_max);        // symin = yymax;
            add_bf(g_bf_save_x_3rd, g_bf_x_max, g_bf_x_min); // sx3rd = xxmax + xxmin - xx3rd;
            sub_a_bf(g_bf_save_x_3rd, g_bf_x_3rd);
            add_bf(g_bf_save_y_3rd, g_bf_y_max, g_bf_y_min); // sy3rd = yymax + yymin - yy3rd;
            sub_a_bf(g_bf_save_y_3rd, g_bf_y_3rd);
        }
        break;
    }
    reset_zoom_corners();
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

static char const *expand_var(char const *var, char *buf)
{
    std::time_t ltime;
    char *str;
    char const *out;

    std::time(&ltime);
    str = std::ctime(&ltime);

    // ctime format
    // Sat Aug 17 21:34:14 1996
    // 012345678901234567890123
    //           1         2
    if (std::strcmp(var, "year") == 0)       // 4 chars
    {
        str[24] = '\0';
        out = &str[20];
    }
    else if (std::strcmp(var, "month") == 0) // 3 chars
    {
        str[7] = '\0';
        out = &str[4];
    }
    else if (std::strcmp(var, "day") == 0)   // 2 chars
    {
        str[10] = '\0';
        out = &str[8];
    }
    else if (std::strcmp(var, "hour") == 0)  // 2 chars
    {
        str[13] = '\0';
        out = &str[11];
    }
    else if (std::strcmp(var, "min") == 0)   // 2 chars
    {
        str[16] = '\0';
        out = &str[14];
    }
    else if (std::strcmp(var, "sec") == 0)   // 2 chars
    {
        str[19] = '\0';
        out = &str[17];
    }
    else if (std::strcmp(var, "time") == 0)  // 8 chars
    {
        str[19] = '\0';
        out = &str[11];
    }
    else if (std::strcmp(var, "date") == 0)
    {
        str[10] = '\0';
        str[24] = '\0';
        char *dest = &str[4];
        std::strcat(dest, ", ");
        std::strcat(dest, &str[20]);
        out = dest;
    }
    else if (std::strcmp(var, "calctime") == 0)
    {
        get_calculation_time(buf, g_calc_time);
        out = buf;
    }
    else if (std::strcmp(var, "version") == 0)  // 4 chars
    {
        sprintf(buf, "%d", g_release);
        out = buf;
    }
    else if (std::strcmp(var, "patch") == 0)   // 1 or 2 chars
    {
        sprintf(buf, "%d", g_patch_level);
        out = buf;
    }
    else if (std::strcmp(var, "xdots") == 0)   // 2 to 4 chars
    {
        sprintf(buf, "%d", g_logical_screen_x_dots);
        out = buf;
    }
    else if (std::strcmp(var, "ydots") == 0)   // 2 to 4 chars
    {
        sprintf(buf, "%d", g_logical_screen_y_dots);
        out = buf;
    }
    else if (std::strcmp(var, "vidkey") == 0)   // 2 to 3 chars
    {
        char vidmde[5];
        vidmode_keyname(g_video_entry.keynum, vidmde);
        sprintf(buf, "%s", vidmde);
        out = buf;
    }
    else
    {
        char buff[80];
        _snprintf(buff, NUM_OF(buff), "Unknown comment variable %s", var);
        stopmsg(STOPMSG_NONE, buff);
        out = "";
    }
    return out;
}

#define MAXVNAME  13

static char const esc_char = '$';

// extract comments from the comments= command
std::string expand_comments(char const *source)
{
    int escape = 0;
    char c, oldc, varname[MAXVNAME];
    int k = 0;
    int i = 0;
    oldc = 0;
    std::string target;
    while (i < MAX_COMMENT_LEN && (c = *(source + i++)) != '\0')
    {
        if (c == '\\' && oldc != '\\')
        {
            oldc = c;
            continue;
        }
        // expand underscores to blanks
        if (c == '_' && oldc != '\\')
        {
            c = ' ';
        }
        // esc_char marks start and end of variable names
        if (c == esc_char && oldc != '\\')
        {
            escape = 1 - escape;
        }
        if (c != esc_char && escape != 0) // if true, building variable name
        {
            if (k < MAXVNAME-1)
            {
                varname[k++] = c;
            }
        }
        // got variable name
        else if (c == esc_char && escape == 0 && oldc != '\\')
        {
            char buf[100];
            varname[k] = 0;
            target += expand_var(varname, buf);
        }
        else if (c == esc_char && escape != 0 && oldc != '\\')
        {
            k = 0;
        }
        else if ((c != esc_char || oldc == '\\') && escape == 0)
        {
            target += c;
        }
        oldc = c;
    }
    return target;
}

// extract comments from the comments= command
void parse_comments(char *value)
{
    char *next, save;
    for (auto &elem : par_comment)
    {
        save = '\0';
        if (*value == 0)
        {
            break;
        }
        next = std::strchr(value, '/');
        if (*value != '/')
        {
            if (next != nullptr)
            {
                save = *next;
                *next = '\0';
            }
            std::strncpy(elem, value, MAX_COMMENT_LEN);
        }
        if (next == nullptr)
        {
            break;
        }
        if (save != '\0')
        {
            *next = save;
        }
        value = next+1;
    }
}

void init_comments()
{
    for (auto &elem : par_comment)
    {
        elem[0] = '\0';
    }
}
