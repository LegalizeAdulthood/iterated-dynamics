// SPDX-License-Identifier: GPL-3.0-only
//
#include "messages.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace hc
{

bool g_quiet_mode{};                 // true if "/Q" option used
int g_errors{};                      // number of errors reported
int g_warnings{};                    // number of warnings reported
int g_src_line{};                    // .SRC line number (used for errors)
std::string g_current_src_filename;  // current .SRC filename

/*
 * error/warning/message reporting functions.
 */
void report_errors()
{
    std::printf("\n");
    std::printf("Compiler Status:\n");
    std::printf("%8d Error%c\n",       g_errors, (g_errors == 1)   ? ' ' : 's');
    std::printf("%8d Warning%c\n",     g_warnings, (g_warnings == 1) ? ' ' : 's');
}


void print_msg(char const *type, int lnum, char const *format, std::va_list arg)
{
    if (type != nullptr)
    {
        std::printf("   %s", type);
        if (lnum > 0)
        {
            std::printf(" %s %d", g_current_src_filename.c_str(), lnum);
        }
        std::printf(": ");
    }
    vprintf(format, arg);
    std::printf("\n");
    std::fflush(stdout);
}

void fatal_msg(int diff, char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);

    print_msg("Fatal", g_src_line-diff, format, arg);
    va_end(arg);

    if (g_errors || g_warnings)
    {
        report_errors();
    }

    std::exit(g_errors + 1);
}

void error_msg(int diff, char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);

    print_msg("Error", g_src_line-diff, format, arg);
    va_end(arg);

    if (++g_errors >= MAX_ERRORS)
    {
        fatal_msg(0, "Too many errors!");
    }
}

void warn_msg(int diff, char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);

    print_msg("Warning", g_src_line-diff, format, arg);
    va_end(arg);

    if (++g_warnings >= MAX_WARNINGS)
    {
        fatal_msg(0, "Too many warnings!");
    }
}

void notice_msg(char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);
    print_msg("Note", g_src_line, format, arg);
    va_end(arg);
}

void msg_msg(char const *format, ...)
{
    std::va_list arg;

    if (g_quiet_mode)
    {
        return;
    }
    va_start(arg, format);
    print_msg(nullptr, 0, format, arg);
    va_end(arg);
}

void show_line(unsigned int line)
{
    std::printf("[%04u] ", line);
}

} // namespace hc
