#include "compiler.h"
#include "adoc_compiler.h"
#include "help_compiler.h"

namespace hc
{

std::shared_ptr<Compiler> create_compiler(const Options &options)
{
    switch (options.mode)
    {
    case modes::ASCII_DOC:
        return std::make_shared<AsciiDocCompiler>(options);
    default:
        return std::make_shared<HelpCompiler>(options);
    }
}

} // namespace hc
