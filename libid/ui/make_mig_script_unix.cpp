// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "ui/make_mig_script.h"

#include <fmt/core.h>

#include <cstdint>
#include <string_view>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace id::ui
{

namespace
{

fs::path s_executable{"xid"};

fs::path canonical_path(const fs::path &path)
{
    std::error_code err;
    const fs::path canonical{fs::weakly_canonical(path, err)};
    return err || canonical.empty() ? path : canonical;
}

fs::path current_executable_path()
{
#if defined(__APPLE__)
    std::uint32_t size{1024};
    std::vector<char> path(size);
    if (_NSGetExecutablePath(path.data(), &size) != 0)
    {
        if (size == 0)
        {
            return {};
        }
        path.resize(size);
        if (_NSGetExecutablePath(path.data(), &size) != 0)
        {
            return {};
        }
    }
    return canonical_path(path.data());
#elif defined(__linux__)
    std::error_code err;
    const fs::path self{fs::read_symlink("/proc/self/exe", err)};
    return err ? fs::path{} : self;
#else
    return {};
#endif
}

std::string shell_quote(const std::string_view text)
{
    std::string result{"'"};
    for (const char ch : text)
    {
        if (ch == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += ch;
        }
    }
    result += "'";
    return result;
}

void write_error_check(std::FILE *script_file)
{
    fmt::print(script_file,
        "status=$?\n"
        "if [ \"${{status}}\" -ge 2 ]; then\n"
        "    exit \"${{status}}\"\n"
        "fi\n");
}

} // namespace

void set_make_mig_script_executable(const char *path)
{
    const fs::path self{current_executable_path()};
    if (!self.empty())
    {
        s_executable = self;
    }
    else if (path != nullptr && *path != 0)
    {
        s_executable = path;
    }
}

std::filesystem::path make_mig_script_filename()
{
    return "makemig.sh";
}

void write_make_mig_script_header(std::FILE *script_file)
{
    fmt::print(script_file,
        "#!/bin/sh\n"
        "# SPDX-License-Identifier: GPL-3.0-only\n"
        "#\n"
        "# Copyright 2026 Richard Thomson\n"
        "#\n"
        "\n"
        "id_bin={:s}\n"
        "script_dir=$(dirname \"$0\")\n"
        "cd \"${{script_dir}}\" || exit 2\n"
        "\n",
        shell_quote(s_executable.string()));
}

void write_make_mig_script_piece(
    std::FILE *script_file, const std::filesystem::path &parameter_file, const std::string &entry_name)
{
    fmt::print(script_file,
        "\"${{id_bin}}\""
        " batch=yes"
        " overwrite=yes"
        " savedir=."
        " {:s}\n",
        shell_quote(fmt::format("@{:s}/{:s}", parameter_file.string(), entry_name)));
    write_error_check(script_file);
}

void write_make_mig_script_finish(std::FILE *script_file, const int x_multiple, const int y_multiple)
{
    fmt::print(script_file,
        "\"${{id_bin}}\""
        " savedir=."
        " makemig={:d}/{:d}\n",
        x_multiple, y_multiple);
}

void finish_make_mig_script(const std::filesystem::path &script_path)
{
    std::error_code err;
    fs::permissions(script_path, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
        fs::perm_options::add, err);
}

} // namespace id::ui
