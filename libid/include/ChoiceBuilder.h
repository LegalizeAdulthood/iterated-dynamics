// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "full_screen_prompt.h"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

struct FullScreenPrompt
{
    int operator()(char const *hdg, int num_prompts, char const **prompts, FullScreenValues *values,
        int fn_key_mask, char *extra_info) const
    {
        return full_screen_prompt(hdg, num_prompts, prompts, values, fn_key_mask, extra_info);
    }
};

template <int N, typename Prompter = FullScreenPrompt>
class ChoiceBuilder
{
public:
    ChoiceBuilder &yes_no(const char *choice, bool value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'y';
        values[m_current_build].uval.ch.val = value ? 1 : 0;
        return advance();
    }
    ChoiceBuilder &int_number(const char *choice, int value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'i';
        values[m_current_build].uval.ival = value;
        return advance();
    }
    ChoiceBuilder &long_number(const char *choice, long value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'L';
        values[m_current_build].uval.Lval = value;
        return advance();
    }
    ChoiceBuilder &float_number(const char *choice, float value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'f';
        values[m_current_build].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &double_number(const char *choice, double value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'd';
        values[m_current_build].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &int_double_number(const char *choice, double value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'D';
        values[m_current_build].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &comment(const char *choice)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = '*';
        return advance();
    }
    ChoiceBuilder &list(const char *choice, int list_len, int list_value_len, const char *list[], int value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 'l';
        values[m_current_build].uval.ch.llen = list_len;
        values[m_current_build].uval.ch.vlen = list_value_len;
        values[m_current_build].uval.ch.list = list;
        values[m_current_build].uval.ch.val = value;
        return advance();
    }
    ChoiceBuilder &string(const char *choice, const char *value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 's';
        std::strncpy(values[m_current_build].uval.sval, value, 16);
        values[m_current_build].uval.sval[15] = 0;
        return advance();
    }
    ChoiceBuilder &string_buff(const char *choice, char *buffer, int len)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        values[m_current_build].type = 0x100 + len;
        values[m_current_build].uval.sbuf = buffer;
        return advance();
    }

    ChoiceBuilder &reset()
    {
        m_current_build = 0;
        m_current_read = 0;
        choices = std::array<const char *, N>{};
        values = std::array<FullScreenValues, N>{};
        m_prompted = false;
        m_result = 0;
        return *this;
    }

    int count() const
    {
        return m_current_build;
    }

    int prompt(char const *hdg, int fn_key_mask, char *extra_info)
    {
        m_prompted = true;
        m_result = m_prompter(hdg, count(), choices.data(), values.data(), fn_key_mask, extra_info);
        m_current_read = 0;
        return m_result;
    }
    int prompt(char const *hdg, int fn_key_mask)
    {
        return prompt(hdg, fn_key_mask, nullptr);
    }
    int prompt(char const *hdg)
    {
        return prompt(hdg, 0, nullptr);
    }

    bool read_yes_no() const
    {
        check_read_type("yes/no", 'l');
        return values[m_current_read++].uval.ch.val == 1;
    }
    int read_int_number() const
    {
        check_read_type("int", 'i');
        return values[m_current_read++].uval.ival;
    }
    long read_long_number() const
    {
        check_read_type("long", 'L');
        return values[m_current_read++].uval.Lval;
    }
    float read_float_number() const
    {
        check_read_type("float", 'f');
        return static_cast<float>(values[m_current_read++].uval.dval);
    }
    double read_double_number() const
    {
        check_read_type("double", 'd');
        return values[m_current_read++].uval.dval;
    }
    double read_int_double_number() const
    {
        check_read_type("int double", 'D');
        return values[m_current_read++].uval.dval;
    }
    void read_comment() const
    {
        check_read_type("comment", '*');
        m_current_read++;
    }
    int read_list() const
    {
        check_read_type("list", 'l');
        return values[m_current_read++].uval.ch.val;
    }
    const char *read_string() const
    {
        check_read_type("string", 's');
        return values[m_current_read++].uval.sval;
    }
    const char *read_string_buff(int len) const
    {
        check_read_type("string buff", 0x100 + len);
        return values[m_current_read++].uval.sbuf;
    }

private:
    std::array<const char *, N> choices{};
    std::array<FullScreenValues, N> values{};

    ChoiceBuilder &advance()
    {
        ++m_current_build;
        return *this;
    }
    void check_overflow(int val) const
    {
        if (val == N)
        {
            throw std::runtime_error("ChoiceBuilder<" + std::to_string(N) + "> overflow ");
        }
    }
    void check_build_overflow() const
    {
        check_overflow(m_current_build);
    }
    void check_read_type(const char *label, int type) const
    {
        if (!m_prompted)
        {
            throw std::runtime_error("Reading result before being prompted for result");
        }
        check_overflow(m_current_read);
        if (values[m_current_read].type != type)
        {
            throw std::runtime_error((std::string{"Attempt to read "} + label + " value from '" +
                static_cast<char>(values[m_current_read].type) + "' type")
                                         .c_str());
        }
    }

    int m_current_build{};
    int m_result{};
    mutable int m_current_read{};
    bool m_prompted{};
    Prompter m_prompter;
};
