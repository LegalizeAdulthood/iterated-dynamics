//
//      GIF Encoder and associated routines
//
#include "encoder.h"

#include "3d.h"
#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "decode_info.h"
#include "diskvid.h"
#include "drivers.h"
#include "engine_timer.h"
#include "evolve.h"
#include "extract_filename.h"
#include "fractype.h"
#include "framain2.h"
#include "get_color.h"
#include "goodbye.h"
#include "id.h"
#include "id_data.h"
#include "jb.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#if defined(XFRACT)
#include "os.h"
#endif
#include "parser.h"
#include "plot3d.h"
#include "resume.h"
#include "rotate.h"
#include "slideshw.h"
#include "sticky_orbits.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "trig_fns.h"
#include "update_save_name.h"
#include "version.h"
#include "video_mode.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>

static bool compress(int rowlimit);
static int shftwrite(BYTE const *color, int numcolors);
static int extend_blk_len(int datalen);
static int put_extend_blk(int block_id, int block_len, char const *block_data);
static int store_item_name(char const *name);
static void setup_save_info(FRACTAL_INFO *save_info);

//                        Save-To-Disk Routines (GIF)
//
// The following routines perform the GIF encoding when the 's' key is pressed.
//
// The compression logic in this file has been replaced by the classic
// UNIX compress code. We have extensively modified the sources to fit
// our needs, but have left the original credits where they
// appear. Thanks to the original authors for making available these
// classic and reliable sources. Of course, they are not responsible for
// all the changes we have made to integrate their sources into this program.
//
// MEMORY ALLOCATION
//
// There are two large arrays:
//
//    long htab[HSIZE]              (5003*4 = 20012 bytes)
//    unsigned short codetab[HSIZE] (5003*2 = 10006 bytes)
//
// At the moment these arrays reuse extraseg and strlocn, respectively.
//
//

static int s_num_saves{}; // For adjusting 'save-to-disk' filenames
static std::FILE *g_outfile{};
static int s_last_color_bar{};
static bool s_save_16bit{};
static int s_out_color_1s{};
static int s_out_color_2s{};
static int s_start_bits{};

static BYTE s_palette_bw[] =
{
    // B&W palette
    0, 0, 0, 63, 63, 63,
};

#ifndef XFRACT
static BYTE s_palette_cga[] =
{
    // 4-color (CGA) palette
    0, 0, 0, 21, 63, 63, 63, 21, 63, 63, 63, 63,
};
#endif

static BYTE s_palette_ega[] =
{
    // 16-color (EGA/CGA) pal
    0, 0, 0, 0, 0, 42, 0, 42, 0, 0, 42, 42,
    42, 0, 0, 42, 0, 42, 42, 21, 0, 42, 42, 42,
    21, 21, 21, 21, 21, 63, 21, 63, 21, 21, 63, 63,
    63, 21, 21, 63, 21, 63, 63, 63, 21, 63, 63, 63,
};

