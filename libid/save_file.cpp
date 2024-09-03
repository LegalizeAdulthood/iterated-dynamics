#include "save_file.h"

std::FILE *open_save_file(const std::string &name, const std::string &mode)
{
    return std::fopen(name.c_str(), mode.c_str());
}
