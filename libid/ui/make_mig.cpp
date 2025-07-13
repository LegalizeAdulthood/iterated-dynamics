// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/make_mig.h"

#include "io/save_file.h"
#include "ui/rotate.h"

#include <fmt/format.h>

#include <array> // std::size
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

static char par_key(unsigned int x)
{
    return x < 10 ? '0' + x : 'a' - 10 + x;
}

/* make_mig() takes a collection of individual GIF images (all
   presumably the same resolution and all presumably generated
   by this program and its "divide and conquer" algorithm) and builds
   a single multiple-image GIF out of them.  This routine is
   invoked by the "batch=stitchmode/x/y" option, and is called
   with the 'x' and 'y' parameters
*/

void make_mig(unsigned int x_mult, unsigned int y_mult)
{
    unsigned int x_res;
    unsigned int y_res;
    unsigned int x_tot;
    unsigned int y_tot;
    unsigned int x_loc;
    unsigned int y_loc;
    unsigned int i;
    std::string gif_in;
    unsigned char *temp;

    int error_flag = 0;                          // no errors so
    int input_error_flag = 0;
    unsigned int all_i_tbl = 0;
    unsigned int all_y_res = all_i_tbl;
    unsigned int all_x_res = all_y_res;
    std::FILE *in = nullptr;
    std::FILE *out = in;

    std::string gif_out{"fractmig.gif"};

    temp = &g_old_dac_box[0][0];                 // a safe place for our temp data

    // process each input image, one at a time
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            if (x_step == 0 && y_step == 0)          // first time through?
            {
                std::printf(" \n"
                            " Generating multi-image GIF file %s using %u X and %u Y components\n"
                            "\n",
                    gif_out.c_str(), x_mult, y_mult);
                // attempt to create the output file
                out = open_save_file(gif_out, "wb");
                if (out == nullptr)
                {
                    std::printf("Cannot create output file %s!\n", gif_out.c_str());
                    std::exit(1);
                }
            }

            gif_in = fmt::format("frmig_{:c}{:c}.gif", par_key(x_step), par_key(y_step));

            in = std::fopen(gif_in.c_str(), "rb");
            if (in == nullptr)
            {
                std::printf("Can't open file %s!\n", gif_in.c_str());
                std::exit(1);
            }

            // (read, but only copy this if it's the first time through)
            if (std::fread(temp, 13, 1, in) != 1)   // read the header and LDS
            {
                input_error_flag = 1;
            }
            std::memcpy(&x_res, &temp[6], 2);     // X-resolution
            std::memcpy(&y_res, &temp[8], 2);     // Y-resolution

            if (x_step == 0 && y_step == 0)  // first time through?
            {
                all_x_res = x_res;             // save the "master" resolution
                all_y_res = y_res;
                x_tot = x_res * x_mult;        // adjust the image size
                y_tot = y_res * y_mult;
                std::memcpy(&temp[6], &x_tot, 2);
                std::memcpy(&temp[8], &y_tot, 2);
                temp[12] = 0; // reserved
                if (std::fwrite(temp, 13, 1, out) != 1)     // write out the header
                {
                    error_flag = 1;
                }
            }                           // end of first-time-through

            unsigned char i_char = (char) (temp[10] & 0x07);        // find the color table size
            unsigned int i_tbl = 1 << (++i_char);
            i_char = (char)(temp[10] & 0x80);        // is there a global color table?
            if (x_step == 0 && y_step == 0)   // first time through?
            {
                all_i_tbl = i_tbl;             // save the color table size
            }
            if (i_char != 0)                // yup
            {
                // (read, but only copy this if it's the first time through)
                if (std::fread(temp, 3*i_tbl, 1, in) != 1)    // read the global color table
                {
                    input_error_flag = 2;
                }
                if (x_step == 0 && y_step == 0)       // first time through?
                {
                    if (std::fwrite(temp, 3*i_tbl, 1, out) != 1)     // write out the GCT
                    {
                        error_flag = 2;
                    }
                }
            }

            if (x_res != all_x_res || y_res != all_y_res || i_tbl != all_i_tbl)
            {
                // Oops - our pieces don't match
                std::printf("File %s doesn't have the same resolution as its predecessors!\n", gif_in.c_str());
                std::exit(1);
            }

            while (true)                       // process each information block
            {
                std::memset(temp, 0, 10);
                if (std::fread(temp, 1, 1, in) != 1)    // read the block identifier
                {
                    input_error_flag = 3;
                }

                if (temp[0] == 0x2c)           // image descriptor block
                {
                    if (std::fread(&temp[1], 9, 1, in) != 1)    // read the Image Descriptor
                    {
                        input_error_flag = 4;
                    }
                    std::memcpy(&x_loc, &temp[1], 2); // X-location
                    std::memcpy(&y_loc, &temp[3], 2); // Y-location
                    x_loc += (x_step * x_res);     // adjust the locations
                    y_loc += (y_step * y_res);
                    std::memcpy(&temp[1], &x_loc, 2);
                    std::memcpy(&temp[3], &y_loc, 2);
                    if (std::fwrite(temp, 10, 1, out) != 1)     // write out the Image Descriptor
                    {
                        error_flag = 4;
                    }

                    i_char = (char)(temp[9] & 0x80);     // is there a local color table?
                    if (i_char != 0)            // yup
                    {
                        if (std::fread(temp, 3*i_tbl, 1, in) != 1)       // read the local color table
                        {
                            input_error_flag = 5;
                        }
                        if (std::fwrite(temp, 3*i_tbl, 1, out) != 1)     // write out the LCT
                        {
                            error_flag = 5;
                        }
                    }

                    if (std::fread(temp, 1, 1, in) != 1)        // LZH table size
                    {
                        input_error_flag = 6;
                    }
                    if (std::fwrite(temp, 1, 1, out) != 1)
                    {
                        error_flag = 6;
                    }
                    while (true)
                    {
                        if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            input_error_flag = 7;
                        }
                        if (std::fwrite(temp, 1, 1, out) != 1)
                        {
                            error_flag = 7;
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // LZH data block
                        {
                            input_error_flag = 8;
                        }
                        if (std::fwrite(temp, i, 1, out) != 1)
                        {
                            error_flag = 8;
                        }
                    }
                }

                if (temp[0] == 0x21)           // extension block
                {
                    // (read, but only copy this if it's the last time through)
                    if (std::fread(&temp[2], 1, 1, in) != 1)    // read the block type
                    {
                        input_error_flag = 9;
                    }
                    if (x_step == x_mult-1 && y_step == y_mult-1)
                    {
                        if (std::fwrite(temp, 2, 1, out) != 1)
                        {
                            error_flag = 9;
                        }
                    }
                    while (true)
                    {
                        if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            input_error_flag = 10;
                        }
                        if (x_step == x_mult-1 && y_step == y_mult-1)
                        {
                            if (std::fwrite(temp, 1, 1, out) != 1)
                            {
                                error_flag = 10;
                            }
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // data block
                        {
                            input_error_flag = 11;
                        }
                        if (x_step == x_mult-1 && y_step == y_mult-1)
                        {
                            if (std::fwrite(temp, i, 1, out) != 1)
                            {
                                error_flag = 11;
                            }
                        }
                    }
                }

                if (temp[0] == 0x3b)           // end-of-stream indicator
                {
                    break;                      // done with this file
                }

                if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
                {
                    break;
                }
            }
            std::fclose(in);                     // done with an input GIF

            if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
            {
                break;
            }
        }

        if (error_flag != 0 || input_error_flag != 0)  // oops - did something go wrong?
        {
            break;
        }
    }

    temp[0] = 0x3b;                 // end-of-stream indicator
    if (std::fwrite(temp, 1, 1, out) != 1)
    {
        error_flag = 12;
    }
    std::fclose(out);                    // done with the output GIF

    if (input_error_flag != 0)       // uh-oh - something failed
    {
        std::printf("\007 Process failed = early EOF on input file %s\n", gif_in.c_str());
    }

    if (error_flag != 0)            // uh-oh - something failed
    {
        std::printf("\007 Process failed = out of disk space?\n");
    }

    // now delete each input image, one at a time
    if (error_flag == 0 && input_error_flag == 0)
    {
        for (unsigned y_step = 0U; y_step < y_mult; y_step++)
        {
            for (unsigned x_step = 0U; x_step < x_mult; x_step++)
            {
                gif_in = fmt::format("frmig_{:c}{:c}.gif", par_key(x_step), par_key(y_step));
                std::filesystem::remove(gif_in);
            }
        }
    }

    // tell the world we're done
    if (error_flag == 0 && input_error_flag == 0)
    {
        std::printf("File %s has been created (and its component files deleted)\n", gif_out.c_str());
    }
}