static int gif_savetodisk(char *filename)      // save-to-disk routine
{
    std::filesystem::path open_file;
    std::string open_file_ext;
    std::filesystem::path tmp_file;
    bool new_file;
    int interrupted;

restart:
    s_save_16bit = g_disk_16_bit;
    open_file = filename;                  // decode and open the filename
    open_file_ext = DEFAULT_FRACTAL_TYPE; // determine the file extension
    if (s_save_16bit)
    {
        open_file_ext = ".pot";
    }

    if (open_file.has_extension())
    {
        open_file_ext = open_file.extension().string();
        open_file.replace_extension();
    }
    if (g_resave_flag != 1)
    {
        update_save_name(filename); // for next time
    }

    open_file.replace_extension(open_file_ext);

    tmp_file = open_file;
    if (!exists(open_file))  // file doesn't exist
    {
        new_file = true;
    }
    else
    {
        // file already exists
        if (!g_overwrite_file)
        {
            if (g_resave_flag == 0)
            {
                goto restart;
            }
            if (!g_started_resaves)
            {
                // first save of a savetime set
                update_save_name(filename);
                goto restart;
            }
        }
        if ((status(open_file).permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
        {
            stopmsg(std::string{"Can't write "} + open_file.string());
            return -1;
        }
        new_file = false;
        tmp_file.replace_filename("id.tmp");
    }

    g_started_resaves = (g_resave_flag == 1);
    if (g_resave_flag == 2)          // final save of savetime set?
    {
        g_resave_flag = 0;
    }

    g_outfile = std::fopen(tmp_file.string().c_str(), "wb");
    if (g_outfile == nullptr)
    {
        stopmsg(std::string{"Can't create "} + tmp_file.string());
        return -1;
    }

    if (driver_diskp())
    {
        // disk-video
        dvid_status(1, "Saving " + extract_filename(open_file.string().c_str()));
    }
#ifdef XFRACT
    else
    {
        driver_put_string(3, 0, 0, "Saving to:");
        driver_put_string(4, 0, 0, open_file.string());
        driver_put_string(5, 0, 0, "               ");
    }
#endif

    g_busy = true;

    if (g_debug_flag != debug_flags::benchmark_encoder)
    {
        interrupted = encoder() ? 1 : 0;
    }
    else
    {
        interrupted = timer(timer_type::ENCODER, nullptr);     // invoke encoder() via timer
    }

    g_busy = false;

    std::fclose(g_outfile);

    if (interrupted)
    {
        std::string buf{"Save of "};
        buf += open_file.string();
        buf += " interrupted.\nCancel to ";
        if (new_file)
        {
            buf += "delete the file,\ncontinue to keep the partial image.";
        }
        else
        {
            buf += "retain the original file,\ncontinue to replace original with new partial image.";
        }
        interrupted = 1;
        if (stopmsg(stopmsg_flags::CANCEL, buf))
        {
            interrupted = -1;
            std::filesystem::remove(tmp_file);
        }
    }

    if (!new_file && interrupted >= 0)
    {
        // replace the real file
        std::filesystem::remove(open_file);          // success assumed since we checked
        std::filesystem::rename(tmp_file, open_file); // earlier with access
    }

    if (!driver_diskp())
    {
        // supress this on disk-video
        int outcolor1 = s_out_color_1s;
        int outcolor2 = s_out_color_2s;
        for (int j = 0; j <= s_last_color_bar; j++)
        {
            if ((j & 4) == 0)
            {
                if (++outcolor1 >= g_colors)
                {
                    outcolor1 = 0;
                }
                if (++outcolor2 >= g_colors)
                {
                    outcolor2 = 0;
                }
            }
            for (int i = 0; 250*i < g_logical_screen_x_dots; i++)
            {
                // clear vert status bars
                g_put_color(i, j, getcolor(i, j) ^ outcolor1);
                g_put_color(g_logical_screen_x_dots - 1 - i, j,
                         getcolor(g_logical_screen_x_dots - 1 - i, j) ^ outcolor2);
            }
        }
    }
    else                           // disk-video
    {
        dvid_status(1, "");
    }

    if (interrupted)
    {
        texttempmsg(" *interrupted* save ");
        if (g_init_batch >= batch_modes::NORMAL)
        {
            g_init_batch = batch_modes::BAILOUT_ERROR_NO_SAVE;         // if batch mode, set error level
        }
        return -1;
    }
    if (g_timed_save == 0)
    {
        driver_buzzer(buzzer_codes::COMPLETE);
        if (g_init_batch == batch_modes::NONE)
        {
            texttempmsg((" File saved as " + extract_filename(open_file.string().c_str()) + ' ').c_str());
        }
    }
    if (g_init_save_time < 0)
    {
        goodbye();
    }
    return 0;
}

int savetodisk(std::string &filename)
{
    char buff[FILE_MAX_PATH];
    std::strncpy(buff, filename.c_str(), FILE_MAX_PATH);
    int const result = gif_savetodisk(buff);
    filename = buff;
    return result;
}

bool encoder()
{
    bool interrupted;
    int width;
    int rowlimit;
    BYTE bitsperpixel;
    BYTE x;
    FRACTAL_INFO save_info;

    if (g_init_batch != batch_modes::NONE)                 // flush any impending keystrokes
    {
        while (driver_key_pressed())
        {
            driver_get_key();
        }
    }

    setup_save_info(&save_info);

#ifndef XFRACT
    bitsperpixel = 0;            // calculate bits / pixel
    for (int i = g_colors; i >= 2; i /= 2)
    {
        bitsperpixel++;
    }

    s_start_bits = bitsperpixel + 1;// start coding with this many bits
    if (g_colors == 2)
    {
        s_start_bits++;    // B&W Klooge
    }
#else
    if (g_colors == 2)
    {
        bitsperpixel = 1;
        s_start_bits = 3;
    }
    else
    {
        bitsperpixel = 8;
        s_start_bits = 9;
    }
#endif

    int i = 0;
    if (std::fwrite("GIF89a", 6, 1, g_outfile) != 1)
    {
        goto oops; // new GIF Signature
    }

    width = g_logical_screen_x_dots;
    rowlimit = g_logical_screen_y_dots;
    if (s_save_16bit)
    {
        // pot16bit info is stored as: file:    double width rows, right side
        // of row is low 8 bits diskvid: ydots rows of colors followed by ydots
        // rows of low 8 bits decoder: returns (row of color info then row of
        // low 8 bits) * ydots
        rowlimit <<= 1;
        width <<= 1;
    }
    if (write2(&width, 2, 1, g_outfile) != 1)
    {
        goto oops;                // screen descriptor
    }
    if (write2(&g_logical_screen_y_dots, 2, 1, g_outfile) != 1)
    {
        goto oops;
    }
    x = (BYTE)(128 + ((6 - 1) << 4) + (bitsperpixel - 1));      // color resolution == 6 bits worth
    if (write1(&x, 1, 1, g_outfile) != 1)
    {
        goto oops;
    }
    if (std::fputc(0, g_outfile) != 0)
    {
        goto oops;                // background color
    }

    // TODO: pixel aspect ratio should be 1:1?
    if (g_view_window                               // less than full screen?
        && (g_view_x_dots == 0 || g_view_y_dots == 0))     // and we picked the dots?
    {
        i = (int)(((double) g_screen_y_dots / (double) g_screen_x_dots) * 64.0 / g_screen_aspect - 14.5);
    }
    else       // must risk loss of precision if numbers low
    {
        i = (int)((((double) g_logical_screen_y_dots / (double) g_logical_screen_x_dots) / g_final_aspect_ratio) * 64 - 14.5);
    }
    if (i < 1)
    {
        i = 1;
    }
    if (i > 255)
    {
        i = 255;
    }
    if (std::fputc(i, g_outfile) != i)
    {
        goto oops;                // pixel aspect ratio
    }

#ifndef XFRACT
    if (g_colors == 256)
    {
        // write out the 256-color palette
        if (g_got_real_dac)
        {
            // got a DAC - must be a VGA
            if (!shftwrite((BYTE *) g_dac_box, g_colors))
            {
                goto oops;
            }
#else
    if (g_colors > 2)
    {
        if (g_got_real_dac || g_fake_lut)
        {
            // got a DAC - must be a VGA
            if (!shftwrite((BYTE *) g_dac_box, 256))
            {
                goto oops;
            }
#endif
        }
        else
        {
            // uh oh - better fake it
            for (int j = 0; j < 256; j += 16)
            {
                if (!shftwrite((BYTE *)s_palette_ega, 16))
                {
                    goto oops;
                }
            }
        }
    }
    if (g_colors == 2)
    {
        // write out the B&W palette
        if (!shftwrite((BYTE *)s_palette_bw, g_colors))
        {
            goto oops;
        }
    }
#ifndef XFRACT
    if (g_colors == 4)
    {
        // write out the CGA palette
        if (!shftwrite((BYTE *)s_palette_cga, g_colors))
        {
            goto oops;
        }
    }
    if (g_colors == 16)
    {
        // Either EGA or VGA
        if (g_got_real_dac)
        {
            if (!shftwrite((BYTE *) g_dac_box, g_colors))
            {
                goto oops;
            }
        }
        else
        {
            // no DAC - must be an EGA
            if (!shftwrite((BYTE *)s_palette_ega, g_colors))
            {
                goto oops;
            }
        }
    }
#endif

    if (std::fwrite(",", 1, 1, g_outfile) != 1)
    {
        goto oops;                // Image Descriptor
    }
    i = 0;
    if (write2(&i, 2, 1, g_outfile) != 1)
    {
        goto oops;
    }
    if (write2(&i, 2, 1, g_outfile) != 1)
    {
        goto oops;
    }
    if (write2(&width, 2, 1, g_outfile) != 1)
    {
        goto oops;
    }
    if (write2(&g_logical_screen_y_dots, 2, 1, g_outfile) != 1)
    {
        goto oops;
    }
    if (write1(&i, 1, 1, g_outfile) != 1)
    {
        goto oops;
    }

    bitsperpixel = (BYTE)(s_start_bits - 1);

    if (write1(&bitsperpixel, 1, 1, g_outfile) != 1)
    {
        goto oops;
    }

    interrupted = compress(rowlimit);

    if (ferror(g_outfile))
    {
        goto oops;
    }

    if (std::fputc(0, g_outfile) != 0)
    {
        goto oops;
    }

    // store non-standard fractal info
    // loadfile has notes about extension block structure
    if (interrupted)
    {
        save_info.calc_status = static_cast<short>(calc_status_value::PARAMS_CHANGED); // partial save is not resumable
    }
    save_info.tot_extend_len = 0;
    if (!g_resume_data.empty() && save_info.calc_status == static_cast<short>(calc_status_value::RESUMABLE))
    {
        // resume info block, 002
        save_info.tot_extend_len += extend_blk_len(g_resume_len);
        std::copy(&g_resume_data[0], &g_resume_data[g_resume_len], &g_block[0]);
        if (!put_extend_blk(2, g_resume_len, (char *)g_block))
        {
            goto oops;
        }
    }
    // save_info.fractal_type gets modified in setup_save_info() in float only version, so we need to use fractype.
    //    if (save_info.fractal_type == FORMULA || save_info.fractal_type == FFORMULA)
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        save_info.tot_extend_len += store_item_name(g_formula_name.c_str());
    }
    //    if (save_info.fractal_type == LSYSTEM)
    if (g_fractal_type == fractal_type::LSYSTEM)
    {
        save_info.tot_extend_len += store_item_name(g_l_system_name.c_str());
    }
    //    if (save_info.fractal_type == IFS || save_info.fractal_type == IFS3D)
    if (g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D)
    {
        save_info.tot_extend_len += store_item_name(g_ifs_name.c_str());
    }
    if (g_display_3d <= display_3d_modes::NONE && g_iteration_ranges_len)
    {
        // ranges block, 004
        int const num_bytes = g_iteration_ranges_len*2;
        save_info.tot_extend_len += extend_blk_len(num_bytes);
        std::vector<char> buffer;
        for (int range : g_iteration_ranges)
        {
            // ranges are stored as 16-bit ints in little-endian byte order
            buffer.push_back(range & 0xFF);
            buffer.push_back(static_cast<unsigned>(range & 0xFFFF) >> 8);
        }
        if (!put_extend_blk(4, num_bytes, &buffer[0]))
        {
            goto oops;
        }

    }
    // Extended parameters block 005
    if (g_bf_math != bf_math_type::NONE)
    {
        save_info.tot_extend_len += extend_blk_len(22 * (g_bf_length + 2));
        // note: this assumes variables allocated in order starting with
        // g_bf_x_min in init_bf_2() in BIGNUM.C
        if (!put_extend_blk(5, 22 * (g_bf_length + 2), (char *) g_bf_x_min))
        {
            goto oops;
        }
    }

    // Extended parameters block 006
    if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
    {
        EVOLUTION_INFO esave_info;
        if (!g_have_evolve_info || g_calc_status == calc_status_value::COMPLETED)
        {
            esave_info.x_parameter_range = g_evolve_x_parameter_range;
            esave_info.y_parameter_range = g_evolve_y_parameter_range;
            esave_info.x_parameter_offset = g_evolve_x_parameter_offset;
            esave_info.y_parameter_offset = g_evolve_y_parameter_offset;
            esave_info.discrete_x_parameter_offset = (short) g_evolve_discrete_x_parameter_offset;
            esave_info.discrete_y_paramter_offset = (short) g_evolve_discrete_y_parameter_offset;
            esave_info.px              = (short)g_evolve_param_grid_x;
            esave_info.py              = (short)g_evolve_param_grid_y;
            esave_info.sxoffs          = (short)g_logical_screen_x_offset;
            esave_info.syoffs          = (short)g_logical_screen_y_offset;
            esave_info.xdots           = (short)g_logical_screen_x_dots;
            esave_info.ydots           = (short)g_logical_screen_y_dots;
            esave_info.image_grid_size = (short) g_evolve_image_grid_size;
            esave_info.evolving        = (short) +g_evolving;
            esave_info.this_generation_random_seed = (unsigned short) g_evolve_this_generation_random_seed;
            esave_info.max_random_mutation = g_evolve_max_random_mutation;
            esave_info.ecount          = (short)(g_evolve_image_grid_size * g_evolve_image_grid_size); // flag for done
        }
        else
        {
            // we will need the resuming information
            esave_info.x_parameter_range = g_evolve_info.x_parameter_range;
            esave_info.y_parameter_range = g_evolve_info.y_parameter_range;
            esave_info.x_parameter_offset = g_evolve_info.x_parameter_offset;
            esave_info.y_parameter_offset = g_evolve_info.y_parameter_offset;
            esave_info.discrete_x_parameter_offset = (short)g_evolve_info.discrete_x_parameter_offset;
            esave_info.discrete_y_paramter_offset = (short)g_evolve_info.discrete_y_paramter_offset;
            esave_info.px              = (short)g_evolve_info.px;
            esave_info.py              = (short)g_evolve_info.py;
            esave_info.sxoffs          = (short)g_evolve_info.sxoffs;
            esave_info.syoffs          = (short)g_evolve_info.syoffs;
            esave_info.xdots           = (short)g_evolve_info.xdots;
            esave_info.ydots           = (short)g_evolve_info.ydots;
            esave_info.image_grid_size = (short)g_evolve_info.image_grid_size;
            esave_info.evolving        = (short)g_evolve_info.evolving;
            esave_info.this_generation_random_seed = (unsigned short)g_evolve_info.this_generation_random_seed;
            esave_info.max_random_mutation = g_evolve_info.max_random_mutation;
            esave_info.ecount          = g_evolve_info.ecount;
        }
        for (int j = 0; j < NUM_GENES; j++)
        {
            esave_info.mutate[j] = (short)g_gene_bank[j].mutate;
        }

        for (int j = 0; j < sizeof(esave_info.future) / sizeof(short); j++)
        {
            esave_info.future[j] = 0;
        }

        decode_evolver_info(&esave_info, 0);
        // evolution info block, 006
        save_info.tot_extend_len += extend_blk_len(sizeof(esave_info));
        if (!put_extend_blk(6, sizeof(esave_info), (char *) &esave_info))
        {
            goto oops;
        }
    }

    // Extended parameters block 007
    if (g_std_calc_mode == 'o')
    {
        ORBITS_INFO osave_info{};
        osave_info.oxmin     = g_orbit_corner_min_x;
        osave_info.oxmax     = g_orbit_corner_max_x;
        osave_info.oymin     = g_orbit_corner_min_y;
        osave_info.oymax     = g_orbit_corner_max_y;
        osave_info.ox3rd     = g_orbit_corner_3_x;
        osave_info.oy3rd     = g_orbit_corner_3_y;
        osave_info.keep_scrn_coords = (short) (g_keep_screen_coords ? 1 : 0);
        osave_info.drawmode  = g_draw_mode;

        // some big-endian logic for the doubles needed here
        decode_orbits_info(&osave_info, 0);
        // orbits info block, 007
        save_info.tot_extend_len += extend_blk_len(sizeof(osave_info));
        if (!put_extend_blk(7, sizeof(osave_info), (char *) &osave_info))
        {
            goto oops;
        }
    }

    // main and last block, 001
    save_info.tot_extend_len += extend_blk_len(sizeof(FRACTAL_INFO));
    decode_fractal_info(&save_info, 0);
    if (!put_extend_blk(1, sizeof(FRACTAL_INFO), (char *) &save_info))
    {
        goto oops;
    }

    if (std::fwrite(";", 1, 1, g_outfile) != 1)
    {
        goto oops;                // GIF Terminator
    }

    return interrupted;

oops:
    {
        fflush(g_outfile);
        stopmsg("Error Writing to disk (Disk full?)");
        return true;
    }
}

// TODO: should we be doing this?  We need to store full colors, not the VGA truncated business.
// shift IBM colors to GIF
static int shftwrite(BYTE const *color, int numcolors)
{
    for (int i = 0; i < numcolors; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            BYTE thiscolor = color[3 * i + j];
            thiscolor = (BYTE)(thiscolor << 2);
            thiscolor = (BYTE)(thiscolor + (BYTE)(thiscolor >> 6));
            if (std::fputc(thiscolor, g_outfile) != (int) thiscolor)
            {
                return 0;
            }
        }
    }
    return 1;
}

static int extend_blk_len(int datalen)
{
    return datalen + (datalen + 254) / 255 + 15;
    // data   +     1.per.block   + 14 for id + 1 for null at end
}

static int put_extend_blk(int block_id, int block_len, char const *block_data)
{
    int i;
    int j;
    char header[15];
    std::strcpy(header, "!\377\013fractint");
    std::sprintf(&header[11], "%03d", block_id);
    if (std::fwrite(header, 14, 1, g_outfile) != 1)
    {
        return 0;
    }
    i = (block_len + 254) / 255;
    while (--i >= 0)
    {
        block_len -= (j = std::min(block_len, 255));
        if (std::fputc(j, g_outfile) != j)
        {
            return 0;
        }
        while (--j >= 0)
        {
            std::fputc(*(block_data++), g_outfile);
        }
    }
    if (std::fputc(0, g_outfile) != 0)
    {
        return 0;
    }
    return 1;
}

static int store_item_name(char const *name)
{
    formula_info fsave_info{};
    std::strcpy(fsave_info.form_name, name);
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        fsave_info.uses_p1 = (short) (g_frm_uses_p1 ? 1 : 0);
        fsave_info.uses_p2 = (short) (g_frm_uses_p2 ? 1 : 0);
        fsave_info.uses_p3 = (short) (g_frm_uses_p3 ? 1 : 0);
        fsave_info.uses_ismand = (short) (g_frm_uses_ismand ? 1 : 0);
        fsave_info.ismand = (short) (g_is_mandelbrot ? 1 : 0);
        fsave_info.uses_p4 = (short) (g_frm_uses_p4 ? 1 : 0);
        fsave_info.uses_p5 = (short) (g_frm_uses_p5 ? 1 : 0);
    }
    else
    {
        fsave_info.uses_p1 = 0;
        fsave_info.uses_p2 = 0;
        fsave_info.uses_p3 = 0;
        fsave_info.uses_ismand = 0;
        fsave_info.ismand = 0;
        fsave_info.uses_p4 = 0;
        fsave_info.uses_p5 = 0;
    }

    // formula/lsys/ifs info block, 003
    put_extend_blk(3, sizeof(fsave_info), (char *) &fsave_info);
    return extend_blk_len(sizeof(fsave_info));
}

static void setup_save_info(FRACTAL_INFO *save_info)
{
    if (g_fractal_type != fractal_type::FORMULA && g_fractal_type != fractal_type::FFORMULA)
    {
        g_max_function = 0;
    }
    // set save parameters in save structure
    std::strcpy(save_info->info_id, INFO_ID);
    save_info->version = FRACTAL_INFO_VERSION;

    if (g_max_iterations <= SHRT_MAX)
    {
        save_info->iterationsold = (short) g_max_iterations;
    }
    else
    {
        save_info->iterationsold = (short) SHRT_MAX;
    }

    save_info->fractal_type = +g_fractal_type;
    save_info->xmin = g_x_min;
    save_info->xmax = g_x_max;
    save_info->ymin = g_y_min;
    save_info->ymax = g_y_max;
    save_info->creal = g_params[0];
    save_info->cimag = g_params[1];
    save_info->videomodeax = 0;
    save_info->videomodebx = 0;
    save_info->videomodecx = 0;
    save_info->videomodedx = 0;
    save_info->dotmode = (short)(g_dot_mode % 100);
    save_info->xdots = (short) g_video_entry.xdots;
    save_info->ydots = (short) g_video_entry.ydots;
    save_info->colors = (short) g_video_entry.colors;
    save_info->parm3 = 0;        // pre version==7 fields
    save_info->parm4 = 0;
    save_info->dparm3 = g_params[2];
    save_info->dparm4 = g_params[3];
    save_info->dparm5 = g_params[4];
    save_info->dparm6 = g_params[5];
    save_info->dparm7 = g_params[6];
    save_info->dparm8 = g_params[7];
    save_info->dparm9 = g_params[8];
    save_info->dparm10 = g_params[9];
    save_info->fillcolor = (short) g_fill_color;
    save_info->potential[0] = (float) g_potential_params[0];
    save_info->potential[1] = (float) g_potential_params[1];
    save_info->potential[2] = (float) g_potential_params[2];
    save_info->rflag = (short) (g_random_seed_flag ? 1 : 0);
    save_info->rseed = (short) g_random_seed;
    save_info->inside = (short) g_inside_color;
    if (g_log_map_flag <= SHRT_MAX)
    {
        save_info->logmapold = (short) g_log_map_flag;
    }
    else
    {
        save_info->logmapold = (short) SHRT_MAX;
    }
    save_info->invert[0] = (float) g_inversion[0];
    save_info->invert[1] = (float) g_inversion[1];
    save_info->invert[2] = (float) g_inversion[2];
    save_info->decomp[0] = (short) g_decomp[0];
    save_info->biomorph = (short) g_user_biomorph_value;
    save_info->symmetry = (short) g_force_symmetry;
    save_info->init3d[0] = (std::int16_t) g_sphere ? 1 : 0;   // sphere? 1 = yes, 0 = no
    save_info->init3d[1] = (std::int16_t) g_x_rot;            // rotate x-axis 60 degrees
    save_info->init3d[2] = (std::int16_t) g_y_rot;            // rotate y-axis 90 degrees
    save_info->init3d[3] = (std::int16_t) g_z_rot;            // rotate x-axis  0 degrees
    save_info->init3d[4] = (std::int16_t) g_x_scale;          // scale x-axis, 90 percent
    save_info->init3d[5] = (std::int16_t) g_y_scale;          // scale y-axis, 90 percent
    save_info->init3d[1] = (std::int16_t) g_sphere_phi_min;   // longitude start, 180
    save_info->init3d[2] = (std::int16_t) g_sphere_phi_max;   // longitude end ,   0
    save_info->init3d[3] = (std::int16_t) g_sphere_theta_min; // latitude start,-90 degrees
    save_info->init3d[4] = (std::int16_t) g_sphere_theta_max; // latitude stop,  90 degrees
    save_info->init3d[5] = (std::int16_t) g_sphere_radius;    // should be user input
    save_info->init3d[6] = (std::int16_t) g_rough;            // scale z-axis, 30 percent
    save_info->init3d[7] = (std::int16_t) g_water_line;       // water level
    save_info->init3d[8] = (std::int16_t) g_fill_type;        // fill type
    save_info->init3d[9] = (std::int16_t) g_viewer_z;         // perspective view point
    save_info->init3d[10] = (std::int16_t) g_shift_x;         // x shift
    save_info->init3d[11] = (std::int16_t) g_shift_y;         // y shift
    save_info->init3d[12] = (std::int16_t) g_light_x;         // x light vector coordinate
    save_info->init3d[13] = (std::int16_t) g_light_y;         // y light vector coordinate
    save_info->init3d[14] = (std::int16_t) g_light_z;         // z light vector coordinate
    save_info->init3d[15] = (std::int16_t) g_light_avg;       // number of points to average
    save_info->previewfactor = (short) g_preview_factor;
    save_info->xtrans = (short) g_adjust_3d_x;
    save_info->ytrans = (short) g_adjust_3d_y;
    save_info->red_crop_left = (short) g_red_crop_left;
    save_info->red_crop_right = (short) g_red_crop_right;
    save_info->blue_crop_left = (short) g_blue_crop_left;
    save_info->blue_crop_right = (short) g_blue_crop_right;
    save_info->red_bright = (short) g_red_bright;
    save_info->blue_bright = (short) g_blue_bright;
    save_info->xadjust = (short) g_converge_x_adjust;
    save_info->yadjust = (short) g_converge_y_adjust;
    save_info->eyeseparation = (short) g_eye_separation;
    save_info->glassestype = (short) g_glasses_type;
    save_info->outside = (short) g_outside_color;
    save_info->x3rd = g_x_3rd;
    save_info->y3rd = g_y_3rd;
    save_info->calc_status = (short) g_calc_status;
    save_info->stdcalcmode = (char)((g_three_pass && g_std_calc_mode == '3') ? 127 : g_std_calc_mode);
    if (g_distance_estimator <= 32000)
    {
        save_info->distestold = (short) g_distance_estimator;
    }
    else
    {
        save_info->distestold = 32000;
    }
    save_info->floatflag = g_float_flag ? 1 : 0;
    if (g_bail_out >= 4 && g_bail_out <= 32000)
    {
        save_info->bailoutold = (short) g_bail_out;
    }
    else
    {
        save_info->bailoutold = 0;
    }

    save_info->calctime = g_calc_time;
    save_info->trigndx[0] = static_cast<BYTE>(g_trig_index[0]);
    save_info->trigndx[1] = static_cast<BYTE>(g_trig_index[1]);
    save_info->trigndx[2] = static_cast<BYTE>(g_trig_index[2]);
    save_info->trigndx[3] = static_cast<BYTE>(g_trig_index[3]);
    save_info->finattract = (short) (g_finite_attractor ? 1 : 0);
    save_info->initorbit[0] = g_init_orbit.x;
    save_info->initorbit[1] = g_init_orbit.y;
    save_info->useinitorbit = static_cast<char>(g_use_init_orbit);
    save_info->periodicity = (short) g_periodicity_check;
    save_info->pot16bit = (short) (g_disk_16_bit ? 1 : 0);
    save_info->faspectratio = g_final_aspect_ratio;
    save_info->system = (short) g_save_system;
    save_info->release = g_release;
    save_info->display_3d = (short) g_display_3d;
    save_info->ambient = (short) g_ambient;
    save_info->randomize = (short) g_randomize_3d;
    save_info->haze = (short) g_haze;
    save_info->transparent[0] = (short) g_transparent_color_3d[0];
    save_info->transparent[1] = (short) g_transparent_color_3d[1];
    save_info->rotate_lo = (short) g_color_cycle_range_lo;
    save_info->rotate_hi = (short) g_color_cycle_range_hi;
    save_info->distestwidth = (short) g_distance_estimator_width_factor;
    save_info->mxmaxfp = g_julibrot_x_max;
    save_info->mxminfp = g_julibrot_x_min;
    save_info->mymaxfp = g_julibrot_y_max;
    save_info->myminfp = g_julibrot_y_min;
    save_info->zdots = (short) g_julibrot_z_dots;
    save_info->originfp = g_julibrot_origin_fp;
    save_info->depthfp = g_julibrot_depth_fp;
    save_info->heightfp = g_julibrot_height_fp;
    save_info->widthfp = g_julibrot_width_fp;
    save_info->distfp = g_julibrot_dist_fp;
    save_info->eyesfp = g_eyes_fp;
    save_info->orbittype = (short) g_new_orbit_type;
    save_info->juli3Dmode = static_cast<short>(g_julibrot_3d_mode);
    save_info->maxfn = g_max_function;
    save_info->inversejulia = (short)((static_cast<int>(g_major_method) << 8) + static_cast<int>(g_inverse_julia_minor_method));
    save_info->bailout = g_bail_out;
    save_info->bailoutest = (short) g_bail_out_test;
    save_info->iterations = g_max_iterations;
    save_info->g_bf_length = (short) g_bn_length;
    save_info->bf_math = (short) g_bf_math;
    save_info->old_demm_colors = (short) (g_old_demm_colors ? 1 : 0);
    save_info->logmap = g_log_map_flag;
    save_info->distest = g_distance_estimator;
    save_info->dinvert[0] = g_inversion[0];
    save_info->dinvert[1] = g_inversion[1];
    save_info->dinvert[2] = g_inversion[2];
    save_info->logcalc = (short) g_log_map_fly_calculate;
    save_info->stoppass = (short) g_stop_pass;
    save_info->quick_calc = (short) (g_quick_calc ? 1 : 0);
    save_info->closeprox = g_close_proximity;
    save_info->nobof = (short) (g_bof_match_book_images ? 0 : 1);
    save_info->orbit_interval = g_orbit_interval;
    save_info->orbit_delay = (short) g_orbit_delay;
    save_info->math_tol[0] = g_math_tol[0];
    save_info->math_tol[1] = g_math_tol[1];
    for (int i = 0; i < sizeof(save_info->future)/sizeof(short); i++)
    {
        save_info->future[i] = 0;
    }

}

//
//  GIFENCOD.C       - GIF Image compression routines
//
//  Lempel-Ziv compression based on 'compress'.  GIF modifications by
//  David Rowley (mgardi@watdcsu.waterloo.edu).
//  Thoroughly massaged by the Stone Soup team for our purposes.
//
//

enum
{
    BITSF = 12,
    HSIZE = 5003            // 80% occupancy
};

// GIF Image compression - modified 'compress'
//
// Based on: compress.c - File compression ala IEEE Computer, June 1984.
//
// By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
//              Jim McKie               (decvax!mcvax!jim)
//              Steve Davies            (decvax!vax135!petsd!peora!srd)
//              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
//              James A. Woods          (decvax!ihnp4!ames!jaw)
//              Joe Orost               (decvax!vax135!petsd!joe)
//

// prototypes

static void output(int code);
static void char_out(int c);
static void flush_char();
static void cl_block();

static int s_n_bits{};                     // number of bits/code
static int s_max_bits{BITSF};               // user settable max # bits/code
static int s_max_code{};                    // maximum code, given n_bits
static int s_max_max_cde{(int) 1 << BITSF}; // should NEVER generate this code

constexpr int max_code(int n_bits)
{
    return (1 << n_bits) - 1;
}

BYTE g_block[4096]{};

static long s_h_tab[HSIZE]{};
static unsigned short s_code_tab[10240]{};

static int s_free_ent{};                  // first unused entry

// block compression parameters -- after all codes are used up,
// and compression rate changes, start over.
static bool s_clear_flag{};

//
// compress stdin to stdout
//
// Algorithm:  use open addressing double hashing (no chaining) on the
// prefix code / next character combination.  We do a variant of Knuth's
// algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
// secondary probe.  Here, the modular division first probe is gives way
// to a faster exclusive-or manipulation.  Also do block compression with
// an adaptive reset, whereby the code table is cleared when the compression
// ratio decreases, but after the table fills.  The variable-length output
// codes are re-sized at this point, and a special CLEAR code is generated
// for the decompressor.  Late addition:  construct the table according to
// file size for noticeable speed improvement on small files.  Please direct
// questions about this implementation to ames!jaw.
//

static int s_clear_code{};
static int s_eof_code{};
static int s_a_count{}; // Number of characters so far in this 'packet'
static unsigned long s_cur_accum{};
static int s_cur_bits{};

//
// Define the storage for the packet accumulator
//
static char s_accum[256]{};

static bool compress(int rowlimit)
{
    int outcolor1;
    int outcolor2;
    int ent;
    int disp;
    int hsize_reg;
    int hshift;
    int color;
    int in_count = 0;
    bool interrupted = false;
    int tempkey;

    outcolor1 = 0;               // use these colors to show progress
    outcolor2 = 1;               // (this has nothing to do with GIF)

    if (g_colors > 2)
    {
        outcolor1 = 2;
        outcolor2 = 3;
    }
    if (((++s_num_saves) & 1) == 0)
    {
        // reverse the colors on alt saves
        int i = outcolor1;
        outcolor1 = outcolor2;
        outcolor2 = i;
    }
    s_out_color_1s = outcolor1;
    s_out_color_2s = outcolor2;

    // Set up the necessary values
    s_cur_accum = 0;
    s_cur_bits = 0;
    s_clear_flag = false;
    ent = 0;
    s_n_bits = s_start_bits;
    s_max_code = max_code(s_n_bits);

    s_clear_code = (1 << (s_start_bits - 1));
    s_eof_code = s_clear_code + 1;
    s_free_ent = s_clear_code + 2;

    s_a_count = 0;
    hshift = 0;
    for (long fcode = (long) HSIZE;  fcode < 65536L; fcode *= 2L)
    {
        hshift++;
    }
    hshift = 8 - hshift;                // set hash code range bound

    std::memset(s_h_tab, 0xff, (unsigned)HSIZE*sizeof(long));
    hsize_reg = HSIZE;

    output((int)s_clear_code);

    for (int rownum = 0; rownum < g_logical_screen_y_dots; rownum++)
    {
        // scan through the dots
        for (int ydot = rownum; ydot < rowlimit; ydot += g_logical_screen_y_dots)
        {
            for (int xdot = 0; xdot < g_logical_screen_x_dots; xdot++)
            {
                if (s_save_16bit == 0 || ydot < g_logical_screen_y_dots)
                {
                    color = getcolor(xdot, ydot);
                }
                else
                {
                    color = readdisk(xdot + g_logical_screen_x_offset, ydot + g_logical_screen_y_offset);
                }
                if (in_count == 0)
                {
                    in_count = 1;
                    ent = color;
                    continue;
                }
                long fcode = (long)(((long) color << s_max_bits) + ent);
                int i = (((int)color << hshift) ^ ent);    // xor hashing

                if (s_h_tab[i] == fcode)
                {
                    ent = s_code_tab[i];
                    continue;
                }
                else if ((long)s_h_tab[i] < 0)        // empty slot
                {
                    goto nomatch;
                }
                disp = hsize_reg - i;           // secondary hash (after G. Knott)
                if (i == 0)
                {
                    disp = 1;
                }
probe:
                if ((i -= disp) < 0)
                {
                    i += hsize_reg;
                }

                if (s_h_tab[i] == fcode)
                {
                    ent = s_code_tab[i];
                    continue;
                }
                if ((long)s_h_tab[i] > 0)
                {
                    goto probe;
                }
nomatch:
                output((int) ent);
                ent = color;
                if (s_free_ent < s_max_max_cde)
                {
                    // code -> hashtable
                    s_code_tab[i] = (unsigned short)s_free_ent++;
                    s_h_tab[i] = fcode;
                }
                else
                {
                    cl_block();
                }
            } // end for xdot
            if (! driver_diskp()       // supress this on disk-video
                && ydot == rownum)
            {
                if ((ydot & 4) == 0)
                {
                    if (++outcolor1 >= g_colors)
                    {
                        outcolor1 = 0;
                    }
                    if (++outcolor2 >= g_colors)
                    {
                        outcolor2 = 0;
                    }
                }
                for (int i = 0; 250*i < g_logical_screen_x_dots; i++)
                {
                    // display vert status bars
                    // (this is NOT GIF-related)
                    g_put_color(i, ydot, getcolor(i, ydot) ^ outcolor1);
                    g_put_color(g_logical_screen_x_dots - 1 - i, ydot,
                             getcolor(g_logical_screen_x_dots - 1 - i, ydot) ^ outcolor2);
                }
                s_last_color_bar = ydot;
            } // end if !driver_diskp()
            tempkey = driver_key_pressed();
            if (tempkey && (tempkey != 's'))  // keyboard hit - bail out
            {
                interrupted = true;
                rownum = g_logical_screen_y_dots;
                break;
            }
            if (tempkey == 's')
            {
                driver_get_key();   // eat the keystroke
            }
        } // end for ydot
    } // end for rownum

    // Put out the final code.
    output((int)ent);
    output((int) s_eof_code);
    return interrupted;
}

//****************************************************************
// TAG(output)
//
// Output the given code.
// Inputs:
//      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
//              that n_bits =< (long)wordsize - 1.
// Outputs:
//      Outputs code to the file.
// Assumptions:
//      Chars are 8 bits long.
// Algorithm:
//      Maintain a BITSF character long buffer (so that 8 codes will
// fit in it exactly).  Use the VAX insv instruction to insert each
// code in turn.  When the buffer fills up empty it and start over.
//


static void output(int code)
{
    static unsigned long masks[] =
    {
        0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
        0x001F, 0x003F, 0x007F, 0x00FF,
        0x01FF, 0x03FF, 0x07FF, 0x0FFF,
        0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
    };

    s_cur_accum &= masks[ s_cur_bits ];

    if (s_cur_bits > 0)
    {
        s_cur_accum |= ((long)code << s_cur_bits);
    }
    else
    {
        s_cur_accum = code;
    }

    s_cur_bits += s_n_bits;

    while (s_cur_bits >= 8)
    {
        char_out((unsigned int)(s_cur_accum & 0xff));
        s_cur_accum >>= 8;
        s_cur_bits -= 8;
    }

    // If the next entry is going to be too big for the code size,
    // then increase it, if possible.
    if (s_free_ent > s_max_code || s_clear_flag)
    {
        if (s_clear_flag)
        {
            s_n_bits = s_start_bits;
            s_max_code = max_code(s_n_bits);
            s_clear_flag = false;
        }
        else
        {
            s_n_bits++;
            if (s_n_bits == s_max_bits)
            {
                s_max_code = s_max_max_cde;
            }
            else
            {
                s_max_code = max_code(s_n_bits);
            }
        }
    }

    if (code == s_eof_code)
    {
        // At EOF, write the rest of the buffer.
        while (s_cur_bits > 0)
        {
            char_out((unsigned int)(s_cur_accum & 0xff));
            s_cur_accum >>= 8;
            s_cur_bits -= 8;
        }

        flush_char();

        fflush(g_outfile);
    }
}

//
// Clear out the hash table
//
static void cl_block()             // table clear for block compress
{
    std::memset(s_h_tab, 0xff, (unsigned)HSIZE*sizeof(long));
    s_free_ent = s_clear_code + 2;
    s_clear_flag = true;
    output((int)s_clear_code);
}

//
// Add a character to the end of the current packet, and if it is 254
// characters, flush the packet to disk.
//
static void char_out(int c)
{
    s_accum[ s_a_count++ ] = (char)c;
    if (s_a_count >= 254)
    {
        flush_char();
    }
}

//
// Flush the packet to disk, and reset the accumulator
//
static void flush_char()
{
    if (s_a_count > 0)
    {
        std::fputc(s_a_count, g_outfile);
        std::fwrite(s_accum, 1, s_a_count, g_outfile);
        s_a_count = 0;
    }
}
