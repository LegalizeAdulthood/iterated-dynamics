// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "HelpCompiler.h"

namespace hc
{

class AsciiDocCompiler : public HelpCompiler
{
public:
    explicit AsciiDocCompiler(const Options &options) : HelpCompiler(options)
    {
    }
    ~AsciiDocCompiler() override = default;

    int process() override;

private:
    void paginate_ascii_doc();
    void print_ascii_doc();
};

} // namespace hc
