#include "check_write_file.h"

#include "cmdfiles.h"
#include "update_save_name.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void check_writefile(std::string &name, char const *ext)
{
    do
    {
        fs::path openfile{name};
        if (!openfile.has_extension())
        {
            openfile.replace_extension(ext);
        }
        if (!exists(openfile))
        {
            name = openfile.string();
            return;
        }
        // file already exists
        if (!g_overwrite_file)
        {
            update_save_name(name);
        }
    } while (!g_overwrite_file);
}
