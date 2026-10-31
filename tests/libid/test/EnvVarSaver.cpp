// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <EnvVarSaver.h>

#include <cstdlib>

namespace id::test
{

EnvVarSaver::EnvVarSaver(const char *name, const char *value) :
    m_name{name}
{
    if (const char *old_value = std::getenv(name); old_value != nullptr)
    {
        m_had_value = true;
        m_value = old_value;
    }
    set(value);
}

EnvVarSaver::~EnvVarSaver()
{
    set(m_had_value ? m_value.c_str() : nullptr);
}

void EnvVarSaver::set(const char *value) const
{
#ifdef _WIN32
    _putenv_s(m_name.c_str(), value == nullptr ? "" : value);
#else
    if (value == nullptr)
    {
        unsetenv(m_name.c_str());
    }
    else
    {
        setenv(m_name.c_str(), value, 1);
    }
#endif
}

} // namespace id::test
