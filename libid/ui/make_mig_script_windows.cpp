// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "ui/make_mig_script.h"

#include <fmt/core.h>

namespace id::ui
{

void set_make_mig_script_executable(const char * /*path*/)
{
}

std::filesystem::path make_mig_script_filename()
{
    return "makemig.bat";
}

void write_make_mig_script_header(std::FILE * /*script_file*/)
{
}

void write_make_mig_script_piece(
    std::FILE *script_file, const std::filesystem::path &parameter_file, const std::string &entry_name)
{
    fmt::print(script_file,
        "start/wait"
        " id"
        " batch=yes"
        " overwrite=yes"
        " @{:s}/{:s}\n"
        "if errorlevel 2 goto oops\n",
        parameter_file.string(), entry_name);
}

void write_make_mig_script_finish(std::FILE *script_file, const int x_multiple, const int y_multiple)
{
    fmt::print(script_file,
        "start/wait"
        " id"
        " makemig={:d}/{:d}\n"
        ":oops\n",
        x_multiple, y_multiple);
}

void finish_make_mig_script(const std::filesystem::path & /*script_path*/)
{
}

} // namespace id::ui
