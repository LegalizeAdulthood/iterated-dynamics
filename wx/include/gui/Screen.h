#pragma once

#include <config/port.h>

#include <vector>

namespace id::gui
{

struct Screen
{
    std::vector<Byte> chars;
    std::vector<Byte> attrs;
};

} // namespace id::gui
