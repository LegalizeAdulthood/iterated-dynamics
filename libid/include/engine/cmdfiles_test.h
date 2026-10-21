// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/cmdfiles.h"

#include <functional>

namespace id::engine::cmd_files_test
{

// To facilitate testing
// TODO: removed later
// Slice 45 moves command-generation actions and their exit decisions to UI.
using Goodbye = void();
using GoodbyeFn = std::function<Goodbye>;
GoodbyeFn get_goodbye();
void set_goodbye(const GoodbyeFn &fn);

using PrintDoc = void(const char *out_filename, bool (*msg_func)(int, int));
using PrintDocFn = std::function<PrintDoc>;
PrintDocFn get_print_document();
void set_print_document(const PrintDocFn &fn);

} // namespace id::engine::cmd_files_test
