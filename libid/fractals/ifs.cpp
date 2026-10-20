// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "fractals/ifs.h"

#include "io/file_gets.h"
#include "io/file_item.h"

#include <config/string_lower.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;

using namespace id::config;
using namespace id::io;

namespace id::fractals
{

fs::path g_ifs_filename;                     // file to find IFS in
std::string g_ifs_name;                      // Name of the IFS definition (if not empty)
std::vector<float> g_ifs_definition;         // ifs parameters
IFSDimension g_ifs_dim{IFSDimension::TWO};   // TWO or THREE
int g_num_affine_transforms{};

enum
{
    NUM_IFS_FUNCTIONS = 64
};

constexpr const char *TOKEN_SEPARATORS{" \t\n\r"};

char *get_ifs_token(char *buf, std::FILE *ifs_file)
{
    while (true)
    {
        if (file_gets(buf, 200, ifs_file) < 0)
        {
            return nullptr;
        }

        char* buf_ptr = std::strchr(buf, ';');
        if (buf_ptr != nullptr) // use ';' as comment to eol
        {
            *buf_ptr = 0;
        }
        buf_ptr = std::strtok(buf, TOKEN_SEPARATORS);
        if (buf_ptr != nullptr)
        {
            return buf_ptr;
        }
    }
}

char *get_next_ifs_token(char *buf, std::FILE *ifs_file)
{
    char *buf_ptr = std::strtok(nullptr, TOKEN_SEPARATORS);
    if (buf_ptr != nullptr)
    {
        return buf_ptr;
    }
    return get_ifs_token(buf, ifs_file);
}

void ifs_load() // read in IFS parameters
{
    g_ifs_definition.clear();
    g_ifs_dim = IFSDimension::TWO;
    g_num_affine_transforms = 0;
    std::FILE *ifs_file{};
    std::string diagnostic;
    if (find_file_item(g_ifs_filename, g_ifs_name, &ifs_file, ItemType::IFS, diagnostic))
    {
        throw std::runtime_error{diagnostic};
    }
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> file{ifs_file, std::fclose};

    char buf[201]{};
    if (file_gets(buf, 200, file.get()) < 0)
    {
        throw std::runtime_error{"invalid IFS definition"};
    }
    char *buf_ptr = std::strchr(buf, ';');
    if (buf_ptr != nullptr)   // use ';' as comment to eol
    {
        *buf_ptr = 0;
    }

    string_lower(buf);
    const int row_size = std::strstr(buf, "(3d)") != nullptr ? NUM_IFS_3D_PARAMS : NUM_IFS_2D_PARAMS;

    std::vector<float> definition;
    buf_ptr = get_ifs_token(buf, file.get());
    while (buf_ptr != nullptr && std::strcmp(buf_ptr, "}") != 0)
    {
        char *end{};
        const float value{std::strtof(buf_ptr, &end)};
        if (end == buf_ptr || *end != 0 || !std::isfinite(value))
        {
            throw std::runtime_error{"invalid IFS definition"};
        }
        definition.push_back(value);
        if (definition.size() >= static_cast<std::size_t>(NUM_IFS_FUNCTIONS * row_size))
        {
            throw std::runtime_error{"IFS definition has too many lines"};
        }
        buf_ptr = get_next_ifs_token(buf, file.get());
    }

    if (buf_ptr == nullptr || definition.size() % row_size != 0)
    {
        throw std::runtime_error{"invalid IFS definition"};
    }
    if (definition.empty())
    {
        throw std::runtime_error{"Empty IFS definition"};
    }
    g_num_affine_transforms = static_cast<int>(definition.size()) / row_size;
    g_ifs_dim = row_size == NUM_IFS_3D_PARAMS ? IFSDimension::THREE : IFSDimension::TWO;
    g_ifs_definition = std::move(definition);
}

} // namespace id::fractals
