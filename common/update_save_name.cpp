#include "update_save_name.h"

#include "port.h"

#include "fractint.h"
#include "prompts2.h"

void update_save_name(char *filename) // go to the next file name
{
    char *save, *hold;
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];

    splitpath(filename, drive, dir, fname, ext);

    hold = fname + std::strlen(fname) - 1; // start at the end
    while (hold >= fname && (*hold == ' ' || std::isdigit(*hold)))   // skip backwards
    {
        hold--;
    }
    hold++;                      // recover first digit
    while (*hold == '0')           // skip leading zeros
    {
        hold++;
    }
    save = hold;
    while (*save)
    {
        // check for all nines
        if (*save != '9')
        {
            break;
        }
        save++;
    }
    if (!*save)                    // if the whole thing is nines then back
    {
        save = hold - 1;          // up one place. Note that this will eat
    }
    // your last letter if you go to far.
    else
    {
        save = hold;
    }
    std::snprintf(save, NUM_OF(save), "%ld", atol(hold)+1); // increment the number
    makepath(filename, drive, dir, fname, ext);
}

void update_save_name(std::string &filename)
{
    char buff[FILE_MAX_PATH];
    std::strncpy(buff, filename.c_str(), FILE_MAX_PATH);
    update_save_name(buff);
    filename = buff;
}
