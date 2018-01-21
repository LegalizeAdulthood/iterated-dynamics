#pragma once
#ifndef BIGINIT_H
#define BIGINIT_H

#include "big.h"

// biginit -- C file prototypes
void free_bf_vars();
bn_t alloc_stack(size_t size);
int save_stack();
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi();

#endif
