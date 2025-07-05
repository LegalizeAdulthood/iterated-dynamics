// SPDX-License-Identifier: GPL-3.0-only
//
// An LZW decoder for GIF
// Copyright (C) 1987, by Steven A. Bennett
//
// Permission is given by the author to freely redistribute and include
// this code in any program as long as this credit is given where due.
//
// In accordance with the above, I want to credit Steve Wilhite who wrote
// the code which this is heavily inspired by...
//
// GIF and 'Graphics Interchange Format' are trademarks (tm) of
// Compuserve, Incorporated, an H&R Block Company.
//
// Release Notes: This file contains a decoder routine for GIF images
// which is similar, structurally, to the original routine by Steve Wilhite.
// It is, however, somewhat noticeably faster in most cases.
//
// This routine was modified for use here.
//
#include "io/decoder.h"

#include "engine/pixel_limits.h"
#include "io/gifview.h"
#include "io/loadfile.h"
#include "misc/Driver.h"
#include "misc/sized_types.h"
#include "ui/video.h"

#include <config/port.h>

static short get_next_code();

// extern short out_line(pixels, linelen)
//     UBYTE pixels[];
//     short linelen;
//
//   - This function takes a full line of pixels (one byte per pixel) and
// displays them (or does whatever your program wants with them...).  It
// should return zero, or negative if an error or some other event occurs
// which would require aborting the decode process...  Note that the length
// passed will almost always be equal to the line length passed to the
// decoder function, with the sole exception occurring when an ending code
// occurs in an odd place in the GIF file...  In any case, linelen will be
// equal to the number of pixels passed...
//
int (*g_out_line)(Byte *, int){out_line};

// Various error codes used by decoder
// and my own routines...   It's okay
// for you to define whatever you want,
// as long as it's negative...  It will be
// returned intact up the various subroutine
// levels...
//
enum
{
    OUT_OF_MEMORY = -10,
    BAD_CODE_SIZE = -20,
    READ_ERROR = -1,
    WRITE_ERROR = -2,
    OPEN_ERROR = -3,
    CREATE_ERROR = -4,
    MAX_CODES = 4095,
    NOPE = 0,
    YUP = -1
};

static short s_curr_size{};       // The current code size
static short s_num_avail_bytes{}; // # bytes left in block
static short s_num_bits_left{};   // # bits left in current byte
static Byte *s_byte_buff{};       // Current block, reuse shared mem
static Byte *s_ptr_bytes{};       // Pointer to next byte in block
static short s_code_mask[13] =
{
    0,
    0x0001, 0x0003,
    0x0007, 0x000F,
    0x001F, 0x003F,
    0x007F, 0x00FF,
    0x01FF, 0x03FF,
    0x07FF, 0x0FFF
};
static Byte s_suffix[10000]{};

// bad_code_count;
//
// This value incremented each time an out of range code is read by the decoder.
// When this value is non-zero after a decode, your GIF file is probably
// corrupt in some way...
//

static int s_bad_code_count{};
static Byte s_decoder_line[MAX_PIXELS]{};

// The reason we have these separated like this instead of using
// a structure like the original Wilhite code did, is because this
// stuff generally produces significantly faster code when compiled...
// This code is full of similar speedups...  (For a good book on writing
// C for speed or for space optimization, see Efficient C by Tom Plum,
// published by Plum-Hall Associates...)
//
// short decoder(linewidth)
//    short linewidth;              * Pixels per line of image *
//
// - This function decodes an LZW image, according to the method used
// in the GIF spec.  Every *linewidth* "characters" (i.e. pixels) decoded
// will generate a call to out_line(), which is a user specific function
// to display a line of pixels.  The function gets its codes from
// get_next_code() which is responsible for reading blocks of data and
// separating them into the proper size codes.  Finally, get_byte() is
// the global routine to read the next byte from the GIF file.
//
// It is generally a good idea to have linewidth correspond to the actual
// width of a line (as specified in the Image header) to make your own
// code a bit simpler, but it isn't absolutely necessary.
//
// Returns: 0 if successful, else negative.  (See ERRS.H)
//
//

