// SPDX-License-Identifier: GPL-3.0-only
//
#include "ifs.h"

#include "cmdfiles.h"
#include "file_gets.h"
#include "file_item.h"
#include "stop_msg.h"

#include <cstdio>
#include <cstring>

int g_num_affine_transforms{};

enum
{
    NUM_IFS_FUNCTIONS = 64
};

static const char *const s_token_separators{" \t\n\r"};

char *get_ifs_token(char *buf, std::FILE *ifs_file)
{
    while (true)
    {
        if (file_gets(buf, 200, ifs_file) < 0)
        {
            return nullptr;
        }

        char* buf_ptr = std::strchr(buf, ';');
        if (buf_ptr != nullptr) // use ';' as comment to eol
        {
            *buf_ptr = 0;
        }
        buf_ptr = std::strtok(buf, s_token_separators);
        if (buf_ptr != nullptr)
        {
            return buf_ptr;
        }
    }
}

char *get_next_ifs_token(char *buf, std::FILE *ifs_file)
{
    char *buf_ptr = std::strtok(nullptr, s_token_separators);
    if (buf_ptr != nullptr)
    {
        return buf_ptr;
    }
    return get_ifs_token(buf, ifs_file);
}

int ifs_load()                   // read in IFS parameters
{
    // release prior params
    g_ifs_definition.clear();
    g_ifs_type = false;
    std::FILE *ifs_file;
    if (find_file_item(g_ifs_filename, g_ifs_name.c_str(), &ifs_file, ItemType::IFS))
    {
        return -1;
    }

    char  buf[201];
    file_gets(buf, 200, ifs_file);
    char *buf_ptr = std::strchr(buf, ';');
    if (buf_ptr != nullptr)   // use ';' as comment to eol
    {
        *buf_ptr = 0;
    }

    strlwr(buf);
    int const row_size = std::strstr(buf, "(3d)") != nullptr ? NUM_IFS_3D_PARAMS : NUM_IFS_PARAMS;
    g_ifs_type = row_size == NUM_IFS_3D_PARAMS;

    int ret = 0;
    int i = ret;
    buf_ptr = get_ifs_token(buf, ifs_file);
    while (buf_ptr != nullptr)
    {
        float value = 0.0f;
        if (std::sscanf(buf_ptr, " %f ", &value) != 1)
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
        buf_ptr = get_next_ifs_token(buf, ifs_file);
        if (buf_ptr == nullptr)
        {
            ret = -1;
            break;
        }
        if (*buf_ptr == '}')
        {
            break;
        }
    }

    if ((i % row_size) != 0 || (buf_ptr && *buf_ptr != '}'))
    {
        stop_msg("invalid IFS definition");
        ret = -1;
    }
    if (i == 0 && ret == 0)
    {
        stop_msg("Empty IFS definition");
        ret = -1;
    }
    std::fclose(ifs_file);

    if (ret == 0)
    {
        g_num_affine_transforms = i/row_size;
    }
    return ret;
}
