#pragma once

#include "help_compiler.h"

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
    void print_ascii_doc();
};

} // namespace hc
