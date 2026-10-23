// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <initializer_list>

namespace id::misc
{
class Driver;
}

namespace id::ui
{

int id_main(int argc, char *argv[], std::initializer_list<misc::Driver *> drivers);

} // namespace id::ui
