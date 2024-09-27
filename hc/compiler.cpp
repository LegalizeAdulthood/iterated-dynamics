// SPDX-License-Identifier: GPL-3.0-only
//
#include "compiler.h"
#include "adoc_compiler.h"
#include "help_compiler.h"

namespace hc
{

std::shared_ptr<Compiler> create_compiler(const Options &options)
{
    return options.mode == modes::ASCII_DOC ? std::make_shared<AsciiDocCompiler>(options)
                                            : std::make_shared<HelpCompiler>(options);
}

} // namespace hc
