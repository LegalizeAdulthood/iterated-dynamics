// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/resume.h"

#include "engine/calcfrac.h"

#include <algorithm>
#include <cstdarg>

namespace id::engine
{

static int s_resume_offset{}; // offset in resume info gets

std::vector<Byte> g_resume_data; // resume info
bool g_resuming{};               // true if resuming after interrupt
int g_resume_len{};              // length of resume info

int put_resume_len(int len, ...)
{
    std::va_list arg_marker;

    if (g_resume_data.empty())
    {
        return -1;
    }

    va_start(arg_marker, len);
    while (len)
    {
        const Byte *source_ptr = va_arg(arg_marker, Byte *);
        std::copy(&source_ptr[0], &source_ptr[len], &g_resume_data[g_resume_len]);
        g_resume_len += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return 0;
}

int alloc_resume(const int max_size, const int version)
{
    g_resume_data.clear();
    g_resume_data.resize(sizeof(int)*max_size);
    g_resume_len = 0;
    put_resume(version);
    g_calc_status = CalcStatus::RESUMABLE;
    return 0;
}

int get_resume_len(int len, ...)
{
    std::va_list arg_marker;

    if (g_resume_data.empty())
    {
        return -1;
    }
    va_start(arg_marker, len);
    while (len)
    {
        Byte *dest_ptr = va_arg(arg_marker, Byte *);
        std::copy(&g_resume_data[s_resume_offset], &g_resume_data[s_resume_offset + len], &dest_ptr[0]);
        s_resume_offset += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return 0;
}

int start_resume()
{
    int version;
    if (g_resume_data.empty())
    {
        return -1;
    }
    s_resume_offset = 0;
    get_resume(version);
    return version;
}

void end_resume()
{
    g_resume_data.clear();
}

} // namespace id::engine
