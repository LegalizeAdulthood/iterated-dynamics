// SPDX-License-Identifier: GPL-3.0-only
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
#include "save_file.h"
#include "slideshw.h"
#include "sticky_orbits.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "trig_fns.h"
#include "update_save_name.h"
#include "version.h"
#include "video.h"
#include "video_mode.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>

static bool compress(int row_limit);
static int shift_write(Byte const *color, int num_colors);
static int extend_blk_len(int data_len);
static int put_extend_blk(int block_id, int block_len, char const *block_data);
static int store_item_name(char const *name);
static void setup_save_info(FractalInfo *save_info);

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
static std::FILE *s_outfile{};
static int s_last_color_bar{};
static bool s_save_16bit{};
static int s_out_color_1s{};
static int s_out_color_2s{};
static int s_start_bits{};

static Byte s_palette_bw[] =
{
    // B&W palette
    0, 0, 0, 63, 63, 63,
};

#ifndef XFRACT
static Byte s_palette_cga[] =
{
    // 4-color (CGA) palette
    0, 0, 0, 21, 63, 63, 63, 21, 63, 63, 63, 63,
};
#endif

static Byte s_palette_ega[] =
{
    // 16-color (EGA/CGA) pal
    0, 0, 0, 0, 0, 42, 0, 42, 0, 0, 42, 42,
    42, 0, 0, 42, 0, 42, 42, 21, 0, 42, 42, 42,
    21, 21, 21, 21, 21, 63, 21, 63, 21, 21, 63, 63,
    63, 21, 21, 63, 21, 63, 63, 63, 21, 63, 63, 63,
};

int save_image(std::string &filename)      // save-to-disk routine
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
            stop_msg(std::string{"Can't write "} + open_file.string());
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

    s_outfile = open_save_file(tmp_file.string(), "wb");
    if (s_outfile == nullptr)
    {
        stop_msg(std::string{"Can't create "} + tmp_file.string());
        return -1;
    }

    if (driver_is_disk())
    {
        // disk-video
        dvid_status(1, "Saving " + extract_file_name(open_file.string().c_str()));
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

    if (g_debug_flag != DebugFlags::BENCHMARK_ENCODER)
    {
        interrupted = encoder() ? 1 : 0;
    }
    else
    {
        interrupted = encoder_timer();     // invoke encoder() via timer
    }

    g_busy = false;

    std::fclose(s_outfile);

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
        if (stop_msg(StopMsgFlags::CANCEL, buf))
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

    if (!driver_is_disk())
    {
        // supress this on disk-video
        int out_color1 = s_out_color_1s;
        int out_color2 = s_out_color_2s;
        for (int j = 0; j <= s_last_color_bar; j++)
        {
            if ((j & 4) == 0)
            {
                if (++out_color1 >= g_colors)
                {
                    out_color1 = 0;
                }
                if (++out_color2 >= g_colors)
                {
                    out_color2 = 0;
                }
            }
            for (int i = 0; 250*i < g_logical_screen_x_dots; i++)
            {
                // clear vert status bars
                g_put_color(i, j, get_color(i, j) ^ out_color1);
                g_put_color(g_logical_screen_x_dots - 1 - i, j,
                         get_color(g_logical_screen_x_dots - 1 - i, j) ^ out_color2);
            }
        }
    }
    else                           // disk-video
    {
        dvid_status(1, "");
    }

    if (interrupted)
    {
        text_temp_msg(" *interrupted* save ");
        if (g_init_batch >= BatchMode::NORMAL)
        {
            g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;         // if batch mode, set error level
        }
        return -1;
    }
    if (g_timed_save == 0)
    {
        driver_buzzer(Buzzer::COMPLETE);
        if (g_init_batch == BatchMode::NONE)
        {
            text_temp_msg((" File saved as " + extract_file_name(open_file.string().c_str()) + ' ').c_str());
        }
    }
    if (g_init_save_time < 0)
    {
        goodbye();
    }
    return 0;
}

