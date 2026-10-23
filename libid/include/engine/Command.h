// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/cmdfiles.h"

#include <string>
#include <string_view>

namespace id::engine
{

inline constexpr int NON_NUMERIC{-32767};

class Command
{
public:
    Command(std::string_view cur_arg, CmdFile a_mode, CommandParseResult &result);
    Command(const Command &) = delete;
    Command(Command &&) = delete;
    Command &operator=(const Command &) = delete;
    Command &operator=(Command &&) = delete;

    CmdArgFlags bad_arg() const;
    void warn_missing_file() const;
    std::string_view string_val(int index) const;

    CmdFile mode{};
    std::string_view value;
    std::string variable;
    int num_val{};                    // numeric value of arg
    char char_val[16]{};              // first character of arg
    int yes_no_val[16]{};             // 0 if 'n', 1 if 'y', -1 if not
    int total_params{};               // # of / delimited parms
    int num_int_params{};             // # of / delimited ints
    int num_float_params{};           // # of / delimited floats
    int int_vals[64]{};               // pre-parsed integer parms
    bool has_val[64]{};               // int_vals slot is valid
    double float_vals[16]{};          // pre-parsed floating parms
    const char *float_val_strs[16]{}; // pointers to float vals
    const char *string_vals[16]{};    // pointers to all string vals
    int string_val_lens[16]{};        // string value lengths
    CmdArgFlags status{CmdArgFlags::NONE};
    CommandParseResult &result;

private:
    std::string m_arg;
};

void arg_error(const char *bad_arg, CommandDiagnostics *diagnostics);

} // namespace id::engine
