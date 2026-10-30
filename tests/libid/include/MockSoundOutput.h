// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <engine/SoundOutput.h>

#include <gmock/gmock.h>

namespace id::test
{

class MockSoundOutput : public engine::SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const engine::SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

} // namespace id::test
