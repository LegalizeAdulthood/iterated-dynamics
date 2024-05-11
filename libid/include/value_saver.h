#pragma once

/// Saves the current value in the c'tor before assigning a new value.
/// The d'tor restores the original value.
template <typename T>
class ValueSaver
{
public:
    ValueSaver(T &data, T value) :
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
