// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::misc
{

/// Saves the current value in the c'tor before assigning a new value.
/// The d'tor restores the original value.
template <typename T>
class ValueSaver
{
public:
    ValueSaver(const ValueSaver &rhs) = delete;
    ValueSaver(ValueSaver &&rhs) = delete;

    template <typename U>
    ValueSaver(T &data, U value) :
        m_saved(data),
        m_data(data)
    {
        m_data = value;
    }

    // doesn't change current value, but captures it
    explicit ValueSaver(T &data) :
        m_saved(data),
        m_data(data)
    {
    }

    ~ValueSaver()
    {
        m_data = m_saved;
    }

    ValueSaver &operator=(const ValueSaver &rhs) = delete;
    ValueSaver &operator=(ValueSaver &&rhs) = delete;

private:
    T m_saved;
    T &m_data;
};

} // namespace id::misc
