// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "FailingOutputBuffer.h"

#include <algorithm>

namespace id::test
{

std::streamsize FailingOutputBuffer::xsputn(const char *data, const std::streamsize count)
{
    const std::streamsize written{std::stringbuf::xsputn(data, std::min(count, remaining))};
    remaining -= written;
    return written;
}

int FailingOutputBuffer::sync()
{
    ++sync_count;
    return fail_sync ? -1 : 0;
}

} // namespace id::test
