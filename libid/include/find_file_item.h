#pragma once

#include <string>

bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, int itemtype);
bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, int itemtype);
