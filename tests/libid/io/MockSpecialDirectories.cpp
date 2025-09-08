// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockSpecialDirectories.h"

namespace id::io
{

namespace test
{

std::shared_ptr<MockSpecialDirectories> g_mock_special_dirs;

} // namespace test

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    using namespace id::io::test;
    g_mock_special_dirs = std::make_shared<MockSpecialDirectories>();
    return g_mock_special_dirs;
}

} // namespace id::io
