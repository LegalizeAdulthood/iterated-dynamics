// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <gtest/gtest.h>

namespace id::test
{

template <typename T>
class ValueUnchanged
{
public:
    ValueUnchanged(const char *label, T &data, T value) :
        m_label(label),
        m_data(data),
        m_saved_value(data),
        m_value(value)
    {
        m_data = value;
    }

    ~ValueUnchanged()
    {
        EXPECT_EQ(m_value, m_data) << m_label;
        m_data = m_saved_value;
    }

    ValueUnchanged(const ValueUnchanged &rhs) = delete;
    ValueUnchanged(ValueUnchanged &&rhs) = delete;
    ValueUnchanged &operator=(const ValueUnchanged &rhs) = delete;
    ValueUnchanged &operator=(ValueUnchanged &&rhs) = delete;

private:
    std::string m_label;
    T &m_data;
    T m_saved_value;
    T m_value;
};

#define VALUE_UNCHANGED(var_, value_) ValueUnchanged saved_##var_(#var_, var_, value_)

} // namespace id::test
