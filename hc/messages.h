// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

/*
 * When defined, SHOW_ERROR_LINE will cause the line number in HC.C where
 * errors/warnings/messages are generated to be displayed at the start of
 * the line.
 *
 * Used when debugging HC.  Also, useful for finding the line (in HC.C) that
 * generated an error or warning.
 */
#define SHOW_ERROR_LINE

#ifdef SHOW_ERROR_LINE
#   define MSG_ERROR(...)  (show_line(__LINE__), error_msg(__VA_ARGS__))
#   define MSG_WARN(...)   (show_line(__LINE__), warn_msg(__VA_ARGS__))
#   define MSG_NOTICE(...) (show_line(__LINE__), notice_msg(__VA_ARGS__))
#   define MSG_MSG(...)    (g_quiet_mode ? static_cast<void>(0) : (show_line(__LINE__), msg_msg(__VA_ARGS__)))
#else
#define MSG_ERROR(...)  error_msg(__VA_ARGS__)
#define MSG_WARN(...)   warn_msg(__VA_ARGS__)
#define MSG_NOTICE(...) notice_msg(__VA_ARGS__)
#define MSG_MSG(...)    msg_msg(__VA_ARGS__)
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
void error_msg(int line_offset, const char *format, ...);
void warn_msg(int line_offset, const char *format, ...);
void notice_msg(const char *format, ...);
void msg_msg(const char *format, ...);

}
