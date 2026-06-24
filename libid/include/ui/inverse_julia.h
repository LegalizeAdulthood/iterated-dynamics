// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/KeyboardHandler.h"

namespace id::engine
{
class InverseJuliaKeyboardContext;
}

namespace id::ui
{

class CrossHairCursor;

int inverse_julia_fractal_type();
KeyboardHandlerPtr make_inverse_julia_keyboard_handler(engine::InverseJuliaKeyboardContext &context);
bool process_inverse_julia_keyboard(
    engine::InverseJuliaKeyboardContext &context, CrossHairCursor &cursor, bool wait_for_key);
void requeue_inverse_julia_key(int key);

} // namespace id::ui
