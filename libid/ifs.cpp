#include "ifs.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "file_gets.h"
#include "find_file_item.h"
#include "id.h"
#include "stop_msg.h"

#include <cstdio>
#include <cstring>

static const char *const seps{" \t\n\r"};

char *get_ifs_token(char *buf, std::FILE *ifsfile)
{
    char *bufptr;
    while (true)
    {
        if (file_gets(buf, 200, ifsfile) < 0)
        {
            return nullptr;
        }
        else
        {
            bufptr = std::strchr(buf, ';');
            if (bufptr != nullptr)   // use ';' as comment to eol
            {
                *bufptr = 0;
            }
            bufptr = std::strtok(buf, seps);
            if (bufptr != nullptr)
            {
                return bufptr;
            }
        }
    }
}

char *get_next_ifs_token(char *buf, std::FILE *ifsfile)
{
    char *bufptr = std::strtok(nullptr, seps);
    if (bufptr == nullptr)
    {
        bufptr = get_ifs_token(buf, ifsfile);
    }
    return bufptr;
}

int g_num_affine_transforms;
int ifsload()                   // read in IFS parameters
{
    // release prior params
    g_ifs_definition.clear();
    g_ifs_type = false;
    std::FILE *ifsfile;
    if (find_file_item(g_ifs_filename, g_ifs_name.c_str(), &ifsfile, 3))
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
    bufptr = &buf[0];
    int rowsize = NUM_IFS_PARAMS;
    while (*bufptr)
    {
        if (std::strncmp(bufptr, "(3d)", 4) == 0)
        {
            g_ifs_type = true;
            rowsize = NUM_IFS_3D_PARAMS;
        }
        ++bufptr;
    }

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
        if (++i >= NUM_IFS_FUNCTIONS*rowsize)
        {
            stopmsg(STOPMSG_NONE, "IFS definition has too many lines");
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

    if ((i % rowsize) != 0 || (bufptr && *bufptr != '}'))
    {
        stopmsg(STOPMSG_NONE, "invalid IFS definition");
        ret = -1;
    }
    if (i == 0 && ret == 0)
    {
        stopmsg(STOPMSG_NONE, "Empty IFS definition");
        ret = -1;
    }
    std::fclose(ifsfile);

    if (ret == 0)
    {
        g_num_affine_transforms = i/rowsize;
    }
    return ret;
}
