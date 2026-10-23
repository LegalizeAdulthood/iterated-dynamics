// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace id::fractals
{
struct FractalSpecific;
}

namespace id::engine
{

class Command;

enum class RecordColorsMode
{
    NONE = 0,
    AUTOMATIC = 'a',
    COMMENT = 'c',
    YES = 'y'
};

enum class CmdFile
{
    AT_CMD_LINE = 0,         // command line @filename
    SSTOOLS_INI = 1,         // sstools.ini
    AT_AFTER_STARTUP = 2,    // <@> command after startup
    AT_CMD_LINE_SET_NAME = 3 // command line @filename/setname
};

enum class CmdArgFlags
{
    BAD_ARG          = -1,
    NONE             = 0,
    FRACTAL_PARAM    = 1,
    PARAM_3D         = 2,
    YES_3D           = 4,
    RESET            = 8,
};

inline int operator+(const CmdArgFlags value)
{
    return static_cast<int>(value);
}

inline CmdArgFlags operator|(const CmdArgFlags lhs, const CmdArgFlags rhs)
{
    return static_cast<CmdArgFlags>(+lhs | +rhs);
}

inline CmdArgFlags &operator|=(CmdArgFlags &lhs, const CmdArgFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline bool bit_set(const CmdArgFlags flags, const CmdArgFlags bit)
{
    return (+flags & +bit) == +bit;
}

// for init_batch
enum class BatchMode
{
    FINISH_CALC_BEFORE_SAVE = -1,
    NONE,
    NORMAL,
    SAVE,
    BAILOUT_ERROR_NO_SAVE,
    BAILOUT_INTERRUPTED_TRY_SAVE,
    BAILOUT_INTERRUPTED_SAVE
};

extern bool                  g_colors_preloaded;
extern std::filesystem::path g_parameter_file;
extern std::string           g_parameter_set_name;
extern bool                  g_escape_exit;
extern bool                  g_first_init;
extern std::string           g_image_filename_mask;
extern BatchMode             g_init_batch;
extern bool                  g_read_color;
constexpr int MAX_MAX_LINE_LENGTH{128};
constexpr int MIN_MAX_LINE_LENGTH{40};
extern int g_max_line_length;

extern RecordColorsMode      g_record_colors;

struct CommandDiagnostics
{
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct MakeHelpDocument
{
    std::filesystem::path filename;
};

struct MakeParameterFile
{
    std::filesystem::path filename;
    std::string entry_name;
};

struct MakeMultiImageGif
{
    unsigned int x_mult;
    unsigned int y_mult;
};

using CommandAction = std::variant<std::monostate, MakeHelpDocument, MakeParameterFile, MakeMultiImageGif>;

struct CommandParseResult
{
    CmdArgFlags flags{};
    CommandDiagnostics diagnostics;
    CommandAction action;
    bool needs_file_entry{};
    bool initialize_fm{};
};

class CommandProcessor
{
public:
    // Adopt an open file at its existing source position.
    CommandProcessor(std::FILE *file, CmdFile mode);
    ~CommandProcessor();
    CommandProcessor(const CommandProcessor &) = delete;
    CommandProcessor(CommandProcessor &&) = delete;
    CommandProcessor &operator=(const CommandProcessor &) = delete;
    CommandProcessor &operator=(CommandProcessor &&) = delete;

    // Collect the previous outcome and expose the next command, if any.
    bool advance();

    // The command remains valid until the next advance.
    const Command &command() const;

    const CommandParseResult &result() const
    {
        return m_result;
    }

private:
    using FilePtr = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

    void finish_file();

    FilePtr m_file;
    CmdFile m_mode;
    std::string m_text;
    std::string m_line_buf;
    std::string_view m_line;
    CommandParseResult m_result;
    CommandParseResult m_command_result;
    std::unique_ptr<Command> m_command;
};

CommandParseResult cmd_files(const std::vector<std::string> &args);
CommandParseResult reset_startup_commands();
CommandParseResult load_commands(std::FILE *infile);
void set_3d_defaults();
CommandParseResult cmd_arg(std::string_view cur_arg, CmdFile mode);

// Unknown names return nullopt; recognized rejections return BAD_ARG.
std::optional<CmdArgFlags> handle_engine_command(const Command &cmd);

int next_command(std::string &cmd, std::FILE *handle, std::string &line_buf, std::string_view &line, CmdFile mode,
    CommandDiagnostics *diagnostics = nullptr);

} // namespace id::engine
