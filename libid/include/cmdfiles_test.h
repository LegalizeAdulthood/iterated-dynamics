#pragma once

#include "cmdfiles.h"

#include <functional>

namespace cmd_arg
{

// To facilitate testing
using StopMsg = bool(int flags, const std::string &msg);
using StopMsgFn = std::function<StopMsg>;
StopMsgFn get_stop_msg();
void set_stop_msg(const StopMsgFn &fn);

using Goodbye = void();
using GoodbyeFn = std::function<Goodbye>;
GoodbyeFn get_goodbye();
void set_goodbye(const GoodbyeFn &fn);

using PrintDoc = void(char const *outfname, bool (*msg_func)(int, int));
using PrintDocFn = std::function<PrintDoc>;
PrintDocFn get_print_document();
void set_print_document(const PrintDocFn &fn);

} // namespace cmd_arg
