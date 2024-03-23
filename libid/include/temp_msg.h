#pragma once

#include <string>

int texttempmsg(char const *);
bool showtempmsg(char const *);
inline bool showtempmsg(const std::string &msg)
{
    return showtempmsg(msg.c_str());
}
void cleartempmsg();
void freetempmsg();
