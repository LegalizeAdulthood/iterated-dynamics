// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

/*
 * When defined, SHOW_ERROR_LINE will cause the line number in HC.C where
 * errors/warnings/messages are generated to be displayed at the start of
 * the line.
 *
 * Used when debugging HC.  Also useful for finding the line (in HC.C) that
 * generated a error or warning.
 */
#define SHOW_ERROR_LINE

#ifdef SHOW_ERROR_LINE
#   define error(...)  (show_line(__LINE__), error_msg(__VA_ARGS__))
#   define warn(...)   (show_line(__LINE__), warn_msg(__VA_ARGS__))
#   define notice(...) (show_line(__LINE__), notice_msg(__VA_ARGS__))
#   define msg(...)    (g_quiet_mode ? static_cast<void>(0) : (show_line(__LINE__), msg_msg(__VA_ARGS__)))
#else
#define error(...)  error_msg(__VA_ARGS__)
#define warn(...)   warn_msg(__VA_ARGS__)
#define notice(...) notice_msg(__VA_ARGS__)
#define msg(...)    msg_msg(__VA_ARGS__)
#endif

namespace hc
{

constexpr int MAX_ERRORS{25};   // stop after this many errors
constexpr int MAX_WARNINGS{25}; // stop after this many warnings, 0 = never stop

extern bool g_quiet_mode; // don't report line numbers in messages
extern int g_errors;      // number of errors reported
extern int g_warnings;    // number of warnings reported

extern int g_src_line;
extern std::string g_current_src_filename;

void report_errors();
void show_line(unsigned int line);
void error_msg(int diff, char const *format, ...);
void warn_msg(int diff, char const *format, ...);
void notice_msg(char const *format, ...);
void msg_msg(char const *format, ...);

}
