// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/diffusion_scan.h"

#include "engine/Diffusion.h"

namespace id::engine
{

int diffusion_scan()
{
    Diffusion diffusion;
    return diffusion.scan();
}

} // namespace id::engine
