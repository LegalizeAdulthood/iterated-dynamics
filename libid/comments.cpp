#include "comments.h"

#include "get_calculation_time.h"
#include "id_data.h"
#include "stop_msg.h"
#include "version.h"
#include "video_mode.h"

#include <libcpuid/libcpuid.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>

static std::string get_cpu_id();

std::string g_command_comment[4];
char g_par_comment[4][MAX_COMMENT_LEN]{};
std::function<std::string()> g_get_cpu_id{get_cpu_id};

constexpr char ESC{'$'};

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

static char const *expand_var(char const *var, char *buf)
{
    std::time_t ltime;
    char *str;
    char const *out;

    std::time(&ltime);
    str = std::ctime(&ltime);

    // ctime format
    // Sat Aug 17 21:34:14 1996
    // 012345678901234567890123
    //           1         2
    if (std::strcmp(var, "year") == 0)       // 4 chars
    {
        str[24] = '\0';
        out = &str[20];
    }
    else if (std::strcmp(var, "month") == 0) // 3 chars
    {
        str[7] = '\0';
        out = &str[4];
    }
    else if (std::strcmp(var, "day") == 0)   // 2 chars
    {
        str[10] = '\0';
        out = &str[8];
    }
    else if (std::strcmp(var, "hour") == 0)  // 2 chars
    {
        str[13] = '\0';
        out = &str[11];
    }
    else if (std::strcmp(var, "min") == 0)   // 2 chars
    {
        str[16] = '\0';
        out = &str[14];
    }
    else if (std::strcmp(var, "sec") == 0)   // 2 chars
    {
        str[19] = '\0';
        out = &str[17];
    }
    else if (std::strcmp(var, "time") == 0)  // 8 chars
    {
        str[19] = '\0';
        out = &str[11];
    }
    else if (std::strcmp(var, "date") == 0)
    {
        str[10] = '\0';
        str[24] = '\0';
        char *dest = &str[4];
        std::strcat(dest, ", ");
        std::strcat(dest, &str[20]);
        out = dest;
    }
    else if (std::strcmp(var, "calctime") == 0)
    {
        strcpy(buf, get_calculation_time(g_calc_time).c_str());
        out = buf;
    }
    else if (std::strcmp(var, "version") == 0)  // 4 chars
    {
        std::sprintf(buf, "%d", g_release);
        out = buf;
    }
    else if (std::strcmp(var, "patch") == 0)   // 1 or 2 chars
    {
        std::sprintf(buf, "%d", g_patch_level);
        out = buf;
    }
    else if (std::strcmp(var, "xdots") == 0)   // 2 to 4 chars
    {
        std::sprintf(buf, "%d", g_logical_screen_x_dots);
        out = buf;
    }
    else if (std::strcmp(var, "ydots") == 0)   // 2 to 4 chars
    {
        std::sprintf(buf, "%d", g_logical_screen_y_dots);
        out = buf;
    }
    else if (std::strcmp(var, "vidkey") == 0)   // 2 to 3 chars
    {
        char vidmde[5];
        vidmode_keyname(g_video_entry.keynum, vidmde);
        std::sprintf(buf, "%s", vidmde);
        out = buf;
    }
    else if (std::strcmp(var, "cpu") == 0)
    {
        std::strcpy(buf, g_get_cpu_id().c_str());
        out = buf;
    }
    else
    {
        char buff[80];
        std::snprintf(buff, std::size(buff), "Unknown comment variable %s", var);
        stopmsg(buff);
        out = "";
    }
    return out;
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

enum
{
    MAXVNAME = 13
};

// extract comments from the comments= command
static std::string expand_comments(char const *source)
{
    int escape = 0;
    char c;
    char oldc;
    char varname[MAXVNAME];
    int k = 0;
    int i = 0;
    oldc = 0;
    std::string target;
    while (i < MAX_COMMENT_LEN && (c = *(source + i++)) != '\0')
    {
        if (c == '\\' && oldc != '\\')
        {
            oldc = c;
            continue;
        }
        // expand underscores to blanks
        if (c == '_' && oldc != '\\')
        {
            c = ' ';
        }
        // esc_char marks start and end of variable names
        if (c == ESC && oldc != '\\')
        {
            escape = 1 - escape;
        }
        if (c != ESC && escape != 0) // if true, building variable name
        {
            if (k < MAXVNAME-1)
            {
                varname[k++] = c;
            }
        }
        // got variable name
        else if (c == ESC && escape == 0 && oldc != '\\')
        {
            char buf[100];
            varname[k] = 0;
            target += expand_var(varname, buf);
        }
        else if (c == ESC && escape != 0 && oldc != '\\')
        {
            k = 0;
        }
        else if ((c != ESC || oldc == '\\') && escape == 0)
        {
            target += c;
        }
        oldc = c;
    }
    return target;
}

std::string expand_command_comment(int i)
{
    g_command_comment[i] = expand_comments(g_par_comment[i]);
    return g_command_comment[i];
}

void init_comments()
{
    for (char *elem : g_par_comment)
    {
        elem[0] = '\0';
    }
}
