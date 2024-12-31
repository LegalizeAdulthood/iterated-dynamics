// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>

namespace hc
{

struct Options;

class Compiler
{
public:
    virtual ~Compiler() = default;

    virtual int process() = 0;
};

std::shared_ptr<Compiler> create_compiler(const Options &options);

} // namespace hc