bool encoder()
{
    bool interrupted;
    int width;
    int row_limit;
    Byte bits_per_pixel;
    Byte x;
    FractalInfo save_info;

    if (g_init_batch != BatchMode::NONE)                 // flush any impending keystrokes
    {
        while (driver_key_pressed())
        {
            driver_get_key();
        }
    }

    setup_save_info(&save_info);

#ifndef XFRACT
    bits_per_pixel = 0;            // calculate bits / pixel
    for (int i = g_colors; i >= 2; i /= 2)
    {
        bits_per_pixel++;
    }

    s_start_bits = bits_per_pixel + 1;// start coding with this many bits
    if (g_colors == 2)
    {
        s_start_bits++;    // B&W Klooge
    }
#else
    if (g_colors == 2)
    {
        bits_per_pixel = 1;
        s_start_bits = 3;
    }
    else
    {
        bits_per_pixel = 8;
        s_start_bits = 9;
    }
#endif

    int i = 0;
    if (std::fputs("GIF89a", s_outfile) < 0)
    {
        goto oops; // new GIF Signature
    }

    width = g_logical_screen_x_dots;
    row_limit = g_logical_screen_y_dots;
    if (s_save_16bit)
    {
        // pot16bit info is stored as: file:    double width rows, right side
        // of row is low 8 bits diskvid: ydots rows of colors followed by ydots
        // rows of low 8 bits decoder: returns (row of color info then row of
        // low 8 bits) * ydots
        row_limit <<= 1;
        width <<= 1;
    }
    if (std::fwrite(&width, 2, 1, s_outfile) != 1)
    {
        goto oops;                // screen descriptor
    }
    if (std::fwrite(&g_logical_screen_y_dots, 2, 1, s_outfile) != 1)
    {
        goto oops;
    }
    x = (Byte)(128 + ((6 - 1) << 4) + (bits_per_pixel - 1));      // color resolution == 6 bits worth
    if (std::fwrite(&x, 1, 1, s_outfile) != 1)
    {
        goto oops;
    }
    if (std::fputc(0, s_outfile) != 0)
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
    i = std::max(i, 1);
    i = std::min(i, 255);
    if (std::fputc(i, s_outfile) != i)
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
            if (!shift_write((Byte *) g_dac_box, g_colors))
            {
                goto oops;
            }
#else
    if (g_colors > 2)
    {
        if (g_got_real_dac || g_fake_lut)
        {
            // got a DAC - must be a VGA
            if (!shift_write((Byte *) g_dac_box, 256))
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
                if (!shift_write((Byte *)s_palette_ega, 16))
                {
                    goto oops;
                }
            }
        }
    }
    if (g_colors == 2)
    {
        // write out the B&W palette
        if (!shift_write((Byte *)s_palette_bw, g_colors))
        {
            goto oops;
        }
    }
#ifndef XFRACT
    if (g_colors == 4)
    {
        // write out the CGA palette
        if (!shift_write((Byte *)s_palette_cga, g_colors))
        {
            goto oops;
        }
    }
    if (g_colors == 16)
    {
        // Either EGA or VGA
        if (g_got_real_dac)
        {
            if (!shift_write((Byte *) g_dac_box, g_colors))
            {
                goto oops;
            }
        }
        else
        {
            // no DAC - must be an EGA
            if (!shift_write((Byte *)s_palette_ega, g_colors))
            {
                goto oops;
            }
        }
    }
