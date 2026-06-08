// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace id::io
{

namespace
{

namespace fs = std::filesystem;

constexpr std::string_view DOCUMENTS_KEY{"XDG_DOCUMENTS_DIR"};
constexpr const char *PROC_SELF_EXE{"/proc/self/exe"};
constexpr const char *USER_DIRS_FILE{"user-dirs.dirs"};

class X11SpecialDirectories : public SpecialDirectories
{
public:
    ~X11SpecialDirectories() override = default;

    std::filesystem::path program_dir() const override;
    std::filesystem::path documents_dir() const override;
};

bool is_space(const char ch)
{
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::string_view trim(std::string_view text)
{
    while (!text.empty() && is_space(text.front()))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_space(text.back()))
    {
        text.remove_suffix(1);
    }
    return text;
}

std::string_view unquote(std::string_view text)
{
    text = trim(text);
    if (text.size() >= 2 &&
        ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\'')))
    {
        text.remove_prefix(1);
        text.remove_suffix(1);
    }
    return text;
}

fs::path home_dir()
{
    const char *home = std::getenv("HOME");
    return home != nullptr && home[0] != '\0' ? fs::path{home} : fs::path{};
}

fs::path xdg_config_home()
{
    if (const char *config_home = std::getenv("XDG_CONFIG_HOME"); config_home != nullptr && config_home[0] != '\0')
    {
        return config_home;
    }

    const fs::path home{home_dir()};
    return home.empty() ? fs::path{} : home / ".config";
}

bool starts_with(std::string_view text, const std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

fs::path expand_user_dir(std::string_view value)
{
    value = unquote(value);
    const fs::path home{home_dir()};
    if (value == "$HOME" || value == "${HOME}")
    {
        return home;
    }
    if (starts_with(value, "$HOME/"))
    {
        return home / std::string{value.substr(6)};
    }
    if (starts_with(value, "${HOME}/"))
    {
        return home / std::string{value.substr(8)};
    }

    const fs::path path{std::string{value}};
    if (path.is_absolute() || home.empty())
    {
        return path;
    }
    return home / path;
}

std::optional<fs::path> read_user_documents_dir()
{
    const fs::path config_home{xdg_config_home()};
    if (config_home.empty())
    {
        return {};
    }

    std::ifstream user_dirs{config_home / USER_DIRS_FILE};
    std::string line;
    while (std::getline(user_dirs, line))
    {
        const std::string_view text{trim(line)};
        if (text.empty() || text.front() == '#')
        {
            continue;
        }
        constexpr std::string_view SEPARATOR{"="};
        const std::size_t separator{text.find(SEPARATOR)};
        if (separator == std::string_view::npos || trim(text.substr(0, separator)) != DOCUMENTS_KEY)
        {
            continue;
        }
        const fs::path path{expand_user_dir(text.substr(separator + SEPARATOR.size()))};
        if (!path.empty())
        {
            return path;
        }
    }
    return {};
}

std::filesystem::path X11SpecialDirectories::program_dir() const
{
    std::error_code err;
    const fs::path executable{fs::read_symlink(PROC_SELF_EXE, err)};
    if (!err && !executable.empty())
    {
        return executable.parent_path();
    }
    return fs::current_path();
}

std::filesystem::path X11SpecialDirectories::documents_dir() const
{
    if (const char *documents_dir = std::getenv("XDG_DOCUMENTS_DIR");
        documents_dir != nullptr && documents_dir[0] != '\0')
    {
        return expand_user_dir(documents_dir);
    }
    if (const std::optional<fs::path> documents_dir{read_user_documents_dir()}; documents_dir.has_value())
    {
        return *documents_dir;
    }

    const fs::path home{home_dir()};
    return home.empty() ? fs::current_path() : home / "Documents";
}

} // namespace

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<X11SpecialDirectories>();
}

} // namespace id::io
