// SPDX-License-Identifier: GPL-3.0-only
//

#include "ui/read_ticker.h"

#include <ctime>

// returns current ticker value
long read_ticker()
{
    return std::clock();
}

