// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

/// Saves the current value in the c'tor before assigning a new value.
/// The d'tor restores the original value.
template <typename T>
class ValueSaver
{
public:
    template <typename U>
    ValueSaver(T &data, U value) :
        m_saved(data),
        m_data(data)
    {
        m_data = value;
    }
    ~ValueSaver()
    {
        m_data = m_saved;
    }

private:
    T m_saved;
    T &m_data;
};