// moved sizeofstring here for possible re-use elsewhere
static short s_sizeof_string[MAX_CODES + 1]{};  // size of string list

short decoder(short line_width)
{
    U16 prefix[MAX_CODES+1]{};     // Prefix linked list
    short ret;
    short c;

    // Initialize for decoding a new image...

    short size = (short) get_byte();
    if (size < 0)
    {
        return size;
    }
    if (size < 2 || 9 < size)
    {
        return BAD_CODE_SIZE;
    }

    s_curr_size = (short)(size + 1);
    short top_slot = (short) (1 << s_curr_size); // Highest code for current size
    short clear = (short) (1 << size);           // Value for a clear code
    short ending = (short) (clear + 1);          // Value for an ending code
    short new_codes = (short) (ending + 1);       // First available code
    short slot = new_codes;                       // Last read code
    short old_code = 0;
    short y_skip = 0;
    short x_skip = 0;
    s_sizeof_string[slot] = 0;
    s_num_bits_left = 0;
    s_num_avail_bytes = 0;
    Byte out_value = 0;
    for (short i = 0; i < slot; i++)
    {
        s_sizeof_string[i] = 0;
    }

    // Initialize in case they forgot to put in a clear code. (This shouldn't
    // happen, but we'll try and decode it anyway...)

    // Set up the stack pointer and decode buffer pointer
    Byte decode_stack[4096]{};
    Byte *sp = decode_stack;
    Byte *buf_ptr = s_decoder_line;
    short buf_cnt = line_width; // how many empty spaces left in buffer

    // This is the main loop.  For each code we get we pass through the linked
    // list of prefix codes, pushing the corresponding "character" for each
    // code onto the stack.  When the list reaches a single "character" we
    // push that on the stack too, and then start unstacking each character
    // for output in the correct order.  Special handling is included for the
    // clear code, and the whole thing ends when we get an ending code.
    while ((c = get_next_code()) != ending)
    {

        // If we had a file error, return without completing the decode
        if (c < 0)
        {
            return 0;
        }

        // If the code is a clear code, reinitialize all necessary items.
        if (c == clear)
        {
            s_curr_size = (short)(size + 1);
            slot = new_codes;
            s_sizeof_string[slot] = 0;
            top_slot = (short)(1 << s_curr_size);

            // Continue reading codes until we get a non-clear code (Another
            // unlikely, but possible case...)
            do
            {
                c = get_next_code();
            }
            while (c == clear);

            // If we get an ending code immediately after a clear code (Yet
            // another unlikely case), then break out of the loop.
            if (c == ending)
            {
                break;
            }

            // Finally, if the code is beyond the range of already set codes,
            // (This one had better NOT happen...   I have no idea what will
            // result from this, but I doubt it will look good...) then set it
            // to color zero.
            if (c >= slot)
            {
                c = 0;
            }

            old_code = c;
            out_value = (Byte) old_code;

            // And let us not forget to put the char into the buffer...
            *sp++ = (Byte) c;
        }
        else
        {
            // In this case, it's not a clear code or an ending code, so it must
            // be a code code...  So we can now decode the code into a stack of
            // character codes. (Clear as mud, right?)
            short code = c;

            // Here we go again with one of those off chances...  If, on the off
            // chance, the code we got is beyond the range of those already set
            // up (Another thing which had better NOT happen...) we trick the
            // decoder into thinking it actually got the next slot avail.

            if (code >= slot)
            {
                if (code > slot)
                {
                    ++s_bad_code_count;
                    c = slot;
                }
                code = old_code;
                *sp++ = out_value;
            }

            // Here we scan back along the linked list of prefixes.  If they can
            // fit into the output buffer than transfer them direct.  ELSE push
            // them into the stack until we are down to enough characters that
            // they do fit.  Output the line then fall through to unstack the
            // ones that would not fit.
            short fast_loop = NOPE;
            while (code >= new_codes)
            {
                int i = s_sizeof_string[code];
                short j = i;
                if (i > 0 && buf_cnt - i > 0 && g_skip_x_dots == 0)
                {
                    fast_loop = YUP;

                    do
                    {
                        *(buf_ptr + j) = s_suffix[code];
                        code = prefix[code];
                    }
                    while (--j > 0);
                    *buf_ptr = (Byte) code;
                    buf_ptr += ++i;
                    buf_cnt -= i;
                    if (buf_cnt == 0) // finished an input row?
                    {
                        if (--y_skip < 0)
                        {
                            ret = (short) g_out_line(s_decoder_line, (int) (buf_ptr - s_decoder_line));
                            if (ret < 0)
                            {
                                return ret;
                            }
                            y_skip = g_skip_y_dots;
                        }
                        if (driver_key_pressed())
                        {
                            return -1;
                        }
                        buf_ptr = s_decoder_line;
                        buf_cnt = line_width;
                        x_skip = 0;
                    }
                }
                else
                {
                    *sp++ = s_suffix[code];
                    code = prefix[code];
                }
            }

            // Push the last character on the stack, and set up the new prefix
            // and suffix, and if the required slot number is greater than that
            // allowed by the current bit size, increase the bit size.  (NOTE -
            // If we are all full, we *don't* save the new suffix and prefix...
            // I'm not certain if this is correct... it might be more proper to
            // overwrite the last code...)
            if (fast_loop == NOPE)
            {
                *sp++ = (Byte) code;
            }

            if (slot < top_slot)
            {
                s_sizeof_string[slot] = (short)(s_sizeof_string[old_code] + 1);
                out_value = (Byte) code;
                s_suffix[slot] = out_value;
                prefix[slot++] = old_code;
                old_code = c;
            }
            if (slot >= top_slot)
            {
                if (s_curr_size < 12)
                {
                    top_slot <<= 1;
                    ++s_curr_size;
                }
            }
        }
        while (sp > decode_stack)
        {
            --sp;
            if (--x_skip < 0)
            {
                x_skip = g_skip_x_dots;
                *buf_ptr++ = *sp;
            }
            if (--buf_cnt == 0)     // finished an input row?
            {
                if (--y_skip < 0)
                {
                    ret = (short) g_out_line(s_decoder_line, (int) (buf_ptr - s_decoder_line));
                    if (ret < 0)
                    {
                        return ret;
                    }
                    y_skip = g_skip_y_dots;
                }
                if (driver_key_pressed())
                {
                    return -1;
                }
                buf_ptr = s_decoder_line;
                buf_cnt = line_width;
                x_skip = 0;
            }
        }
    }
    return 0;
}

