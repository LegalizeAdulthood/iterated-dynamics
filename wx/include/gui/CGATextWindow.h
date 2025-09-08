#pragma once

#include <config/port.h>

#include <vector>

namespace id::gui
{

enum
{
    WINTEXT_MAX_COL = 80,
    WINTEXT_MAX_ROW = 25
};

struct Screen
{
    std::vector<Byte> chars;
    std::vector<Byte> attrs;
};

class CGATextWindow
{
};

} // namespace id::gui
