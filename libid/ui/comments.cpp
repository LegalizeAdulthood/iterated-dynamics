// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/comments.h"

#include "engine/id_data.h"
#include "ui/get_calculation_time.h"
#include "ui/stop_msg.h"
#include "ui/video_mode.h"
#include "version.h"

#include <libcpuid/libcpuid.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <string_view>

static std::string get_cpu_id();

std::string g_command_comment[4];
char g_par_comment[4][MAX_COMMENT_LEN]{};
std::function<std::string()> g_get_cpu_id{get_cpu_id};

static std::string get_cpu_id()
{
    static std::string cpu_id;
    if (cpu_id.empty())
    {
        if (!cpuid_present())
        {
            cpu_id = "(Unknown CPU)";
        }
        auto check = [](int result, const char *label)
        {
            if (result < 0)
            {
                throw std::runtime_error(std::string{label} + ": " + cpuid_error());
            }
        };
        cpu_id_t data;
        check(cpu_identify(nullptr, &data), "cpu_identify");
        std::string brand{data.brand_str};
        auto drop = [&](const std::string_view text)
        {
            for (std::string::size_type pos = brand.find(text); pos != std::string::npos;
                 pos = brand.find(text))
            {
                brand.erase(pos, text.length());
            }
        };
        drop("(R)");
        drop("(TM)");
        cpu_id = brand;
    }
    return cpu_id;
}

static std::string_view expand_time(std::time_t local_time)
{
    return std::ctime(&local_time);
}

static std::string expand_time(std::time_t local_time, int start, int count)
{
    const std::string_view str{expand_time(local_time)};
    return std::string{str.substr(start, count)};
}

static std::string expand_var(const std::string &var, std::time_t local_time)
{
    // ctime format
    // Sat Aug 17 21:34:14 1996
    // 012345678901234567890123
    //           1         2
    if (var == "year")       // 4 chars
    {
        return expand_time(local_time, 20, 4);
    }
    if (var == "month") // 3 chars
    {
        return expand_time(local_time, 4, 3);
    }
    if (var == "day")   // 2 chars
    {
        return expand_time(local_time, 8, 2);
    }
    if (var == "hour")  // 2 chars
    {
        return expand_time(local_time, 11, 2);
    }
    if (var == "min")   // 2 chars
    {
        return expand_time(local_time, 14, 2);
    }
    if (var == "sec")   // 2 chars
    {
        return expand_time(local_time, 17, 2);
    }
    if (var == "time")  // 8 chars
    {
        return expand_time(local_time, 11, 8);
    }
    if (var == "date")
    {
        const std::string str{expand_time(local_time)};
        return str.substr(4, 6) + ", " + str.substr(20, 4);
    }
    if (var == "calctime")
    {
        return get_calculation_time(g_calc_time);
    }
    if (var == "version")  // 4 chars
    {
        return std::to_string(g_release);
    }
    if (var == "patch")   // 1 or 2 chars
    {
        return std::to_string(g_patch_level);
    }
    if (var == "xdots")   // 2 to 4 chars
    {
        return std::to_string(g_logical_screen_x_dots);
    }
    if (var == "ydots")   // 2 to 4 chars
    {
        return std::to_string(g_logical_screen_y_dots);
    }
    if (var == "vidkey")   // 2 to 3 chars
    {
        char vid_mode[5];
        vid_mode_key_name(g_video_entry.key, vid_mode);
        return vid_mode;
    }
    if (var == "cpu")
    {
        return g_get_cpu_id();
    }
    stop_msg("Unknown comment variable " + var);
    return {};
}

// extract comments from the comments= command
void parse_comments(char *value)
{
    for (char *elem : g_par_comment)
    {
        char save = '\0';
        if (*value == 0)
        {
            break;
        }
        char *next = std::strchr(value, '/');
        if (*value != '/')
        {
            if (next != nullptr)
            {
                save = *next;
                *next = '\0';
            }
            std::strncpy(elem, value, MAX_COMMENT_LEN);
        }
        if (next == nullptr)
        {
            break;
        }
        if (save != '\0')
        {
            *next = save;
        }
        value = next+1;
    }
}

// expands comments from the comments= command
static std::string expand_comments(const std::string_view source, std::time_t local_time)
{
    constexpr char QUOTE{'\\'};    // used to quote the next character (_, \ or $) from special interpretation
    constexpr char DELIMITER{'$'}; // delimits variable names
    bool in_variable{};
    std::string var_name;
    char last_c{};
    std::string target;
    for (char c : source)
    {
        if (c == QUOTE && last_c != QUOTE)
        {
            last_c = c;
            continue;
        }
        // expand underscores to blanks
        if (c == '_' && last_c != QUOTE)
        {
            c = ' ';
        }
        // DELIMITER marks start and end of variable names
        if (c == DELIMITER && last_c != QUOTE)
        {
            in_variable = !in_variable;
        }
        if (c != DELIMITER && in_variable) // if true, building variable name
        {
            var_name += c;
        }
        // got variable name
        else if (c == DELIMITER && !in_variable && last_c != QUOTE)
        {
            target += expand_var(var_name, local_time);
        }
        else if ((c != DELIMITER || last_c == QUOTE) && !in_variable)
        {
            target += c;
        }
        last_c = c == QUOTE && last_c == QUOTE ? '\0' : c;
    }
    return target;
}

const std::string &expand_command_comment(int i, std::time_t local_time)
{
    g_command_comment[i] = expand_comments(g_par_comment[i], local_time);
    return g_command_comment[i];
}

const std::string &expand_command_comment(int i)
{
    std::time_t now;
    std::time(&now);
    return expand_command_comment(i, now);    
}

void init_comments()
{
    for (char *comment : g_par_comment)
    {
        comment[0] = '\0';
    }
}
