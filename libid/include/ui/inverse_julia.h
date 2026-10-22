// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/jiim.h"
#include "ui/KeyboardHandler.h"

#include <memory>

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class CrossHairCursor;

class InverseJuliaKeyboardContext
{
public:
    virtual ~InverseJuliaKeyboardContext() = default;

    virtual engine::JIIMType which() const = 0;
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

class InverseJuliaMouseScope
{
public:
    explicit InverseJuliaMouseScope(CrossHairCursor &cursor);
    ~InverseJuliaMouseScope();

    InverseJuliaMouseScope(const InverseJuliaMouseScope &) = delete;
    InverseJuliaMouseScope(InverseJuliaMouseScope &&) = delete;
    InverseJuliaMouseScope &operator=(const InverseJuliaMouseScope &) = delete;
    InverseJuliaMouseScope &operator=(InverseJuliaMouseScope &&) = delete;

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
};

void jiim(engine::JIIMType which);
bool prepare_inverse_julia_queue(unsigned long request);
int inverse_julia_fractal_type(engine::NonStandardFractal &calculation);
KeyboardHandlerPtr make_inverse_julia_keyboard_handler(InverseJuliaKeyboardContext &context);

} // namespace id::ui
