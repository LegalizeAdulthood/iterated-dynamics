// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <engine/SoundOutput.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string_view>

namespace id::ui
{

class DriverSoundOutput : public engine::SoundOutput
{
public:
    enum class Source
    {
        OVERLAY,
        STICKY_ORBIT,
        ORBIT,
        RESTORED_IMAGE,
    };

    using Clock = std::chrono::steady_clock;
    using NowFn = std::function<Clock::time_point()>;
    // Legacy time records use CPU milliseconds, not playback deadlines.
    using TimestampFn = std::function<long long()>;

    // Capture the current sound configuration for this session.
    explicit DriverSoundOutput(Source source);
    DriverSoundOutput(Source source, NowFn now, TimestampFn timestamp);
    ~DriverSoundOutput() override;

    DriverSoundOutput(const DriverSoundOutput &) = delete;
    DriverSoundOutput(DriverSoundOutput &&) = delete;
    DriverSoundOutput &operator=(const DriverSoundOutput &) = delete;
    DriverSoundOutput &operator=(DriverSoundOutput &&) = delete;

    bool play(const engine::SoundSample &sample) override;
    void update() override;
    bool ready() const override;
    void begin_orbit() override;
    void mute() override;
    bool finish() override;

private:
    int frequency(const engine::SoundSample &sample) const;
    void require_open() const;
    void write_record(std::string_view record);

    Source m_source;
    int m_mode;
    int m_base_hertz;
    int m_x_dots;
    int m_y_dots;
    int m_colors;
    bool m_scaled;
    bool m_record;
    std::chrono::microseconds m_tone_duration;
    NowFn m_now;
    TimestampFn m_timestamp;
    Clock::time_point m_deadline{};
    std::filesystem::path m_path;
    std::ofstream m_file;
    bool m_playing{};
    bool m_finished{};
};

} // namespace id::ui