#endif

    if (std::fputs(",", s_outfile) < 0)
    {
        goto oops;                // Image Descriptor
    }
    i = 0;
    if (std::fwrite(&i, 2, 1, s_outfile) != 1)
    {
        goto oops;
    }
    if (std::fwrite(&i, 2, 1, s_outfile) != 1)
    {
        goto oops;
    }
    if (std::fwrite(&width, 2, 1, s_outfile) != 1)
    {
        goto oops;
    }
    if (std::fwrite(&g_logical_screen_y_dots, 2, 1, s_outfile) != 1)
    {
        goto oops;
    }
    if (std::fwrite(&i, 1, 1, s_outfile) != 1)
    {
        goto oops;
    }

    bits_per_pixel = (Byte)(s_start_bits - 1);

    if (std::fwrite(&bits_per_pixel, 1, 1, s_outfile) != 1)
    {
        goto oops;
    }

    interrupted = compress(row_limit);

    if (ferror(s_outfile))
    {
        goto oops;
    }

    if (std::fputc(0, s_outfile) != 0)
    {
        goto oops;
    }

    // store non-standard fractal info
    // loadfile has notes about extension block structure
    if (interrupted)
    {
        save_info.calc_status = static_cast<short>(CalcStatus::PARAMS_CHANGED); // partial save is not resumable
    }
    save_info.tot_extend_len = 0;
    if (!g_resume_data.empty() && save_info.calc_status == static_cast<short>(CalcStatus::RESUMABLE))
    {
        // resume info block, 002
        save_info.tot_extend_len += extend_blk_len(g_resume_len);
        std::copy(g_resume_data.data(), &g_resume_data[g_resume_len], &g_block[0]);
        if (!put_extend_blk(2, g_resume_len, (char *)g_block))
        {
            goto oops;
        }
    }
    // save_info.fractal_type gets modified in setup_save_info() in float only version, so we need to use fractype.
    //    if (save_info.fractal_type == FORMULA || save_info.fractal_type == FFORMULA)
    if (g_fractal_type == FractalType::FORMULA || g_fractal_type == FractalType::FORMULA_FP)
    {
        save_info.tot_extend_len += store_item_name(g_formula_name.c_str());
    }
    //    if (save_info.fractal_type == LSYSTEM)
    if (g_fractal_type == FractalType::L_SYSTEM)
    {
        save_info.tot_extend_len += store_item_name(g_l_system_name.c_str());
    }
    //    if (save_info.fractal_type == IFS || save_info.fractal_type == IFS3D)
    if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
    {
        save_info.tot_extend_len += store_item_name(g_ifs_name.c_str());
    }
    if (g_display_3d <= Display3DMode::NONE && g_iteration_ranges_len)
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
        if (!put_extend_blk(4, num_bytes, buffer.data()))
        {
            goto oops;
        }
    }
    // Extended parameters block 005
    if (g_bf_math != BFMathType::NONE)
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
    if (bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
    {
        EvolutionInfo evolution_info;
        if (!g_have_evolve_info || g_calc_status == CalcStatus::COMPLETED)
        {
            evolution_info.x_parameter_range = g_evolve_x_parameter_range;
            evolution_info.y_parameter_range = g_evolve_y_parameter_range;
            evolution_info.x_parameter_offset = g_evolve_x_parameter_offset;
            evolution_info.y_parameter_offset = g_evolve_y_parameter_offset;
            evolution_info.discrete_x_parameter_offset =
                static_cast<std::int16_t>(g_evolve_discrete_x_parameter_offset);
            evolution_info.discrete_y_parameter_offset = static_cast<std::int16_t>(g_evolve_discrete_y_parameter_offset);
            evolution_info.px              = static_cast<std::int16_t>(g_evolve_param_grid_x);
            evolution_info.py              = static_cast<std::int16_t>(g_evolve_param_grid_y);
            evolution_info.screen_x_offset          = static_cast<std::int16_t>(g_logical_screen_x_offset);
            evolution_info.screen_y_offset          = static_cast<std::int16_t>(g_logical_screen_y_offset);
            evolution_info.x_dots           = static_cast<std::int16_t>(g_logical_screen_x_dots);
            evolution_info.y_dots           = static_cast<std::int16_t>(g_logical_screen_y_dots);
            evolution_info.image_grid_size = static_cast<std::int16_t>(g_evolve_image_grid_size);
            evolution_info.evolving        = static_cast<std::int16_t>(+g_evolving);
            evolution_info.this_generation_random_seed =
                static_cast<std::uint16_t>(g_evolve_this_generation_random_seed);
            evolution_info.max_random_mutation = g_evolve_max_random_mutation;
            evolution_info.count          =
                static_cast<std::int16_t>(g_evolve_image_grid_size * g_evolve_image_grid_size); // flag for done
        }
        else
        {
            // we will need the resuming information
            evolution_info.x_parameter_range = g_evolve_info.x_parameter_range;
            evolution_info.y_parameter_range = g_evolve_info.y_parameter_range;
            evolution_info.x_parameter_offset = g_evolve_info.x_parameter_offset;
            evolution_info.y_parameter_offset = g_evolve_info.y_parameter_offset;
            evolution_info.discrete_x_parameter_offset = g_evolve_info.discrete_x_parameter_offset;
            evolution_info.discrete_y_parameter_offset = g_evolve_info.discrete_y_parameter_offset;
            evolution_info.px = g_evolve_info.px;
            evolution_info.py = g_evolve_info.py;
            evolution_info.screen_x_offset = g_evolve_info.screen_x_offset;
            evolution_info.screen_y_offset = g_evolve_info.screen_y_offset;
            evolution_info.x_dots = g_evolve_info.x_dots;
            evolution_info.y_dots = g_evolve_info.y_dots;
            evolution_info.image_grid_size = g_evolve_info.image_grid_size;
            evolution_info.evolving = g_evolve_info.evolving;
            evolution_info.this_generation_random_seed = g_evolve_info.this_generation_random_seed;
            evolution_info.max_random_mutation = g_evolve_info.max_random_mutation;
            evolution_info.count = g_evolve_info.count;
        }
        for (int j = 0; j < NUM_GENES; j++)
        {
            evolution_info.mutate[j] = static_cast<std::int16_t>(g_gene_bank[j].mutate);
        }

        for (int j = 0; j < sizeof(evolution_info.future) / sizeof(std::int16_t); j++)  // NOLINT(modernize-loop-convert)
        {
            evolution_info.future[j] = 0;
        }

        decode_evolver_info(&evolution_info, 0);
        // evolution info block, 006
        save_info.tot_extend_len += extend_blk_len(sizeof(evolution_info));
        if (!put_extend_blk(6, sizeof(evolution_info), (char *) &evolution_info))
        {
            goto oops;
        }
    }

    // Extended parameters block 007
    if (g_std_calc_mode == 'o')
    {
        OrbitsInfo orbits_info{};
        orbits_info.orbit_corner_min_x     = g_orbit_corner_min_x;
        orbits_info.orbit_corner_max_x     = g_orbit_corner_max_x;
        orbits_info.orbit_corner_min_y     = g_orbit_corner_min_y;
        orbits_info.orbit_corner_max_y     = g_orbit_corner_max_y;
        orbits_info.orbit_corner_3rd_x     = g_orbit_corner_3rd_x;
        orbits_info.orbit_corner_3rd_y     = g_orbit_corner_3rd_y;
        orbits_info.keep_screen_coords = static_cast<std::int16_t>(g_keep_screen_coords);
        orbits_info.draw_mode  = g_draw_mode;

        // some big-endian logic for the doubles needed here
        decode_orbits_info(&orbits_info, 0);
        // orbits info block, 007
        save_info.tot_extend_len += extend_blk_len(sizeof(orbits_info));
        if (!put_extend_blk(7, sizeof(orbits_info), (char *) &orbits_info))
        {
            goto oops;
        }
    }

    // main and last block, 001
    save_info.tot_extend_len += extend_blk_len(sizeof(FractalInfo));
    decode_fractal_info(&save_info, 0);
    if (!put_extend_blk(1, sizeof(FractalInfo), (char *) &save_info))
    {
        goto oops;
    }

    if (std::fputs(";", s_outfile) < 0)
    {
        goto oops;                // GIF Terminator
    }

    return interrupted;

oops:
    {
        fflush(s_outfile);
        stop_msg("Error Writing to disk (Disk full?)");
        return true;
    }
}

