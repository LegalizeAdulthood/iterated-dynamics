#include "make_mig.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "id.h"
#include "rotate.h"
#include "save_file.h"

#include <array>
#include <cstring>
#include <cstdio>

inline char par_key(unsigned int x)
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

void make_mig(unsigned int xmult, unsigned int ymult)
{
    unsigned int xres;
    unsigned int yres;
    unsigned int allxres;
    unsigned int allyres;
    unsigned int xtot;
    unsigned int ytot;
    unsigned int xloc;
    unsigned int yloc;
    unsigned char ichar;
    unsigned int allitbl;
    unsigned int itbl;
    unsigned int i;
    char gifin[15];
    char gifout[15];
    int errorflag;
    int inputerrorflag;
    unsigned char *temp;
    std::FILE *out;
    std::FILE *in;

    errorflag = 0;                          // no errors so
    inputerrorflag = 0;
    allitbl = 0;
    allyres = allitbl;
    allxres = allyres;
    in = nullptr;
    out = in;

    std::strcpy(gifout, "fractmig.gif");

    temp = &g_old_dac_box[0][0];                 // a safe place for our temp data

    // process each input image, one at a time
    for (unsigned ystep = 0U; ystep < ymult; ystep++)
    {
        for (unsigned xstep = 0U; xstep < xmult; xstep++)
        {
            if (xstep == 0 && ystep == 0)          // first time through?
            {
                std::printf(" \n Generating multi-image GIF file %s using", gifout);
                std::printf(" %u X and %u Y components\n\n", xmult, ymult);
                // attempt to create the output file
                out = open_save_file(gifout, "wb");
                if (out == nullptr)
                {
                    std::printf("Cannot create output file %s!\n", gifout);
                    exit(1);
                }
            }

            std::snprintf(gifin, std::size(gifin), "frmig_%c%c.gif", par_key(xstep), par_key(ystep));

            in = std::fopen(gifin, "rb");
            if (in == nullptr)
            {
                std::printf("Can't open file %s!\n", gifin);
                exit(1);
            }

            // (read, but only copy this if it's the first time through)
            if (std::fread(temp, 13, 1, in) != 1)   // read the header and LDS
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
                temp[12] = 0; // reserved
                if (std::fwrite(temp, 13, 1, out) != 1)     // write out the header
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
                if (std::fread(temp, 3*itbl, 1, in) != 1)    // read the global color table
                {
                    inputerrorflag = 2;
                }
                if (xstep == 0 && ystep == 0)       // first time through?
                {
                    if (std::fwrite(temp, 3*itbl, 1, out) != 1)     // write out the GCT
                    {
                        errorflag = 2;
                    }
                }
            }

            if (xres != allxres || yres != allyres || itbl != allitbl)
            {
                // Oops - our pieces don't match
                std::printf("File %s doesn't have the same resolution as its predecessors!\n", gifin);
                exit(1);
            }

            while (true)                       // process each information block
            {
                std::memset(temp, 0, 10);
                if (std::fread(temp, 1, 1, in) != 1)    // read the block identifier
                {
                    inputerrorflag = 3;
                }

                if (temp[0] == 0x2c)           // image descriptor block
                {
                    if (std::fread(&temp[1], 9, 1, in) != 1)    // read the Image Descriptor
                    {
                        inputerrorflag = 4;
                    }
                    std::memcpy(&xloc, &temp[1], 2); // X-location
                    std::memcpy(&yloc, &temp[3], 2); // Y-location
                    xloc += (xstep * xres);     // adjust the locations
                    yloc += (ystep * yres);
                    std::memcpy(&temp[1], &xloc, 2);
                    std::memcpy(&temp[3], &yloc, 2);
                    if (std::fwrite(temp, 10, 1, out) != 1)     // write out the Image Descriptor
                    {
                        errorflag = 4;
                    }

                    ichar = (char)(temp[9] & 0x80);     // is there a local color table?
                    if (ichar != 0)            // yup
                    {
                        if (std::fread(temp, 3*itbl, 1, in) != 1)       // read the local color table
                        {
                            inputerrorflag = 5;
                        }
                        if (std::fwrite(temp, 3*itbl, 1, out) != 1)     // write out the LCT
                        {
                            errorflag = 5;
                        }
                    }

                    if (std::fread(temp, 1, 1, in) != 1)        // LZH table size
                    {
                        inputerrorflag = 6;
                    }
                    if (std::fwrite(temp, 1, 1, out) != 1)
                    {
                        errorflag = 6;
                    }
                    while (true)
                    {
                        if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            inputerrorflag = 7;
                        }
                        if (std::fwrite(temp, 1, 1, out) != 1)
                        {
                            errorflag = 7;
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // LZH data block
                        {
                            inputerrorflag = 8;
                        }
                        if (std::fwrite(temp, i, 1, out) != 1)
                        {
                            errorflag = 8;
                        }
                    }
                }

                if (temp[0] == 0x21)           // extension block
                {
                    // (read, but only copy this if it's the last time through)
                    if (std::fread(&temp[2], 1, 1, in) != 1)    // read the block type
                    {
                        inputerrorflag = 9;
                    }
                    if (xstep == xmult-1 && ystep == ymult-1)
                    {
                        if (std::fwrite(temp, 2, 1, out) != 1)
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
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            inputerrorflag = 10;
                        }
                        if (xstep == xmult-1 && ystep == ymult-1)
                        {
                            if (std::fwrite(temp, 1, 1, out) != 1)
                            {
                                errorflag = 10;
                            }
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // data block
                        {
                            inputerrorflag = 11;
                        }
                        if (xstep == xmult-1 && ystep == ymult-1)
                        {
                            if (std::fwrite(temp, i, 1, out) != 1)
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
            std::fclose(in);                     // done with an input GIF

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
    if (std::fwrite(temp, 1, 1, out) != 1)
    {
        errorflag = 12;
    }
    std::fclose(out);                    // done with the output GIF

    if (inputerrorflag != 0)       // uh-oh - something failed
    {
        std::printf("\007 Process failed = early EOF on input file %s\n", gifin);
        /* following line was for debugging
            std::printf("inputerrorflag = %d\n", inputerrorflag);
        */
    }

    if (errorflag != 0)            // uh-oh - something failed
    {
        std::printf("\007 Process failed = out of disk space?\n");
        /* following line was for debugging
            std::printf("errorflag = %d\n", errorflag);
        */
    }

    // now delete each input image, one at a time
    if (errorflag == 0 && inputerrorflag == 0)
    {
        for (unsigned ystep = 0U; ystep < ymult; ystep++)
        {
            for (unsigned xstep = 0U; xstep < xmult; xstep++)
            {
                std::snprintf(gifin, std::size(gifin), "frmig_%c%c.gif", par_key(xstep), par_key(ystep));
                std::remove(gifin);
            }
        }
    }

    // tell the world we're done
    if (errorflag == 0 && inputerrorflag == 0)
    {
        std::printf("File %s has been created (and its component files deleted)\n", gifout);
    }
}
