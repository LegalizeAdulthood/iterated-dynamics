// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <limits>
#include <sstream>

namespace id::test
{

class FailingOutputBuffer : public std::stringbuf
{
public:
    std::streamsize remaining{std::numeric_limits<std::streamsize>::max()};
    bool fail_sync{};
    int sync_count{};

protected:
    std::streamsize xsputn(const char *data, std::streamsize count) override;
    int sync() override;
};

} // namespace id::test
