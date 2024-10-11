#pragma once

#include <vector>

enum
{
    WINTEXT_MAX_COL = 80,
    WINTEXT_MAX_ROW = 25
};

struct Screen
{
    std::vector<BYTE> chars;
    std::vector<BYTE> attrs;
};

class CGATextWindow
{
};
