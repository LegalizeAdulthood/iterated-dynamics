// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/file_item.h"

#include "engine/cmdfiles.h"
#include "io/library.h"
#include "misc/id.h"
#include "ui/stop_msg.h"

#include <config/string_case_compare.h>
#include <fmt/format.h>

using namespace id::config;
using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

constexpr int MAX_ENTRIES = 2000;

static bool is_newline(const int c)
{
    return c == '\n' || c == '\r';
}

// skip to next non-white space character and return it
static int skip_white_space(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = std::getc(infile);
        (*file_offset)++;
    }
    while (c == ' ' || c == '\t' || is_newline(c));
    return c;
}

// skip to end of line
static int skip_comment(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = std::getc(infile);
        (*file_offset)++;
    } while (!is_newline(c) && c != EOF);
    return c;
}

static int scan_entries(std::FILE *infile, FileEntry *choices, const char *item_name)
{
    // returns the number of entries found; if a
    // specific entry is being looked for, returns -1 if
    // the entry is found, 0 otherwise.
    char buf[101];
    int exclude_entry;
    long file_offset = -1;
    int num_entries = 0;

    while (true)
    {
        // scan the file for entry names
top:
        int c = skip_white_space(infile, &file_offset);
        if (c == ';')
        {
            c = skip_comment(infile, &file_offset);
            if (c == EOF)
            {
                break;
            }
            continue;
        }
        long temp_offset = file_offset;
        const long name_offset = file_offset;
        // next equiv roughly to fscanf(..,"%40[^* \n\r\t({\032]",buf)
        int len = 0;
        // allow spaces in entry names in next
        while (c != ' ' && c != '\t' && c != '(' && c != ';' //
            && c != '{' && !is_newline(c) && c != EOF)
        {
            if (len < 40)
            {
                buf[len++] = static_cast<char>(c);
            }
            c = std::getc(infile);
            ++file_offset;
        }
        if (is_newline(c))
        {
            continue;
        }
        buf[len] = 0;
        while (c != '{' && !is_newline(c) && c != EOF)
        {
            if (c == ';')
            {
                c = skip_comment(infile, &file_offset);
            }
            else
            {
                c = std::getc(infile);
                ++file_offset;
            }
        }
        if (is_newline(c))
        {
            continue;
        }
        if (c == '{')
        {
            while (c != '}' && c != EOF)
            {
                if (c == ';')
                {
                    c = skip_comment(infile, &file_offset);
                }
                else
                {
                    if (is_newline(c))       // reset temp_offset to
                    {
                        temp_offset = file_offset;  // beginning of new line
                    }
                    c = std::getc(infile);
                    ++file_offset;
                }
                if (c == '{') //second '{' found
                {
                    if (temp_offset == name_offset) // if on same line, skip line
                    {
                        skip_comment(infile, &file_offset);
                        goto top;
                    }
                    std::fseek(infile, temp_offset, SEEK_SET); // else, go back to
                    file_offset = temp_offset - 1;             // beginning of line
                    goto top;
                }
            }
            if (c != '}')     // i.e. is EOF
            {
                break;
            }

            if (string_case_equal(buf, "frm:", 4)    //
                || string_case_equal(buf, "ifs:", 4) //
                || string_case_equal(buf, "par:", 4))
            {
                exclude_entry = 4;
            }
            else if (string_case_equal(buf, "lsys:", 5))
            {
                exclude_entry = 5;
            }
            else
            {
                exclude_entry = 0;
            }

            buf[ITEM_NAME_LEN + exclude_entry] = 0;
            if (item_name != nullptr)  // looking for one entry
            {
                if (string_case_equal(buf, item_name))
                {
                    std::fseek(infile, name_offset + static_cast<long>(exclude_entry), SEEK_SET);
                    return -1;
                }
            }
            else // make a whole list of entries
            {
                if (buf[0] != 0 && !string_case_equal(buf, "comment") && !exclude_entry)
                {
                    std::strcpy(choices[num_entries].name, buf);
                    choices[num_entries].point = name_offset;
                    if (++num_entries >= MAX_ENTRIES)
                    {
                        stop_msg(fmt::format("Too many entries in file, first {:d} used", MAX_ENTRIES));
                        break;
                    }
                }
            }
        }
        else if (c == EOF)
        {
            break;
        }
    }
    return num_entries;
}

int scan_entries(std::FILE *infile, FileEntry *choices)
{
    return scan_entries(infile, choices, nullptr);
}

bool search_for_entry(std::FILE *infile, const std::string &item_name)
{
    return scan_entries(infile, nullptr, item_name.c_str()) == -1;
}

static bool check_path(const std::filesystem::path &path, std::FILE **infile, const std::string &item_name)
{
    if (std::FILE *f = std::fopen(path.string().c_str(), "rb"); f != nullptr)
    {
        if (search_for_entry(f, item_name))
        {
            *infile = f;
            return true;
        }

        std::fclose(f);
        *infile = nullptr;
    }

    return false;
}

bool find_file_item(
    std::filesystem::path &path, const std::string &item_name, std::FILE **file_ptr, const ItemType item_type)
{
    std::FILE *infile = nullptr;
    bool found = false;

    if (!string_case_equal(path.string().c_str(), g_parameter_file.string().c_str()))
    {
        found = check_path(path, &infile, item_name);

        if (!found && g_check_cur_dir)
        {
            const std::filesystem::path full_path{path.filename()};
            found = check_path(full_path, &infile, item_name);
            if (found)
            {
                path = full_path;
            }
        }
    }

    if (!found)
    {
        std::string par_search_name;
        switch (item_type)
        {
        case ItemType::FORMULA:
            par_search_name = "frm:" + item_name;
            break;
        case ItemType::L_SYSTEM:
            par_search_name = "lsys:" + item_name;
            break;
        case ItemType::IFS:
            par_search_name = "ifs:" + item_name;
            break;
        case ItemType::PAR_SET:
        default:
            par_search_name = item_name;
            break;
        }
        found = check_path(g_parameter_file, &infile, par_search_name);
        if (found)
        {
            path = g_parameter_file;
        }
    }

    if (!found)
    {
        const auto read_file = [](ItemType type)
        {
            switch (type)
            {
            case ItemType::FORMULA:
                return ReadFile::FORMULA;
            case ItemType::L_SYSTEM:
                return ReadFile::LSYSTEM;
            case ItemType::IFS:
                return ReadFile::IFS;
            case ItemType::PAR_SET:
                return ReadFile::PARAMETER;
            }
            throw std::runtime_error("Unknown ItemType " + std::to_string(static_cast<int>(type)));
        };
        const std::filesystem::path lib_path{find_file(read_file(item_type), path.filename())};
        if (!lib_path.empty())
        {
            found = check_path(lib_path, &infile, item_name);
        }
        if (found)
        {
            path = lib_path;
        }
    }

    if (!found)
    {
        stop_msg(fmt::format("'{:s}' file entry item not found", item_name));
        return true;
    }

    // found file
    if (file_ptr != nullptr)
    {
        *file_ptr = infile;
    }
    else if (infile != nullptr)
    {
        std::fclose(infile);
    }
    return false;
}

} // namespace id::ui
