#include "split_path.h"

#include "port.h"

#include "prototyp.h"

#include <algorithm>
#include <cstring>

#ifndef XFRACT
int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext)
{
    int length;
    int len;
    int offset;
    char const *tmp;
    if (drive)
    {
        drive[0] = 0;
    }
    if (dir)
    {
        dir[0]   = 0;
    }
    if (fname)
    {
        fname[0] = 0;
    }
    if (ext)
    {
        ext[0]   = 0;
    }

    length = (int) std::strlen(file_template);
    if (length == 0)
    {
        return 0;
    }

    offset = 0;

    // get drive
    if (length >= 2)
    {
        if (file_template[1] == ':')
        {
            if (drive)
            {
                drive[0] = file_template[offset++];
                drive[1] = file_template[offset++];
                drive[2] = 0;
            }
            else
            {
                offset++;
                offset++;
            }
        }
    }

    // get dir
    if (offset < length)
    {
        tmp = std::strrchr(file_template, SLASHC);
        if (tmp)
        {
            tmp++;  // first character after slash
            len = (int)(tmp - (char *)&file_template[offset]);
            if (len >= 0 && len < FILE_MAX_DIR && dir)
            {
                std::strncpy(dir, &file_template[offset], std::min(len, FILE_MAX_DIR));
            }
            if (len < FILE_MAX_DIR && dir)
            {
                dir[len] = 0;
            }
            offset += len;
        }
    }
    else
    {
        return 0;
    }

    // get fname
    if (offset < length)
    {
        tmp = std::strrchr(file_template, '.');
        if (tmp < std::strrchr(file_template, SLASHC) || tmp < std::strrchr(file_template, ':'))
        {
            tmp = nullptr; // in this case the '.' must be a directory
        }
        if (tmp)
        {
            len = (int)(tmp - (char *)&file_template[offset]);
            if ((len > 0) && (offset+len < length) && fname)
            {
                std::strncpy(fname, &file_template[offset], std::min(len, FILE_MAX_FNAME));
                if (len < FILE_MAX_FNAME)
                {
                    fname[len] = 0;
                }
                else
                {
                    fname[FILE_MAX_FNAME-1] = 0;
                }
            }
            offset += len;
            if ((offset < length) && ext)
            {
                std::strncpy(ext, &file_template[offset], FILE_MAX_EXT);
                ext[FILE_MAX_EXT-1] = 0;
            }
        }
        else if ((offset < length) && fname)
        {
            std::strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
            fname[FILE_MAX_FNAME-1] = 0;
        }
    }
    return 0;
}
#else
/*
 *----------------------------------------------------------------------
 *
 * splitpath --
 *
 *      This is the splitpath code from prompts.c
 *
 * Results:
 *      Returns drive, dir, base, and extension.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext)
{
    int length;
    int len;
    int offset;
    char const *tmp;

    if (drive)
        drive[0] = 0;
    if (dir)
        dir[0]   = 0;
    if (fname)
        fname[0] = 0;
    if (ext)
        ext[0]   = 0;

    length = std::strlen(file_template);
    if (length == 0)
        return 0;
    offset = 0;

    // get drive
    if (length >= 2)
        if (file_template[1] == ':')
        {
            if (drive)
            {
                drive[0] = file_template[offset++];
                drive[1] = file_template[offset++];
                drive[2] = 0;
            }
            else
            {
                offset++;
                offset++;
            }
        }

    // get dir
    if (offset < length)
    {
        tmp = std::strrchr(file_template, SLASHC);
        if (tmp)
        {
            tmp++;  // first character after slash
            len = tmp - &file_template[offset];
            if (len >=0 && len < FILE_MAX_DIR && dir)
                        std::strncpy(dir, &file_template[offset], std::min(len, FILE_MAX_DIR));
            if (len < FILE_MAX_DIR && dir)
                        dir[len] = 0;
            offset += len;
        }
    }
    else
        return 0;

    // get fname
    if (offset < length)
    {
        tmp = std::strrchr(file_template, '.');
        if (tmp < std::strrchr(file_template, SLASHC) || tmp < std::strrchr(file_template, ':'))
        {
            tmp = nullptr; // in this case the '.' must be a directory
        }
        if (tmp)
        {
            // tmp++; */ /* first character past "."
            len = tmp - &file_template[offset];
            if ((len > 0) && (offset+len < length) && fname)
            {
                std::strncpy(fname, &file_template[offset], std::min(len, FILE_MAX_FNAME));
                if (len < FILE_MAX_FNAME)
                            fname[len] = 0;
                else
                    fname[FILE_MAX_FNAME-1] = 0;
            }
            offset += len;
            if ((offset < length) && ext)
            {
                std::strncpy(ext, &file_template[offset], FILE_MAX_EXT);
                ext[FILE_MAX_EXT-1] = 0;
            }
        }
        else if ((offset < length) && fname)
        {
            std::strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
            fname[FILE_MAX_FNAME-1] = 0;
        }
    }
    return 0;
}

int
_splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext)
{
    return splitpath(file_template, drive, dir, fname, ext);
}

#endif
