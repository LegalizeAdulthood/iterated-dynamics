// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/command_effects.h"

#include "engine/calc_frac_init.h"
#include "engine/cmdfiles.h"
#include "engine/LogicalScreen.h"
#include "engine/sound.h"
#include "engine/spindac.h"
#include "engine/text_color.h"
#include "helpcom.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "io/make_mig.h"
#include "misc/Driver.h"
#include "ui/help_title.h"
#include "ui/make_batch_file.h"
#include "ui/zoom.h"

#include <fmt/format.h>

#include <stdexcept>

using namespace id::engine;
using namespace id::help;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

namespace
{

void make_document(const MakeHelpDocument &request)
{
    constexpr int BOX_ROW{6};
    constexpr int BOX_COL{11};
    constexpr int BOX_WIDTH{57};
    constexpr int BOX_DEPTH{12};
    help_title();
    driver_set_attr(1, 0, C_DVID_BKGRD, 24 * 80);
    for (int row = 0; row < BOX_DEPTH; ++row)
    {
        driver_set_attr(BOX_ROW + row, BOX_COL, C_DVID_LO, BOX_WIDTH);
    }
    driver_put_string(BOX_ROW + 2, BOX_COL + 4, C_DVID_HI, "Creating Help Document");
    driver_put_string(BOX_ROW + 4, BOX_COL + 4, C_DVID_LO, "Save name: " + request.filename.string());
    driver_put_string(BOX_ROW + 6, BOX_COL + 4, C_DVID_LO, "Status:");
    print_document(request.filename.string().c_str(), make_doc_msg_func);
}

void make_parameter_file(const MakeParameterFile &request)
{
    g_parameter_file = request.filename;
    g_parameter_set_name = request.entry_name;
    g_make_parameter_file = true;
    if (!g_read_filename.empty())
    {
        std::filesystem::path path{g_read_filename};
        if (!path.has_extension())
        {
            path.replace_extension(".gif");
        }
        const std::filesystem::path found{find_file(ReadFile::IMAGE, path)};
        ImageMetadata metadata{read_image_metadata(found.empty() ? path : found)};
        apply_image_parameters(metadata);
        apply_image_extensions(metadata);
    }
    else if (!g_map_name.empty())
    {
        g_make_parameter_file_map = true;
    }
    g_logical_screen.x_dots = g_file_x_dots;
    g_logical_screen.y_dots = g_file_y_dots;
    g_logical_screen.x_size_dots = g_logical_screen.x_dots - 1;
    g_logical_screen.y_size_dots = g_logical_screen.y_dots - 1;
    invalidate_translated_resume(calc_frac_init());
    make_batch_file();
}

} // namespace

bool apply_command_effects(CommandParseResult &result)
{
    if (!result.diagnostics.errors.empty())
    {
        return false;
    }
    if (result.initialize_fm && (g_sound_flag & SOUNDFLAG_OPL3_FM) != 0 && !driver_init_fm())
    {
        g_sound_flag &= ~SOUNDFLAG_OPL3_FM;
    }
    if (result.action.index() == 0)
    {
        return false;
    }
    try
    {
        if (const MakeHelpDocument *request = std::get_if<MakeHelpDocument>(&result.action))
        {
            make_document(*request);
        }
        else if (const MakeParameterFile *request = std::get_if<MakeParameterFile>(&result.action))
        {
            make_parameter_file(*request);
        }
        else if (const MakeMultiImageGif *request = std::get_if<MakeMultiImageGif>(&result.action))
        {
            fmt::print("\n Generating multi-image GIF file fractmig.gif using {} X and {} Y components\n\n",
                request->x_mult, request->y_mult);
            make_mig(request->x_mult, request->y_mult);
            fmt::print("File fractmig.gif has been created (and its component files deleted)\n");
        }
    }
    catch (const std::runtime_error &error)
    {
        result.diagnostics.errors.push_back(error.what());
    }
    return true;
}

} // namespace id::ui
