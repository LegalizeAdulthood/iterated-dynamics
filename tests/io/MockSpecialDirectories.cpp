// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockSpecialDirectories.h"

std::shared_ptr<MockSpecialDirectories> g_mock_special_dirs;

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    g_mock_special_dirs = std::make_shared<MockSpecialDirectories>();
    return g_mock_special_dirs;
}
