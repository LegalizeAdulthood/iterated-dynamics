// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::engine
{

enum class JIIMType
{
    JIIM = 0,
    ORBIT
};

constexpr double JULIA_C_NOT_SET{100000.0}; //

extern bool g_has_inverse;                  //
extern math::DComplex g_julia_c;            //
extern math::DComplex g_save_c;             //

class InverseJuliaKeyboardContext
{
public:
    virtual ~InverseJuliaKeyboardContext() = default;

    virtual JIIMType which() const = 0;
    virtual void begin_key_batch() = 0;
    virtual bool leaving_now() const = 0;
    virtual void set_last_key(int key) = 0;
    virtual void leave_now() = 0;
    virtual void leave_after_refresh() = 0;
    virtual bool continuing() const = 0;
    virtual void reset_julia_selection() = 0;
    virtual void select_julia_from_cursor() = 0;
    virtual void reset_zoom() = 0;
    virtual void zoom(float factor) = 0;
    virtual void toggle_circle_mode() = 0;
    virtual void toggle_line_mode() = 0;
    virtual void toggle_numbers() = 0;
    virtual void set_exact_point(math::DComplex point) = 0;
    virtual void toggle_hidden_fractal() = 0;
    virtual void set_secret_mode(int mode) = 0;
    virtual void move_cursor(int d_col, int d_row) = 0;
};

void jiim(JIIMType which);
math::DComplex pop_float();
math::DComplex dequeue_float();
bool init_queue(unsigned long request);
void free_queue();
void   clear_queue();
int    queue_empty();
int    queue_full();
int    queue_full_almost();
int    push_float(float x, float y);
int    enqueue_float(float x, float y);

} // namespace id::engine
