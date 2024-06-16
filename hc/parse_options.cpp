#include "compiler.h"

#include <stdexcept>

namespace hc {

compiler_options parse_options(int argc, char **argv)
{
    compiler_options result{};
    for (int i = 1; i < argc; ++i)
    {
        std::string arg{argv[i]};
        if (arg[0] == '/' || arg[0] == '-')
        {
            arg.erase(0, 1); // drop '/' or '-'
            if (arg == "a")
            {
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::APPEND;
                }
                else
                {
                    throw std::runtime_error("Cannot have /a with /adoc, /c, /d, /h or /p");
                }
            }
            else if (arg == "adoc")
            {
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::ASCII_DOC;
                }
                else
                {
                    throw std::runtime_error("Cannot have /adoc with /a, /c, /d, /h or /p");
                }
            }
            else if (arg == "c")
            {
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::COMPILE;
                }
                else
                {
                    throw std::runtime_error("Cannot have /c with /a, /adoc, /d, /h or /p");
                }
            }
            else if (arg == "d")
            {
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::DELETE;
                }
                else
                {
                    throw std::runtime_error("Cannot have /d with /a, /adoc, /c, /h or /p");
                }
            }
            else if (arg == "h")
            {
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::HTML;
                }
                else
                {
                    throw std::runtime_error("Cannot have /h with /a, /adoc, /c, /d or /p");
                }
            }
            else if (arg == "i")
            {
                if (i < argc - 1)
                {
                    result.include_paths.emplace_back(argv[i + 1]);
                    ++i;
                }
                else
                {
                    throw std::runtime_error("Missing argument for /i");
                }
            }
            else if (arg == "m")
            {
                if (result.mode == modes::COMPILE)
                {
                    result.show_mem = true;
                }
                else
                {
                    throw std::runtime_error("/m switch allowed only when compiling (/c)");
                }
            }
            else if (arg == "o")
            {
                std::string output_dir;
                if (i < argc - 1)
                {
                    output_dir = argv[i + 1];
                    ++i;
                }
                else
                {
                    throw std::runtime_error("Missing argument for /o");
                }
                if (result.mode == modes::HTML || result.mode == modes::ASCII_DOC)
                {
                    result.output_dir = output_dir;
                }
                else
                {
                    throw std::runtime_error("/o switch allowed only when writing HTML (/h) or ASCII doc (/adoc)");
                }
            }
            else if (arg == "p")
            {
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::PRINT;
                }
                else
                {
                    throw std::runtime_error("Cannot have /p with /a, /adoc, /c, /h or /d");
                }
            }
            else if (arg == "s")
            {
                if (result.mode == modes::COMPILE)
                {
                    result.show_stats = true;
                }
                else
                {
                    throw std::runtime_error("/s switch allowed only when compiling (/c)");
                }
            }
            else if (arg == "r")
            {
                std::string swap_path;
                if (i < argc - 1)
                {
                    swap_path = argv[i + 1];
                    ++i;
                }
                else
                {
                    throw std::runtime_error("Missing argument for /r");
                }
                if (result.mode == modes::COMPILE || result.mode == modes::PRINT)
                {
                    result.swappath = swap_path;
                }
                else
                {
                    throw std::runtime_error("/r switch allowed when compiling (/c) or printing (/p)");
                }
            }
            else if (arg == "q")
            {
                result.quiet_mode = true;
            }
            else
            {
                throw std::runtime_error("Bad command-line switch /" + arg);
            }
        }
        else
        {
            // assume it is a filename
            if (result.fname1.empty())
            {
                result.fname1 = arg;
            }
            else if (result.fname2.empty())
            {
                result.fname2 = arg;
            }
            else
            {
                throw std::runtime_error("Unexpected command-line argument \" + arg + \"");
            }
        }
    }
    return result;
}

} // namespace hc
