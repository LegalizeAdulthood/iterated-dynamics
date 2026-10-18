// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>

namespace id::engine
{

struct SoundSample
{
    double x{};
    double y{};
    double z{};
};

// No operation waits for playback or handles input.
class SoundOutput
{
public:
    virtual ~SoundOutput() = default;

    // Rejection leaves the sample unaccepted, without output side effects.
    virtual bool play(const SoundSample &sample) = 0;
    // Service due playback and normal voice release without waiting or input.
    virtual void update() = 0;
    // Query only; playback progresses through update().
    virtual bool ready() const = 0;
    // Mark each orbit boundary once, not again when a sample is retried.
    virtual void begin_orbit() = 0;
    // Cancel audible playback without discarding accepted file output.
    virtual void mute() = 0;
    // Return false until playback and release tails finish; never wait or mute.
    // Successful finalization is idempotent.
    virtual bool finish() = 0;
};

using SoundOutputPtr = std::shared_ptr<SoundOutput>;

extern SoundOutputPtr g_sound;

} // namespace id::engine
