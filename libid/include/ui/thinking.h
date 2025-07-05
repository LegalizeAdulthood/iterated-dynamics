// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

bool thinking(int options, const char *msg);

class Thinking
{
public:
    Thinking(int options, const char *msg) :
        m_thinking(true)
    {
        thinking(options, msg);
    }

    ~Thinking()
    {
        if (m_thinking)
        {
            thinking(0, nullptr);
        }
    }

    void cancel()
    {
        m_thinking = false;
    }

private:
    bool m_thinking{};
};