// TODO: should we be doing this?  We need to store full colors, not the VGA truncated business.
// shift IBM colors to GIF
static int shift_write(Byte const *color, int num_colors)
{
    for (int i = 0; i < num_colors; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            Byte this_color = color[3 * i + j];
            this_color = (Byte)(this_color << 2);
            this_color = (Byte)(this_color + (Byte)(this_color >> 6));
            if (std::fputc(this_color, s_outfile) != (int) this_color)
            {
                return 0;
            }
        }
    }
    return 1;
}

static int extend_blk_len(int data_len)
{
    return data_len + (data_len + 254) / 255 + 15;
    // data   +     1.per.block   + 14 for id + 1 for null at end
}

static int put_extend_blk(int block_id, int block_len, char const *block_data)
{
    int j;
    char header[15];
    std::strcpy(header, "!\377\013fractint");
    std::sprintf(&header[11], "%03d", block_id);
    if (std::fwrite(header, 14, 1, s_outfile) != 1)
    {
        return 0;
    }
    int i = (block_len + 254) / 255;
    while (--i >= 0)
    {
        block_len -= (j = std::min(block_len, 255));
        if (std::fputc(j, s_outfile) != j)
        {
            return 0;
        }
        while (--j >= 0)
        {
            std::fputc(*(block_data++), s_outfile);
        }
    }
    if (std::fputc(0, s_outfile) != 0)
    {
        return 0;
    }
    return 1;
}

static int store_item_name(char const *name)
{
    FormulaInfo formula_info{};
    std::strncpy(formula_info.form_name, name, std::size(formula_info.form_name));
    if (g_fractal_type == FractalType::FORMULA || g_fractal_type == FractalType::FORMULA_FP)
    {
        formula_info.uses_p1 = static_cast<std::int16_t>(g_frm_uses_p1);
        formula_info.uses_p2 = static_cast<std::int16_t>(g_frm_uses_p2);
        formula_info.uses_p3 = static_cast<std::int16_t>(g_frm_uses_p3);
        formula_info.uses_ismand = static_cast<std::int16_t>(g_frm_uses_ismand);
        formula_info.ismand = static_cast<std::int16_t>(g_is_mandelbrot);
        formula_info.uses_p4 = static_cast<std::int16_t>(g_frm_uses_p4);
        formula_info.uses_p5 = static_cast<std::int16_t>(g_frm_uses_p5);
    }
    else
    {
        formula_info.uses_p1 = 0;
        formula_info.uses_p2 = 0;
        formula_info.uses_p3 = 0;
        formula_info.uses_ismand = 0;
        formula_info.ismand = 0;
        formula_info.uses_p4 = 0;
        formula_info.uses_p5 = 0;
    }

    // formula/lsys/ifs info block, 003
    put_extend_blk(3, sizeof(formula_info), (char *) &formula_info);
    return extend_blk_len(sizeof(formula_info));
}

