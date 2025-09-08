// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/big.h"

#include <stddef.h>

namespace id::math
{

extern int g_bf_save_len;
extern long g_bignum_max_stack_addr;
extern long g_max_stack;
extern long g_start_stack;

void free_bf_vars();
BigNum alloc_stack(size_t size);
int save_stack();
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi();

class BigStackSaver
{
public:
    BigStackSaver() :
        m_offset(save_stack())
    {
    }
    BigStackSaver(const BigStackSaver &rhs) = delete;
    BigStackSaver(BigStackSaver &&rhs) = delete;
    ~BigStackSaver()
    {
        restore_stack(m_offset);
    }
    BigStackSaver &operator=(const BigStackSaver &rhs) = delete;
    BigStackSaver &operator=(BigStackSaver &&rhs) = delete;

private:
    int m_offset;
};

} // namespace id::math
