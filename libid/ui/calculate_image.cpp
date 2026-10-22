// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/calculate_image.h"

#include "engine/calcfrac.h"
#include "ui/stop_msg.h"
#include "ui/diskvid.h"

#include <string>

using namespace id::engine;

namespace id::ui
{

void complete_image_setup(ImageCalculation &image, const std::vector<std::string> &warnings)
{
    for (const std::string &warning : warnings)
    {
        stop_msg(warning);
    }
    if (!image.targa_storage().header.empty())
    {
        const std::string warning{image.complete_targa_storage(prepare_targa_storage(image.targa_storage()))};
        if (!warning.empty())
        {
            stop_msg(warning);
        }
    }
}

} // namespace id::ui