static void setup_save_info(FractalInfo *save_info)
{
    if (g_fractal_type != FractalType::FORMULA && g_fractal_type != FractalType::FORMULA_FP)
    {
        g_max_function = 0;
    }
    // set save parameters in save structure
    std::strcpy(save_info->info_id, INFO_ID);
    save_info->version = FRACTAL_INFO_VERSION;
    save_info->iterations_old =
        static_cast<std::int16_t>(g_max_iterations <= SHRT_MAX ? g_max_iterations : SHRT_MAX);
    save_info->fractal_type = static_cast<std::int16_t>(+g_fractal_type);
    save_info->x_min = g_x_min;
    save_info->x_max = g_x_max;
    save_info->y_min = g_y_min;
    save_info->y_max = g_y_max;
    save_info->c_real = g_params[0];
    save_info->c_imag = g_params[1];
    save_info->ax = 0;
    save_info->bx = 0;
    save_info->cx = 0;
    save_info->dx = 0;
    constexpr std::int16_t LEGACY_DOT_MODE{19};
    save_info->dot_mode = LEGACY_DOT_MODE;
    save_info->x_dots = static_cast<std::int16_t>(g_video_entry.x_dots);
    save_info->y_dots = static_cast<std::int16_t>(g_video_entry.y_dots);
    save_info->colors = static_cast<std::int16_t>(g_video_entry.colors);
    save_info->param3 = 0;        // pre version==7 fields
    save_info->param4 = 0;
    save_info->d_param3 = g_params[2];
    save_info->d_param4 = g_params[3];
    save_info->d_param5 = g_params[4];
    save_info->d_param6 = g_params[5];
    save_info->d_param7 = g_params[6];
    save_info->d_param8 = g_params[7];
    save_info->d_param9 = g_params[8];
    save_info->d_param10 = g_params[9];
    save_info->fill_color = static_cast<std::int16_t>(g_fill_color);
    save_info->potential[0] = static_cast<float>(g_potential_params[0]);
    save_info->potential[1] = static_cast<float>(g_potential_params[1]);
    save_info->potential[2] = static_cast<float>(g_potential_params[2]);
    save_info->random_seed_flag = static_cast<std::int16_t>(g_random_seed_flag ? 1 : 0);
    save_info->random_seed = static_cast<std::int16_t>(g_random_seed);
    save_info->inside = static_cast<std::int16_t>(g_inside_color);
    save_info->log_map_old = static_cast<std::int16_t>(g_log_map_flag <= SHRT_MAX ? g_log_map_flag : SHRT_MAX);
    save_info->invert[0] = static_cast<float>(g_inversion[0]);
    save_info->invert[1] = static_cast<float>(g_inversion[1]);
    save_info->invert[2] = static_cast<float>(g_inversion[2]);
    save_info->decomp[0] = static_cast<std::int16_t>(g_decomp[0]);
    save_info->biomorph = static_cast<std::int16_t>(g_user_biomorph_value);
    save_info->symmetry = static_cast<std::int16_t>(g_force_symmetry);
    save_info->init3d[0] = static_cast<std::int16_t>(g_sphere ? 1 : 0);  // sphere? 1 = yes, 0 = no
    save_info->init3d[1] = static_cast<std::int16_t>(g_x_rot);            // rotate x-axis 60 degrees
    save_info->init3d[2] = static_cast<std::int16_t>(g_y_rot);            // rotate y-axis 90 degrees
    save_info->init3d[3] = static_cast<std::int16_t>(g_z_rot);            // rotate x-axis  0 degrees
    save_info->init3d[4] = static_cast<std::int16_t>(g_x_scale);          // scale x-axis, 90 percent
    save_info->init3d[5] = static_cast<std::int16_t>(g_y_scale);          // scale y-axis, 90 percent
    save_info->init3d[1] = static_cast<std::int16_t>(g_sphere_phi_min);   // longitude start, 180
    save_info->init3d[2] = static_cast<std::int16_t>(g_sphere_phi_max);   // longitude end ,   0
    save_info->init3d[3] = static_cast<std::int16_t>(g_sphere_theta_min); // latitude start,-90 degrees
    save_info->init3d[4] = static_cast<std::int16_t>(g_sphere_theta_max); // latitude stop,  90 degrees
    save_info->init3d[5] = static_cast<std::int16_t>(g_sphere_radius);    // should be user input
    save_info->init3d[6] = static_cast<std::int16_t>(g_rough);            // scale z-axis, 30 percent
    save_info->init3d[7] = static_cast<std::int16_t>(g_water_line);       // water level
    save_info->init3d[8] = static_cast<std::int16_t>(g_fill_type);        // fill type
    save_info->init3d[9] = static_cast<std::int16_t>(g_viewer_z);         // perspective view point
    save_info->init3d[10] = static_cast<std::int16_t>(g_shift_x);         // x shift
    save_info->init3d[11] = static_cast<std::int16_t>(g_shift_y);         // y shift
    save_info->init3d[12] = static_cast<std::int16_t>(g_light_x);         // x light vector coordinate
    save_info->init3d[13] = static_cast<std::int16_t>(g_light_y);         // y light vector coordinate
    save_info->init3d[14] = static_cast<std::int16_t>(g_light_z);         // z light vector coordinate
    save_info->init3d[15] = static_cast<std::int16_t>(g_light_avg);       // number of points to average
    save_info->preview_factor = static_cast<std::int16_t>(g_preview_factor);
    save_info->x_trans = static_cast<std::int16_t>(g_adjust_3d_x);
    save_info->y_trans = static_cast<std::int16_t>(g_adjust_3d_y);
    save_info->red_crop_left = static_cast<std::int16_t>(g_red_crop_left);
    save_info->red_crop_right = static_cast<std::int16_t>(g_red_crop_right);
    save_info->blue_crop_left = static_cast<short>(g_blue_crop_left);
    save_info->blue_crop_right = static_cast<std::int16_t>(g_blue_crop_right);
    save_info->red_bright = static_cast<std::int16_t>(g_red_bright);
    save_info->blue_bright = static_cast<std::int16_t>(g_blue_bright);
    save_info->x_adjust = static_cast<std::int16_t>(g_converge_x_adjust);
    save_info->y_adjust = static_cast<std::int16_t>(g_converge_y_adjust);
    save_info->eye_separation = static_cast<std::int16_t>(g_eye_separation);
    save_info->glasses_type = static_cast<std::int16_t>(g_glasses_type);
    save_info->outside = static_cast<std::int16_t>(g_outside_color);
    save_info->x3rd = g_x_3rd;
    save_info->y3rd = g_y_3rd;
    save_info->calc_status = static_cast<std::int16_t>(g_calc_status);
    save_info->std_calc_mode =
        static_cast<char>(g_three_pass && g_std_calc_mode == '3' ? 127 : g_std_calc_mode);
    save_info->dist_est_old =
        static_cast<std::int16_t>(g_distance_estimator <= 32000 ? g_distance_estimator : 32000);
    save_info->float_flag = static_cast<std::int16_t>(g_float_flag ? 1 : 0);
    save_info->bail_out_old = static_cast<std::int16_t>(g_bail_out >= 4 && g_bail_out <= 32000 ? g_bail_out : 0);
    save_info->calc_time = static_cast<std::int32_t>(g_calc_time);
    save_info->trig_index[0] = static_cast<Byte>(g_trig_index[0]);
    save_info->trig_index[1] = static_cast<Byte>(g_trig_index[1]);
    save_info->trig_index[2] = static_cast<Byte>(g_trig_index[2]);
    save_info->trig_index[3] = static_cast<Byte>(g_trig_index[3]);
    save_info->finite_attractor = static_cast<std::int16_t>(g_finite_attractor ? 1 : 0);
    save_info->init_orbit[0] = g_init_orbit.x;
    save_info->init_orbit[1] = g_init_orbit.y;
    save_info->use_init_orbit = static_cast<char>(g_use_init_orbit);
    save_info->periodicity = static_cast<std::int16_t>(g_periodicity_check);
    save_info->pot16bit = static_cast<std::int16_t>(g_disk_16_bit ? 1 : 0);
    save_info->final_aspect_ratio = g_final_aspect_ratio;
    save_info->system = static_cast<std::int16_t>(g_save_system);
    save_info->release = static_cast<std::int16_t>(g_release);
    save_info->display_3d = static_cast<std::int16_t>(g_display_3d);
    save_info->ambient = static_cast<std::int16_t>(g_ambient);
    save_info->randomize = static_cast<std::int16_t>(g_randomize_3d);
    save_info->haze = static_cast<std::int16_t>(g_haze);
    save_info->transparent[0] = static_cast<std::int16_t>(g_transparent_color_3d[0]);
    save_info->transparent[1] = static_cast<std::int16_t>(g_transparent_color_3d[1]);
    save_info->rotate_lo = static_cast<std::int16_t>(g_color_cycle_range_lo);
    save_info->rotate_hi = static_cast<std::int16_t>(g_color_cycle_range_hi);
    save_info->dist_est_width = static_cast<std::int16_t>(g_distance_estimator_width_factor);
    save_info->julibrot_x_max = g_julibrot_x_max;
    save_info->julibrot_x_min = g_julibrot_x_min;
    save_info->julibrot_y_max = g_julibrot_y_max;
    save_info->julibrot_y_min = g_julibrot_y_min;
    save_info->julibrot_z_dots = static_cast<std::int16_t>(g_julibrot_z_dots);
    save_info->julibrot_origin_fp = g_julibrot_origin_fp;
    save_info->julibrot_depth_fp = g_julibrot_depth_fp;
    save_info->julibrot_height_fp = g_julibrot_height_fp;
    save_info->julibrot_width_fp = g_julibrot_width_fp;
    save_info->julibrot_dist_fp = g_julibrot_dist_fp;
    save_info->eyes_fp = g_eyes_fp;
    save_info->orbit_type = static_cast<std::int16_t>(g_new_orbit_type);
    save_info->juli3d_mode = static_cast<short>(g_julibrot_3d_mode);
    save_info->max_fn = static_cast<std::int16_t>(g_max_function);
    save_info->inverse_julia = static_cast<std::int16_t>((+g_major_method << 8) + +g_inverse_julia_minor_method);
    save_info->bail_out = static_cast<std::int32_t>(g_bail_out);
    save_info->bail_out_test = static_cast<std::int16_t>(g_bail_out_test);
    save_info->iterations = static_cast<std::int32_t>(g_max_iterations);
    save_info->g_bf_length = static_cast<std::int16_t>(g_bn_length);
    save_info->bf_math = static_cast<std::int16_t>(g_bf_math);
    save_info->old_demm_colors = static_cast<std::int16_t>(g_old_demm_colors ? 1 : 0);
    save_info->log_map = static_cast<std::int32_t>(g_log_map_flag);
    save_info->dist_est = static_cast<std::int32_t>(g_distance_estimator);
    save_info->d_invert[0] = g_inversion[0];
    save_info->d_invert[1] = g_inversion[1];
    save_info->d_invert[2] = g_inversion[2];
    save_info->log_calc = static_cast<std::int16_t>(g_log_map_fly_calculate);
    save_info->stop_pass = static_cast<std::int16_t>(g_stop_pass);
    save_info->quick_calc = static_cast<std::int16_t>(g_quick_calc ? 1 : 0);
    save_info->close_prox = g_close_proximity;
    save_info->no_bof = static_cast<std::int16_t>(g_bof_match_book_images ? 0 : 1);
    save_info->orbit_interval = static_cast<std::int32_t>(g_orbit_interval);
    save_info->orbit_delay = static_cast<std::int16_t>(g_orbit_delay);
    save_info->math_tol[0] = g_math_tol[0];
    save_info->math_tol[1] = g_math_tol[1];
    // NOTE: can't use std::fill here on gcc due to packed constraints.
    for (int i = 0; i < 7; ++i)  // NOLINT(modernize-loop-convert)
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
    BITS_F = 12,
    H_SIZE = 5003            // 80% occupancy
};

