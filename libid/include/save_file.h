#pragma once

#include <cstdio>
#include <string>

std::FILE *open_save_file(const std::string &name, const std::string &mode);
