// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/CommandRouter.h"

#include "engine/CommandProcessor.h"
#include "engine/VideoInfo.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "misc/debug_flags.h"
#include "ui/big_while_loop.h"
#include "ui/history.h"
#include "ui/slideshw.h"

#include <fmt/format.h>

#include <filesystem>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

// askvideo=?
static CmdArgFlags cmd_ask_video(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_ask_video = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_auto_key(const Command &cmd)
{
    if (cmd.value == "record")
    {
        g_slides = SlidesMode::RECORD;
    }
    else if (cmd.value == "play")
    {
        g_slides = SlidesMode::PLAY;
    }
    else
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_auto_key_name(const Command &cmd)
{
    if (const fs::path path{find_file(ReadFile::KEY, fs::path{std::string{cmd.value}})}; !path.empty())
    {
        g_auto_name = path;
    }
    else
    {
        g_auto_name = std::string{cmd.value};
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_max_history(const Command &cmd)
{
    if (cmd.num_val == NON_NUMERIC)
    {
        return cmd.bad_arg();
    }
    if (cmd.num_val < 0)
    {
        return cmd.bad_arg();
    }
    g_max_image_history = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// virtual=?
static CmdArgFlags cmd_virtual(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_virtual_screens = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM;
}

static const CommandHandlerTables s_ui_commands{{CommandHandler{"maxhistory", cmd_max_history}},
    {CommandHandler{"askvideo", cmd_ask_video}, CommandHandler{"autokey", cmd_auto_key},
        CommandHandler{"autokeyname", cmd_auto_key_name}, CommandHandler{"virtual", cmd_virtual}}};

static void add_names(std::set<std::string_view> &names, const std::vector<CommandHandler> &handlers)
{
    for (const CommandHandler &handler : handlers)
    {
        if (!names.insert(handler.name).second)
        {
            throw std::logic_error(fmt::format("Duplicate parameter owner: {}", handler.name));
        }
    }
}

CommandRouter::CommandRouter() :
    CommandRouter(s_ui_commands, engine_command_handlers())
{
}

CommandRouter::CommandRouter(CommandHandlerTables ui, CommandHandlerTables engine) :
    m_ui(std::move(ui)),
    m_engine(std::move(engine))
{
    std::set<std::string_view> names;
    add_names(names, m_ui.startup);
    add_names(names, m_ui.unrestricted);
    add_names(names, m_engine.startup);
    add_names(names, m_engine.unrestricted);
}

CommandParseResult CommandRouter::process(CommandProcessor &processor) const
{
    while (processor.advance())
    {
        const Command &cmd{processor.command()};
        CmdArgFlags flags{cmd.status};
        if (flags == CmdArgFlags::NONE)
        {
            std::optional<CmdArgFlags> handled{handle_command(m_ui, cmd)};
            if (!handled.has_value())
            {
                handled = handle_command(m_engine, cmd);
            }
            flags = handled.has_value() ? *handled : cmd.bad_arg();
        }
        cmd.result.flags = flags;
    }
    return processor.result();
}

static void reset_restart_defaults()
{
    g_ask_video = true;
    g_virtual_screens = true;
    g_init_mode = -1;
    engine::reset_restart_defaults();
}

CommandParseResult CommandRouter::process_startup(const std::vector<std::string> &args) const
{
    if (g_first_init)
    {
        init_process_defaults();
    }
    reset_restart_defaults();
    reset_fractal_defaults();
    CommandProcessor processor{args};
    CommandParseResult result{process(processor)};

    if (!g_first_init)
    {
        g_init_mode = -1;
        g_show_file = ShowFile::IMAGE_LOADED;
    }
    if (g_debug_flag != DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)
    {
        g_first_init = false;
    }
    g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;
    return result;
}

CommandParseResult CommandRouter::reset_startup() const
{
    reset_restart_defaults();
    reset_fractal_defaults();
    CommandProcessor processor;
    CommandParseResult result{process(processor)};
    g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;
    return result;
}

} // namespace id::ui
