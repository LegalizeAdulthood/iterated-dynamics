// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/cmdfiles.h"
#include "ui/stop_msg.h"

#include <functional>

namespace cmd_files_test
{

// To facilitate testing
using StopMsg = bool(StopMsgFlags flags, const std::string &msg);
using StopMsgFn = std::function<StopMsg>;
StopMsgFn get_stop_msg();
void set_stop_msg(const StopMsgFn &fn);

using Goodbye = void();
using GoodbyeFn = std::function<Goodbye>;
GoodbyeFn get_goodbye();
void set_goodbye(const GoodbyeFn &fn);

using PrintDoc = void(const char *out_filename, bool (*msg_func)(int, int));
using PrintDocFn = std::function<PrintDoc>;
PrintDocFn get_print_document();
void set_print_document(const PrintDocFn &fn);

} // namespace cmd_arg
