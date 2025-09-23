// SPDX-License-Identifier: GPL-3.0-only
//
#include "Options.h"

#include <stdexcept>

namespace hc
{

Options parse_options(int argc, char **argv)
{
    Options result{};
    for (int i = 1; i < argc; ++i)
    {
        if (std::string arg{argv[i]}; arg[0] == '/' || arg[0] == '-')
        {
            arg.erase(0, 1); // drop '/' or '-'
            if (arg == "a")
            {
                if (result.mode == Mode::NONE)
                {
                    result.mode = Mode::APPEND;
                }
                else
                {
                    throw std::runtime_error("Cannot have /a with /adoc, /c, /d or /p");
                }
            }
            else if (arg == "adoc")
            {
                if (result.mode == Mode::NONE)
                {
                    result.mode = Mode::ASCII_DOC;
                }
                else
                {
                    throw std::runtime_error("Cannot have /adoc with /a, /c, /d or /p");
                }
            }
            else if (arg == "c")
            {
                if (result.mode == Mode::NONE)
                {
                    result.mode = Mode::COMPILE;
                }
                else
                {
                    throw std::runtime_error("Cannot have /c with /a, /adoc, /d or /p");
                }
            }
            else if (arg == "d")
            {
                if (result.mode == Mode::NONE)
                {
                    result.mode = Mode::DELETE;
                }
                else
                {
                    throw std::runtime_error("Cannot have /d with /a, /adoc, /c or /p");
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
                if (result.mode == Mode::COMPILE)
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
                if (result.mode == Mode::ASCII_DOC)
                {
                    result.output_dir = output_dir;
                }
                else
                {
                    throw std::runtime_error("/o switch allowed only when writing ASCII doc (/adoc)");
                }
            }
            else if (arg == "p")
            {
                if (result.mode == Mode::NONE)
                {
                    result.mode = Mode::PRINT;
                }
                else
                {
                    throw std::runtime_error("Cannot have /p with /a, /adoc, /c or /d");
                }
            }
            else if (arg == "s")
            {
                if (result.mode == Mode::COMPILE)
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
                if (result.mode == Mode::COMPILE || result.mode == Mode::PRINT ||
                    result.mode == Mode::ASCII_DOC)
                {
                    result.swap_path = swap_path;
                }
                else
                {
                    throw std::runtime_error(
                        "/r switch allowed when compiling (/c), printing (/p) or creating AsciiDoc (/adoc)");
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
