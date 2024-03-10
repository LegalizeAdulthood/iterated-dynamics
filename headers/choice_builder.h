#pragma once

#include "full_screen_prompt.h"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

struct FullScreenPrompt
{
    int operator()(char const *hdg, int numprompts, char const **prompts, fullscreenvalues *values, int fkeymask,
        char *extrainfo) const
    {
        return fullscreen_prompt(hdg, numprompts, prompts, values, fkeymask, extrainfo);
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
        uvalues[m_current_build].type = 'y';
        uvalues[m_current_build].uval.ch.val = value ? 1 : 0;
        return advance();
    }
    ChoiceBuilder &int_number(const char *choice, int value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 'i';
        uvalues[m_current_build].uval.ival = value;
        return advance();
    }
    ChoiceBuilder &long_number(const char *choice, long value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 'L';
        uvalues[m_current_build].uval.Lval = value;
        return advance();
    }
    ChoiceBuilder &float_number(const char *choice, float value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 'f';
        uvalues[m_current_build].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &double_number(const char *choice, double value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 'd';
        uvalues[m_current_build].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &int_double_number(const char *choice, double value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 'D';
        uvalues[m_current_build].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &comment(const char *choice)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = '*';
        return advance();
    }
    ChoiceBuilder &list(const char *choice, int list_len, int list_value_len, const char *list[], int value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 'l';
        uvalues[m_current_build].uval.ch.llen = list_len;
        uvalues[m_current_build].uval.ch.vlen = list_value_len;
        uvalues[m_current_build].uval.ch.list = list;
        uvalues[m_current_build].uval.ch.val = value;
        return advance();
    }
    ChoiceBuilder &string(const char *choice, const char *value)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 's';
        std::strncpy(uvalues[m_current_build].uval.sval, value, 16);
        uvalues[m_current_build].uval.sval[15] = 0;
        return advance();
    }
    ChoiceBuilder &string_buff(const char *choice, char *buffer, int len)
    {
        check_build_overflow();
        choices[m_current_build] = choice;
        uvalues[m_current_build].type = 0x100 + len;
        uvalues[m_current_build].uval.sbuf = buffer;
        return advance();
    }
    ChoiceBuilder &reset()
    {
        m_current_build = 0;
        choices = std::array<const char *, N>{};
        uvalues = std::array<fullscreenvalues, N>{};
        m_prompted = false;
        m_result = 0;
        return *this;
    }

    int count() const
    {
        return m_current_build;
    }

    int prompt(char const *hdg, int fkeymask, char *extrainfo)
    {
        m_prompted = true;
        m_result = m_prompter(hdg, count(), choices.data(), uvalues.data(), fkeymask, extrainfo);
        return m_result;
    }

    bool read_yes_no() const
    {
        check_read_type("yes/no", 'y');
        return uvalues[m_current_read++].uval.ch.val == 1;
    }
    int read_int_number() const
    {
        check_read_type("int", 'i');
        return uvalues[m_current_read++].uval.ival;
    }
    long read_long_number() const
    {
        check_read_type("long", 'L');
        return uvalues[m_current_read++].uval.Lval;
    }
    float read_float_number() const
    {
        check_read_type("float", 'f');
        return static_cast<float>(uvalues[m_current_read++].uval.dval);
    }
    double read_double_number() const
    {
        check_read_type("double", 'd');
        return uvalues[m_current_read++].uval.dval;
    }
    double read_int_double_number() const
    {
        check_read_type("int double", 'D');
        return uvalues[m_current_read++].uval.dval;
    }
    void read_comment() const
    {
        check_read_type("comment", '*');
        m_current_read++;
    }
    int read_list() const
    {
        check_read_type("list", 'l');
        return uvalues[m_current_read++].uval.ch.val;
    }
    const char *read_string() const
    {
        check_read_type("string", 's');
        return uvalues[m_current_read++].uval.sval;
    }
    const char *read_string_buff(int len) const
    {
        check_read_type("string buff", 0x100 + len);
        return uvalues[m_current_read++].uval.sbuf;
    }

    std::array<const char *, N> choices{};
    std::array<fullscreenvalues, N> uvalues{};

private:

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
        if (uvalues[m_current_read].type != type)
        {
            throw std::runtime_error((std::string{"Attempt to read "} + label + " value from '" +
                static_cast<char>(uvalues[m_current_read].type) + "' type")
                                         .c_str());
        }
    }

    int m_current_build{};
    int m_result{};
    mutable int m_current_read{};
    bool m_prompted{};
    Prompter m_prompter;
};