// GIF Image compression - modified 'compress'
//
// Based on: compress.c - File compression ala IEEE Computer, June 1984.
//
// By Authors:  Spencer W. Thomas
//              Jim McKie
//              Steve Davies
//              Ken Turkowski
//              James A. Woods
//              Joe Orost
//

// prototypes

static void output(int code);
static void char_out(int c);
static void flush_char();
static void cl_block();

static int s_n_bits{};                     // number of bits/code
static int s_max_bits{BITS_F};               // user settable max # bits/code
static int s_max_code{};                    // maximum code, given n_bits
static int s_max_max_cde{(int) 1 << BITS_F}; // should NEVER generate this code

constexpr int max_code(int n_bits)
{
    return (1 << n_bits) - 1;
}

Byte g_block[4096]{};

static long s_h_tab[H_SIZE]{};
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

static bool compress(int row_limit)
{
    int color;
    int in_count = 0;
    bool interrupted = false;

    int out_color1 = 0;               // use these colors to show progress
    int out_color2 = 1;               // (this has nothing to do with GIF)

    if (g_colors > 2)
    {
        out_color1 = 2;
        out_color2 = 3;
    }
    if (((++s_num_saves) & 1) == 0)
    {
        // reverse the colors on alt saves
        int i = out_color1;
        out_color1 = out_color2;
        out_color2 = i;
    }
    s_out_color_1s = out_color1;
    s_out_color_2s = out_color2;

    // Set up the necessary values
    s_cur_accum = 0;
    s_cur_bits = 0;
    s_clear_flag = false;
    int ent = 0;
    s_n_bits = s_start_bits;
    s_max_code = max_code(s_n_bits);

    s_clear_code = (1 << (s_start_bits - 1));
    s_eof_code = s_clear_code + 1;
    s_free_ent = s_clear_code + 2;

    s_a_count = 0;
    int h_shift = 0;
    for (long f_code = (long) H_SIZE;  f_code < 65536L; f_code *= 2L)
    {
        h_shift++;
    }
    h_shift = 8 - h_shift;                // set hash code range bound

    std::memset(s_h_tab, 0xff, (unsigned)H_SIZE*sizeof(long));
    int h_size_reg = H_SIZE;

    output((int)s_clear_code);

    for (int row_num = 0; row_num < g_logical_screen_y_dots; row_num++)
    {
        // scan through the dots
        for (int y_dot = row_num; y_dot < row_limit; y_dot += g_logical_screen_y_dots)
        {
            for (int x_dot = 0; x_dot < g_logical_screen_x_dots; x_dot++)
            {
                if (s_save_16bit == 0 || y_dot < g_logical_screen_y_dots)
                {
                    color = get_color(x_dot, y_dot);
                }
                else
                {
                    color = disk_read_pixel(x_dot + g_logical_screen_x_offset, y_dot + g_logical_screen_y_offset);
                }
                if (in_count == 0)
                {
                    in_count = 1;
                    ent = color;
                    continue;
                }
                long f_code = (long)(((long) color << s_max_bits) + ent);
                int i = (((int)color << h_shift) ^ ent);    // xor hashing
                int disp{};
                
                if (s_h_tab[i] == f_code)
                {
                    ent = s_code_tab[i];
                    continue;
                }
                else if ((long)s_h_tab[i] < 0)        // empty slot
                {
                    goto nomatch;
                }
                disp = h_size_reg - i;           // secondary hash (after G. Knott)
                if (i == 0)
                {
                    disp = 1;
                }
probe:
                if ((i -= disp) < 0)
                {
                    i += h_size_reg;
                }

                if (s_h_tab[i] == f_code)
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
                    s_h_tab[i] = f_code;
                }
                else
                {
                    cl_block();
                }
            } // end for xdot
            if (! driver_is_disk()       // supress this on disk-video
                && y_dot == row_num)
            {
                if ((y_dot & 4) == 0)
                {
                    if (++out_color1 >= g_colors)
                    {
                        out_color1 = 0;
                    }
                    if (++out_color2 >= g_colors)
                    {
                        out_color2 = 0;
                    }
                }
                for (int i = 0; 250*i < g_logical_screen_x_dots; i++)
                {
                    // display vert status bars
                    // (this is NOT GIF-related)
                    g_put_color(i, y_dot, get_color(i, y_dot) ^ out_color1);
                    g_put_color(g_logical_screen_x_dots - 1 - i, y_dot,
                             get_color(g_logical_screen_x_dots - 1 - i, y_dot) ^ out_color2);
                }
                s_last_color_bar = y_dot;
            } // end if !driver_diskp()
            int key = driver_key_pressed();
            if (key && (key != 's'))  // keyboard hit - bail out
            {
                interrupted = true;
                row_num = g_logical_screen_y_dots;
                break;
            }
            if (key == 's')
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

        fflush(s_outfile);
    }
}

//
// Clear out the hash table
//
static void cl_block()             // table clear for block compress
{
    std::memset(s_h_tab, 0xff, (unsigned)H_SIZE*sizeof(long));
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
        std::fputc(s_a_count, s_outfile);
        std::fwrite(s_accum, 1, s_a_count, s_outfile);
        s_a_count = 0;
    }
}
