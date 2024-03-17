#pragma once

#include "big.h"

#include <stddef.h>

extern int                   g_bf_save_len;
extern long                  g_bignum_max_stack_addr;

void free_bf_vars();
bn_t alloc_stack(size_t size);
int save_stack();
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi();
