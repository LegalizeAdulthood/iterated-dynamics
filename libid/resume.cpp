// SPDX-License-Identifier: GPL-3.0-only
//
#include "resume.h"

#include "port.h"

#include "id_data.h"

#include <algorithm>
#include <cstdarg>
#include <vector>

static int s_resume_offset{}; // offset in resume info gets

std::vector<BYTE> g_resume_data; // resume info
bool g_resuming{};               // true if resuming after interrupt
int g_resume_len{};              // length of resume info

int put_resume(int len, ...)
{
    std::va_list arg_marker;

    if (g_resume_data.empty())
    {
        return -1;
    }

    va_start(arg_marker, len);
    while (len)
    {
        BYTE const *source_ptr = va_arg(arg_marker, BYTE *);
        std::copy(&source_ptr[0], &source_ptr[len], &g_resume_data[g_resume_len]);
        g_resume_len += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return 0;
}

int alloc_resume(int max_size, int version)
{
    g_resume_data.clear();
    g_resume_data.resize(sizeof(int)*max_size);
    g_resume_len = 0;
    put_resume_t(version);
    g_calc_status = CalcStatus::RESUMABLE;
    return 0;
}

int get_resume(int len, ...)
{
    std::va_list arg_marker;

    if (g_resume_data.empty())
    {
        return -1;
    }
    va_start(arg_marker, len);
    while (len)
    {
        BYTE *dest_ptr = va_arg(arg_marker, BYTE *);
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
    get_resume_t(version);
    return version;
}

void end_resume()
{
    g_resume_data.clear();
}