// get_next_code()
// - gets the next code from the GIF file.  Returns the code, or else
// a negative number in case of file errors...
//
static short get_next_code()
{
    static Byte b1;              // Current byte
    static unsigned short ret_code;

    if (s_num_bits_left == 0)
    {
        if (s_num_avail_bytes <= 0)
        {

            // Out of bytes in current block, so read next block
            s_ptr_bytes = s_byte_buff;
            s_num_avail_bytes = (short) get_byte();
            if (s_num_avail_bytes < 0)
            {
                return s_num_avail_bytes;
            }
            if (s_num_avail_bytes)
            {
                get_bytes(s_byte_buff, s_num_avail_bytes);
            }
        }
        b1 = *s_ptr_bytes++;
        s_num_bits_left = 8;
        --s_num_avail_bytes;
    }

    ret_code = (short)(b1 >> (8 - s_num_bits_left));
    while (s_curr_size > s_num_bits_left)
    {
        if (s_num_avail_bytes <= 0)
        {

            // Out of bytes in current block, so read next block
            s_ptr_bytes = s_byte_buff;
            s_num_avail_bytes = (short) get_byte();
            if (s_num_avail_bytes < 0)
            {
                return s_num_avail_bytes;
            }
            if (s_num_avail_bytes)
            {
                get_bytes(s_byte_buff, s_num_avail_bytes);
            }
        }
        b1 = *s_ptr_bytes++;
        ret_code |= b1 << s_num_bits_left;
        s_num_bits_left += 8;
        --s_num_avail_bytes;
    }
    s_num_bits_left -= s_curr_size;
    return (short)(ret_code & s_code_mask[s_curr_size]);
}

// called in parent routine to set byte_buff
void set_byte_buff(Byte * ptr)
{
    s_byte_buff = ptr;
}
