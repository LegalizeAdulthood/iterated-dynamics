// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/Command.h"

#include "engine/lowerize_parameter.h"

#include <cctype>
#include <charconv>
#include <limits>
#include <locale>
#include <sstream>
#include <system_error>

namespace id::engine
{

static bool parse_command_number(std::string_view text, long &value)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0)
    {
        text.remove_prefix(1);
    }
    if (!text.empty() && text.front() == '+')
    {
        text.remove_prefix(1);
    }
    if (text.empty())
    {
        return false;
    }
    const char *const first{text.data()};
    const char *const last{first + text.size()};
    const std::from_chars_result result{std::from_chars(first, last, value)};
    return result.ec == std::errc{} && result.ptr == last;
}

static bool parse_command_number(std::string_view text, double &value)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0)
    {
        text.remove_prefix(1);
    }
    if (!text.empty() && text.front() == '+')
    {
        text.remove_prefix(1);
    }
    if (text.empty())
    {
        return false;
    }
    std::istringstream input{std::string{text}};
    input.imbue(std::locale::classic());
    input >> value;
    return input && input.eof();
}

// Crude function to detect a floating point number. Intended for
// use with arbitrary precision.
static bool is_a_big_float(const char *str)
{
    // [+|-]numbers][.]numbers[+|-][e|g]numbers
    bool result = true;
    const char *s = str;
    int num_dot{};
    int num_e{};
    int num_sign{};
    while (*s != 0 && *s != '/' && *s != ' ')
    {
        if (*s == '-' || *s == '+')
        {
            num_sign++;
        }
        else if (*s == '.')
        {
            num_dot++;
        }
        else if (*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G')
        {
            num_e++;
        }
        else if (!std::isdigit(*s))
        {
            result = false;
            break;
        }
        s++;
    }
    if (num_dot > 1 || num_sign > 2 || num_e > 1)
    {
        result = false;
    }
    return result;
}

Command::Command(const std::string_view cur_arg, const CmdFile a_mode, CommandParseResult &a_result) :
    mode(a_mode),
    result(a_result),
    m_arg(cur_arg)
{
    lowerize_parameter(m_arg);
    const std::size_t equals{m_arg.find('=', 1)};
    const std::size_t variable_end{equals == std::string::npos ? m_arg.size() : equals};
    const std::size_t value_offset{equals == std::string::npos ? m_arg.size() : equals + 1};
    value = std::string_view{m_arg}.substr(value_offset);
    int j = static_cast<int>(variable_end);
    if (j > 32)
    {
        arg_error(m_arg.c_str(), &result.diagnostics); // keyword too long
        status = CmdArgFlags::BAD_ARG;
        return;
    }
    variable = m_arg.substr(0, variable_end);
    char_val[0] = value.empty() ? '\0' : value[0]; // first letter of value
    yes_no_val[0] = -1;                            // note yes|no value
    if (char_val[0] == 'n')
    {
        yes_no_val[0] = 0;
    }
    if (char_val[0] == 'y')
    {
        yes_no_val[0] = 1;
    }

    num_val = 0;
    for (std::size_t value_pos{}; value_pos < value.size();) // count and pre-parse parms
    {
        const std::size_t slash{value.find('/', value_pos)};
        const std::size_t value_end{slash == std::string_view::npos ? value.size() : slash};
        const std::string_view token{value.substr(value_pos, value_end - value_pos)};
        const std::string_view remaining{value.substr(value_pos)};
        const char *const token_ptr{value.data() + value_pos};
        if (total_params == 0)
        {
            num_val = NON_NUMERIC;
        }
        if (total_params < 16)
        {
            string_vals[total_params] = token_ptr;
            string_val_lens[total_params] = static_cast<int>(token.size());
            char_val[total_params] = *token_ptr; // first letter of value
            if (char_val[total_params] == 'n')
            {
                yes_no_val[total_params] = 0;
            }
            if (char_val[total_params] == 'y')
            {
                yes_no_val[total_params] = 1;
            }
        }
        if (remaining.size() >= 2 && (remaining[0] == '/' || remaining[0] == '=') && remaining[1] == '/')
        {
            ++num_float_params;
            ++num_int_params;
            if (total_params < 16)
            {
                float_vals[total_params] = 0;
                float_val_strs[total_params] = "0";
            }
            if (total_params < 64)
            {
                int_vals[total_params] = 0;
                has_val[total_params] = true;
            }
            if (total_params == 0)
            {
                num_val = 0;
            }
        }
        else if (long ll{}; parse_command_number(token, ll)) // needs a long int here for lyapunov
        {
            ++num_float_params;
            ++num_int_params;
            if (total_params < 16)
            {
                float_vals[total_params] = ll;
                float_val_strs[total_params] = token_ptr;
            }
            if (total_params < 64)
            {
                int_vals[total_params] = static_cast<int>(ll);
                has_val[total_params] = true;
            }
            if (total_params == 0)
            {
                num_val = static_cast<int>(ll);
            }
        }
        else if (double f_temp{}; parse_command_number(token, f_temp)) // got a float
        {
            ++num_float_params;
            if (total_params < 16)
            {
                float_vals[total_params] = f_temp;
                float_val_strs[total_params] = token_ptr;
            }
        }
        // using arbitrary precision and above failed
        else if (remaining.size() > 513                                                  // very long command
            || (total_params > 0 && total_params < 6                                     //
                   && float_vals[total_params - 1] == std::numeric_limits<float>::max()) //
            || is_a_big_float(token_ptr))
        {
            ++num_float_params;
            if (total_params < 16)
            {
                float_vals[total_params] = std::numeric_limits<float>::max();
                float_val_strs[total_params] = token_ptr;
            }
        }
        ++total_params;
        if (slash == std::string_view::npos)
        {
            break;
        }
        value_pos = slash + 1;
    }
}

std::string_view Command::string_val(const int index) const
{
    return {string_vals[index], static_cast<size_t>(string_val_lens[index])};
}

CmdArgFlags Command::bad_arg() const
{
    arg_error(m_arg.c_str(), &result.diagnostics);
    return CmdArgFlags::BAD_ARG;
}

void Command::warn_missing_file() const
{
    static constexpr const char *MODE_NAMES[]{"command line", "sstools.ini", "PAR file", "PAR file"};
    result.diagnostics.warnings.push_back("Can't find " + variable.substr(0, 29) + '=' + std::string{value} +
        ", please check " + MODE_NAMES[static_cast<int>(mode)]);
}

void arg_error(const char *bad_arg, CommandDiagnostics *diagnostics)
{
    std::string spillover{bad_arg};
    if (spillover.length() > 70)
    {
        spillover = spillover.substr(0, 70);
    }
    std::string msg{"Oops. I couldn't understand the argument:\n  "};
    msg += spillover;

    if (g_first_init) // this is 1st call to cmdfiles
    {
        msg += "\n"
               "\n"
               "(see the Startup Help screens or documentation for a complete\n"
               " argument list with descriptions)";
    }
    if (diagnostics != nullptr)
    {
        diagnostics->errors.push_back(msg);
    }
}

} // namespace id::engine
