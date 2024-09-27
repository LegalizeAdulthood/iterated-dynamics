// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "options.h"

#include <memory>

namespace hc
{

class Compiler
{
public:
    virtual ~Compiler() = default;

    virtual int process() = 0;
};

std::shared_ptr<Compiler> create_compiler(const Options &options);

} // namespace hc
