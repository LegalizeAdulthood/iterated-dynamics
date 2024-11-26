// SPDX-License-Identifier: GPL-3.0-only
//
#include "ifs.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "file_gets.h"
#include "file_item.h"
#include "id.h"
#include "stop_msg.h"

#include <cstdio>
#include <cstring>

int g_num_affine_transforms{};

enum
{
    NUM_IFS_FUNCTIONS = 64
};

static const char *const s_token_separators{" \t\n\r"};

char *get_ifs_token(char *buf, std::FILE *ifsfile)
{
    while (true)
    {
        if (file_gets(buf, 200, ifsfile) < 0)
        {
            return nullptr;
        }

        char* bufptr = std::strchr(buf, ';');
        if (bufptr != nullptr) // use ';' as comment to eol
        {
            *bufptr = 0;
        }
        bufptr = std::strtok(buf, s_token_separators);
        if (bufptr != nullptr)
        {
            return bufptr;
        }
    }
}

char *get_next_ifs_token(char *buf, std::FILE *ifsfile)
{
    char *bufptr = std::strtok(nullptr, s_token_separators);
    if (bufptr != nullptr)
    {
        return bufptr;
    }
    return get_ifs_token(buf, ifsfile);
}

int ifs_load()                   // read in IFS parameters
{
    // release prior params
    g_ifs_definition.clear();
    g_ifs_type = false;
    std::FILE *ifsfile;
    if (find_file_item(g_ifs_filename, g_ifs_name.c_str(), &ifsfile, gfe_type::IFS))
    {
        return -1;
    }

    char  buf[201];
    file_gets(buf, 200, ifsfile);
    char *bufptr = std::strchr(buf, ';');
    if (bufptr != nullptr)   // use ';' as comment to eol
    {
        *bufptr = 0;
    }

    strlwr(buf);
    int const row_size = std::strstr(buf, "(3d)") != nullptr ? NUM_IFS_3D_PARAMS : NUM_IFS_PARAMS;
    g_ifs_type = row_size == NUM_IFS_3D_PARAMS;

    int ret = 0;
    int i = ret;
    bufptr = get_ifs_token(buf, ifsfile);
    while (bufptr != nullptr)
    {
        float value = 0.0f;
        if (std::sscanf(bufptr, " %f ", &value) != 1)
        {
            break;
        }
        g_ifs_definition.push_back(value);
        if (++i >= NUM_IFS_FUNCTIONS*row_size)
        {
            stop_msg("IFS definition has too many lines");
            ret = -1;
            break;
        }
        bufptr = get_next_ifs_token(buf, ifsfile);
        if (bufptr == nullptr)
        {
            ret = -1;
            break;
        }
        if (*bufptr == '}')
        {
            break;
        }
    }

    if ((i % row_size) != 0 || (bufptr && *bufptr != '}'))
    {
        stop_msg("invalid IFS definition");
        ret = -1;
    }
    if (i == 0 && ret == 0)
    {
        stop_msg("Empty IFS definition");
        ret = -1;
    }
    std::fclose(ifsfile);

    if (ret == 0)
    {
        g_num_affine_transforms = i/row_size;
    }
    return ret;
}
