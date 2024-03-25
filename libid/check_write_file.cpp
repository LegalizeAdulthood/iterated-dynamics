#include "check_write_file.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "has_ext.h"
#include "update_save_name.h"

#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static void check_writefile(char *name, char const *ext)
{
    // after v16 release, change encoder.c to also use this routine
    char openfile[FILE_MAX_DIR];
    char opentype[20];
    char const *period;
nextname:
    std::strcpy(openfile, name);
    std::strcpy(opentype, ext);
    period = has_ext(openfile);
    if (period != nullptr)
    {
        std::strcpy(opentype, period);
        openfile[period - openfile] = 0;
    }
    std::strcat(openfile, opentype);
    if (!fs::exists(openfile))
    {
        std::strcpy(name, openfile);
        return;
    }
    // file already exists
    if (!g_overwrite_file)
    {
        update_save_name(name);
        goto nextname;
    }
}

void check_writefile(std::string &name, char const *ext)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, name.c_str());
    check_writefile(buff, ext);
    name = buff;
}
