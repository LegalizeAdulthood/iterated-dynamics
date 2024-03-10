#pragma once

#include "full_screen_prompt.h"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

template <int N>
class ChoiceBuilder
{
public:
    ChoiceBuilder &yes_no(const char *choice, bool value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'y';
        uvalues[m_current].uval.ch.val = value ? 1 : 0;
        return advance();
    }
    ChoiceBuilder &int_number(const char *choice, int value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'i';
        uvalues[m_current].uval.ival = value;
        return advance();
    }
    ChoiceBuilder &long_number(const char *choice, long value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'L';
        uvalues[m_current].uval.Lval = value;
        return advance();
    }
    ChoiceBuilder &float_number(const char *choice, float value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'f';
        uvalues[m_current].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &double_number(const char *choice, double value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'd';
        uvalues[m_current].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &int_double_number(const char *choice, double value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'D';
        uvalues[m_current].uval.dval = value;
        return advance();
    }
    ChoiceBuilder &comment(const char *choice)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = '*';
        return advance();
    }
    ChoiceBuilder &list(const char *choice, int list_len, int list_value_len, const char *list[], int value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 'l';
        uvalues[m_current].uval.ch.llen = list_len;
        uvalues[m_current].uval.ch.vlen = list_value_len;
        uvalues[m_current].uval.ch.list = list;
        uvalues[m_current].uval.ch.val = value;
        return advance();
    }
    ChoiceBuilder &string(const char *choice, const char *value)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 's';
        std::strcpy(uvalues[m_current].uval.sval, value);
        return advance();
    }
    ChoiceBuilder &string_buff(const char *choice, char *buffer, int len)
    {
        check_overflow();
        choices[m_current] = choice;
        uvalues[m_current].type = 0x100 + len;
        uvalues[m_current].uval.sbuf = buffer;
        return advance();
    }
    ChoiceBuilder &reset()
    {
        m_current = 0;
        choices = std::array<const char *, N>{};
        uvalues = std::array<fullscreenvalues, N>{};
        return *this;
    }

    int count() const
    {
        return m_current;
    }

    int prompt(char const *hdg, int fkeymask, char *extrainfo)
    {
        return fullscreen_prompt(hdg, count(), choices.data(), uvalues.data(), fkeymask, extrainfo);
    }

    std::array<const char *, N> choices{};
    std::array<fullscreenvalues, N> uvalues{};

private:
    ChoiceBuilder &advance()
    {
        ++m_current;
        return *this;
    }
    void check_overflow()
    {
        if(m_current == N)
        {
            throw std::runtime_error("ChoiceBuilder<" + std::to_string(N) + "> overflow ");
        }
    }

    int m_current{};
};
