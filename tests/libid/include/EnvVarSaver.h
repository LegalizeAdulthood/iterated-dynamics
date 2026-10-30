// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <string>

namespace id::test
{

class EnvVarSaver
{
public:
    EnvVarSaver(const char *name, const char *value);
    ~EnvVarSaver();

private:
    void set(const char *value) const;

    std::string m_name;
    bool m_had_value{};
    std::string m_value;
};

} // namespace id::test
