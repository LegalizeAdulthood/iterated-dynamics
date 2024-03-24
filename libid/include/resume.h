#pragma once

#include "port.h"

#include <vector>

extern std::vector<BYTE>     g_resume_data;
extern bool                  g_resuming;
extern int                   g_resume_len;

int put_resume(int, ...);
int get_resume(int, ...);
int alloc_resume(int, int);
int start_resume();
void end_resume();
