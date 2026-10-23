// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/CommandProcessor.h"

#include "engine/Browse.h"
#include "engine/Command.h"
#include "engine/command_comments.h"
#include "engine/VideoInfo.h"
#include "fractals/formula.h"
#include "fractals/fractalp.h"
#include "fractals/ifs.h"
#include "fractals/lsystem.h"
#include "io/file_gets.h"
#include "io/file_item.h"
#include "io/has_ext.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "misc/debug_flags.h"

#include <algos/string_algorithms.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace id::fractals;
using namespace id::io;
using namespace id::misc;

namespace id::engine
{

static CommandParseResult command_file(std::FILE *handle, CmdFile mode);
static void append_result(CommandParseResult &result, CommandParseResult source);
static CommandParseResult process_commands(CommandProcessor &processor);
static bool next_line(std::FILE *handle, std::string &line_buf, CmdFile mode);

static std::string extract_filename(const fs::path &source)
{
    return source.filename().string();
}

// cmdfiles(argc,argv) process the command-line arguments
// it also processes the 'sstools.ini' file and any
// indirect files ('id @myfile')

static std::FILE *open_sstools_ini()
{
    const std::string sstools_ini{find_file(ReadFile::ID_CONFIG, "sstools.ini").string()}; // look for SSTOOLS.INI
    if (sstools_ini.empty())
    {
        return nullptr;
    }

    return std::fopen(sstools_ini.c_str(), "r");
}

static bool type_needs_file_entry()
{
    return g_fractal_type == FractalType::FORMULA || g_fractal_type == FractalType::L_SYSTEM ||
        g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D;
}

static bool type_has_file_entry()
{
    switch (g_fractal_type)
    {
    case FractalType::FORMULA:
        return !g_formula_name.empty();
    case FractalType::L_SYSTEM:
        return !g_l_system_name.empty();
    case FractalType::IFS:
    case FractalType::IFS_3D:
        return !g_ifs_name.empty();
    default:
        return true;
    }
}

CommandParseResult reset_startup_commands()
{
    reset_restart_defaults();
    reset_fractal_defaults();
    CommandParseResult result;
    if (std::FILE *file{open_sstools_ini()}; file != nullptr)
    {
        append_result(result, command_file(file, CmdFile::SSTOOLS_INI));
    }
    g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;
    return result;
}

static bool process_image_argument(const char *cur_arg)
{
    bool processed{};
    if (std::strchr(cur_arg, '=') == nullptr)
    {
        // not xxx=yyy, so check for gif
        std::string filename = cur_arg;
        if (!has_ext(filename))
        {
            filename += ".gif";
        }
        filename = find_file(ReadFile::IMAGE, filename).string();
        if (!filename.empty())
        {
            if (std::FILE *init_file = std::fopen(filename.c_str(), "rb"))
            {
                char signature[6];
                if (std::fread(signature, 1, 6, init_file) != 6)
                {
                    throw std::system_error(errno, std::system_category(), "process_simple_command failed fread");
                }
                if (signature[0] == 'G' && signature[1] == 'I' && signature[2] == 'F' && signature[3] >= '8' &&
                    signature[3] <= '9' && signature[4] >= '0' && signature[4] <= '9')
                {
                    g_read_filename = find_file(ReadFile::IMAGE, cur_arg);
                    if (!g_read_filename.empty())
                    {
                        g_browse.selected_path = g_read_filename;
                        g_browse.name = extract_filename(g_read_filename);
                        g_show_file = ShowFile::LOAD_IMAGE;
                    }
                    processed = true;
                }
                std::fclose(init_file);
            }
        }
    }
    return processed;
}

static std::FILE *open_parameter_set(
    const std::string &filename, const std::string &param_name, CommandParseResult &result)
{
    const std::filesystem::path saved_path{g_parameter_file};
    const std::string saved_name{g_parameter_set_name};
    fs::path path{filename};
    if (!path.has_extension())
    {
        path.replace_extension(".par");
    }
    path = find_file(ReadFile::PARAMETER, path);
    if (path.empty())
    {
        return nullptr;
    }
    g_parameter_file = path;
    g_parameter_set_name = param_name;
    std::FILE *init_file = nullptr;
    std::string diagnostic;
    if (find_file_item(g_parameter_file, g_parameter_set_name, &init_file, ItemType::PAR_SET, diagnostic) ||
        init_file == nullptr)
    {
        g_parameter_file = saved_path;
        g_parameter_set_name = saved_name;
        if (!diagnostic.empty())
        {
            result.diagnostics.errors.push_back(diagnostic);
        }
        return nullptr;
    }
    return init_file;
}

static std::FILE *open_parameter_file(const char *cur_arg)
{
    fs::path path{&cur_arg[1]};
    if (!path.has_extension())
    {
        path.replace_extension(".par");
    }
    path = find_file(ReadFile::PARAMETER, path.string());
    if (path.empty())
    {
        return nullptr;
    }

    return std::fopen(path.string().c_str(), "r");
}

static void resolve_startup_file_entry(CommandParseResult &result)
{
    if (!type_needs_file_entry() || type_has_file_entry())
    {
        return;
    }

    if (g_init_batch != BatchMode::NONE)
    {
        switch (g_fractal_type)
        {
        case FractalType::FORMULA:
            arg_error("type=formula requires formulaname=", &result.diagnostics);
            break;
        case FractalType::L_SYSTEM:
            arg_error("type=lsystem requires lname=", &result.diagnostics);
            break;
        case FractalType::IFS:
        case FractalType::IFS_3D:
            arg_error("type=ifs requires ifs=", &result.diagnostics);
            break;
        default:
            break;
        }
        return;
    }

    result.needs_file_entry = true;
}

CommandParseResult cmd_files(const std::vector<std::string> &args)
{
    if (g_first_init)
    {
        init_process_defaults();
    }
    reset_restart_defaults();
    reset_fractal_defaults();
    CommandProcessor processor{args};
    CommandParseResult result{process_commands(processor)};

    if (!g_first_init)
    {
        g_init_mode = -1;                     // don't set video when <ins> key used
        g_show_file = ShowFile::IMAGE_LOADED; // nor startup image file
    }

    if (g_debug_flag != DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)
    {
        g_first_init = false;
    }

    // PAR reads a file and sets color, don't read colors from GIF
    g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;
    return result;
}

// when called, file is open in binary mode, positioned at the
// '(' or '{' following the desired parameter set's name
CommandParseResult load_commands(std::FILE *infile)
{
    CommandParseResult ret{command_file(infile, CmdFile::AT_AFTER_STARTUP)};

    // PAR reads a file and sets color, don't read colors from GIF
    g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;

    return ret;
}

// mode = AT_CMD_LINE           command line @filename
//        SSTOOLS_INI           sstools.ini
//        AT_AFTER_STARTUP      <@> command after startup
//        AT_CMD_LINE_SET_NAME  command line @filename/setname
// note that cmdfile could be open as text OR as binary.
// binary is used in @ command processing for reasonable speed note/point
static void append_result(CommandParseResult &result, CommandParseResult source)
{
    if (source.flags != CmdArgFlags::BAD_ARG)
    {
        result.flags |= source.flags;
    }
    result.diagnostics.errors.insert(result.diagnostics.errors.end(),
        std::make_move_iterator(source.diagnostics.errors.begin()),
        std::make_move_iterator(source.diagnostics.errors.end()));
    result.diagnostics.warnings.insert(result.diagnostics.warnings.end(),
        std::make_move_iterator(source.diagnostics.warnings.begin()),
        std::make_move_iterator(source.diagnostics.warnings.end()));
    if (source.action.index() != 0)
    {
        result.action = std::move(source.action);
    }
    result.initialize_fm = result.initialize_fm || source.initialize_fm;
}

CommandProcessor::CommandProcessor() :
    m_file{open_sstools_ini(), std::fclose},
    m_mode(CmdFile::SSTOOLS_INI)
{
}

CommandProcessor::CommandProcessor(std::FILE *file, const CmdFile mode) :
    m_file{file, std::fclose},
    m_mode(mode)
{
    if (file == nullptr)
    {
        throw std::logic_error("CommandProcessor requires an open file");
    }
    if (mode == CmdFile::AT_AFTER_STARTUP)
    {
        init_param_flags();
    }
    if (mode == CmdFile::AT_AFTER_STARTUP || mode == CmdFile::AT_CMD_LINE_SET_NAME)
    {
        int i;
        while ((i = std::getc(m_file.get())) != '{' && i != EOF)
        {
        }
        clear_command_comments();
    }
}

CommandProcessor::~CommandProcessor() = default;

CommandProcessor::CommandProcessor(std::vector<std::string> args) :
    m_source(Source::CONFIGURATION),
    m_args(std::move(args))
{
    if (std::FILE *file{open_sstools_ini()}; file != nullptr)
    {
        m_source_file = std::make_unique<CommandProcessor>(file, CmdFile::SSTOOLS_INI);
    }
}

const Command &CommandProcessor::command() const
{
    if (m_source_file)
    {
        return m_source_file->command();
    }
    if (!m_command)
    {
        throw std::logic_error("CommandProcessor has no current command");
    }
    return *m_command;
}

bool CommandProcessor::advance()
{
    if (m_source == Source::SINGLE_COMMAND)
    {
        return advance_single();
    }
    if (m_source != Source::FILE)
    {
        return advance_startup();
    }
    if (!m_file)
    {
        return false;
    }
    if (m_command)
    {
        const bool failed{m_command_result.flags == CmdArgFlags::BAD_ARG};
        m_command.reset();
        append_result(m_result, std::move(m_command_result));
        m_command_result = {};
        if (failed || m_result.action.index() != 0)
        {
            finish_file();
            return false;
        }
    }

    if (next_command(m_text, m_file.get(), m_line_buf, m_line, m_mode, &m_result.diagnostics) <= 0)
    {
        finish_file();
        return false;
    }
    if ((m_mode == CmdFile::AT_AFTER_STARTUP || m_mode == CmdFile::AT_CMD_LINE_SET_NAME) && m_text == "}")
    {
        finish_file();
        return false;
    }
    m_command = std::make_unique<Command>(m_text, m_mode, m_command_result);
    m_command_result.flags = m_command->status;
    return true;
}

CommandProcessor::CommandProcessor(const std::string_view text, const CmdFile mode) :
    m_mode(mode),
    m_text(text),
    m_source(Source::SINGLE_COMMAND)
{
}

bool CommandProcessor::advance_single()
{
    if (m_command)
    {
        m_command.reset();
        m_result = std::move(m_command_result);
        m_source = Source::DONE;
        return false;
    }
    m_command = std::make_unique<Command>(m_text, m_mode, m_command_result);
    m_command_result.flags = m_command->status;
    return true;
}

bool CommandProcessor::advance_startup()
{
    if (m_source == Source::DONE)
    {
        return false;
    }
    if (m_command)
    {
        m_command.reset();
        append_result(m_result, std::move(m_command_result));
        m_command_result = {};
    }
    while (true)
    {
        if (m_source_file)
        {
            if (m_source_file->advance())
            {
                return true;
            }
            append_result(m_result, m_source_file->result());
            m_source_file.reset();
        }
        if (m_source == Source::CONFIGURATION)
        {
            g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;
            m_source = Source::ARGUMENTS;
        }
        if (m_arg_index >= m_args.size() || m_result.action.index() != 0)
        {
            break;
        }
        m_text = m_args[m_arg_index++];
        const char first_char{m_text.empty() ? '\0' : m_text.front()};
        if (first_char == ';')
        {
            break;
        }
        if (first_char == '@')
        {
            std::FILE *file{};
            if (char *slash{std::strchr(m_text.data(), '/')}; slash != nullptr)
            {
                file = open_parameter_set(std::string{m_text.data() + 1, slash}, slash + 1, m_result);
            }
            else
            {
                file = open_parameter_file(m_text.c_str());
            }
            if (file == nullptr)
            {
                arg_error(m_text.c_str(), &m_result.diagnostics);
            }
            else
            {
                m_source_file = std::make_unique<CommandProcessor>(file, CmdFile::AT_CMD_LINE_SET_NAME);
            }
        }
        else if (!process_image_argument(m_text.c_str()))
        {
            m_command = std::make_unique<Command>(m_text, CmdFile::AT_CMD_LINE, m_command_result);
            m_command_result.flags = m_command->status;
            return true;
        }
    }
    if (m_result.diagnostics.errors.empty() && m_result.action.index() == 0)
    {
        resolve_startup_file_entry(m_result);
    }
    m_source = Source::DONE;
    return false;
}

void CommandProcessor::finish_file()
{
    const FilePtr file{std::move(m_file)};
    m_line = {};
    if (bit_set(m_result.flags, CmdArgFlags::FRACTAL_PARAM))
    {
        backwards_legacy_v18();
        backwards_legacy_v19();
        backwards_legacy_v20();
    }
}

// TODO: removed later
static CommandParseResult command_file(std::FILE *handle, const CmdFile mode)
{
    CommandProcessor processor{handle, mode};
    return process_commands(processor);
}

// TODO: removed later
static CommandParseResult process_commands(CommandProcessor &processor)
{
    while (processor.advance())
    {
        const Command &cmd{processor.command()};
        if (const std::optional<CmdArgFlags> flags{handle_engine_command(cmd)}; flags.has_value())
        {
            cmd.result.flags = *flags;
        }
        else
        {
            cmd.result.flags = cmd.bad_arg();
        }
    }
    return processor.result();
}

int next_command(std::string &cmd, std::FILE *handle, std::string &line_buf, std::string_view &line, const CmdFile mode,
    CommandDiagnostics *diagnostics)
{
    cmd.clear();
    while (true)
    {
        while (line.empty() || line[0] <= ' ' || line[0] == ';')
        {
            if (!cmd.empty()) // space or ; marks end of command
            {
                return static_cast<int>(cmd.size());
            }
            while (!line.empty() && line[0] <= ' ')
            {
                line.remove_prefix(1); // skip spaces and tabs
            }
            if (!line.empty() && line[0] == ';')
            {
                if ((mode == CmdFile::AT_AFTER_STARTUP || mode == CmdFile::AT_CMD_LINE_SET_NAME) &&
                    (g_command_comment[0].empty() || g_command_comment[1].empty() || g_command_comment[2].empty() ||
                        g_command_comment[3].empty()))
                {
                    // save comment
                    line.remove_prefix(1);
                    while (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
                    {
                        line.remove_prefix(1);
                    }
                    if (!line.empty())
                    {
                        const std::string_view comment{line.substr(0, MAX_COMMENT_LEN - 1)};
                        for (std::string &elem : g_command_comment)
                        {
                            if (elem.empty())
                            {
                                elem = comment;
                                break;
                            }
                        }
                    }
                }
                if (next_line(handle, line_buf, mode))
                {
                    return -1;   // eof
                }
                line = line_buf; // start new line
            }
            else if (line.empty() && next_line(handle, line_buf, mode))
            {
                return -1; // eof
            }
            else if (line.empty())
            {
                line = line_buf;                 // start new line
            }
        }
        if (line[0] == '\\' && line.size() == 1) // continuation onto next line?
        {
            if (next_line(handle, line_buf, mode))
            {
                arg_error(cmd.c_str(), diagnostics); // missing continuation
                return -1;
            }
            line = line_buf;
            while (!line.empty() && line[0] <= ' ')
            {
                line.remove_prefix(1); // skip white space @ start next line
            }
            continue;                  // loop to check end of line again
        }
        cmd.push_back(line[0]);        // copy character to command buffer
        line.remove_prefix(1);
    }
}

static bool next_line(std::FILE *handle, std::string &line_buf, const CmdFile mode)
{
    bool tools_section = true;
    char raw_line[513];
    while (file_gets(raw_line, 512, handle) >= 0)
    {
        line_buf = raw_line;
        if (mode == CmdFile::SSTOOLS_INI && !line_buf.empty() && line_buf[0] == '[') // check for [id]
        {
            const std::string section{id::algos::ascii_to_lower_copy(std::string_view{line_buf}.substr(1, 3))};
            tools_section = section == "id]";
            continue; // skip tools section heading
        }
        if (tools_section)
        {
            return false;
        }
    }
    return true;
}

} // namespace id::engine
