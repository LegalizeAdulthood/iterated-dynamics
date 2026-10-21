// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/DriverSoundOutput.h>

#include <engine/LogicalScreen.h>
#include <engine/calcfrac.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <fractals/fractalp.h>
#include <io/check_write_file.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <ui/OrbitDelayTimer.h>
#include <ui/dynamic2d.h>
#include <ui/orbit2d.h>

#include <fmt/format.h>

#include <cmath>
#include <ctime>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

namespace
{

long long sound_time_ms()
{
    return static_cast<long long>(std::clock()) * 1000 / CLOCKS_PER_SEC;
}

DriverSoundOutput::Source image_sound_source()
{
    const CalcType calc_type{g_cur_fractal_specific->calc_type};
    if (calc_type == static_cast<CalcType>(orbit2d_type) || calc_type == orbit3d_type ||
        calc_type == static_cast<CalcType>(dynamic2d_type))
    {
        return DriverSoundOutput::Source::ORBIT;
    }
    if ((calc_type == standard_fractal_type || calc_type == calc_mandelbrot_type) && g_std_calc_mode == CalcMode::ORBIT)
    {
        return DriverSoundOutput::Source::STICKY_ORBIT;
    }
    return DriverSoundOutput::Source::OVERLAY;
}

} // namespace

DriverSoundOutput::DriverSoundOutput(const Source source) :
    DriverSoundOutput(source, Clock::now, sound_time_ms)
{
}

DriverSoundOutput::DriverSoundOutput(const Source source, NowFn now, TimestampFn timestamp) :
    m_source{source},
    m_mode{g_sound_flag & SOUNDFLAG_ORBIT_MASK},
    m_base_hertz{g_base_hertz},
    m_x_dots{g_logical_screen.x_dots},
    m_y_dots{g_logical_screen.y_dots},
    m_colors{g_colors},
    m_scaled{g_debug_flag == DebugFlags::FORCE_SCALED_SOUND_FORMULA},
    m_record{(g_orbit_save_flags & OSF_MIDI) != 0},
    m_tone_duration{std::chrono::microseconds{g_orbit_delay} * static_cast<int>(ORBIT_DELAY_UNIT_MICROSECONDS)},
    m_now{std::move(now)},
    m_timestamp{std::move(timestamp)}
{
    if (!m_now || !m_timestamp || g_orbit_delay < 0 || m_mode > SOUNDFLAG_Z)
    {
        throw std::logic_error("Invalid sound output configuration");
    }
    if (m_mode > SOUNDFLAG_BEEP &&
        ((m_source == Source::OVERLAY && m_scaled && (m_x_dots <= 0 || m_y_dots <= 0)) ||
            (m_source == Source::RESTORED_IMAGE && m_colors <= 0)))
    {
        throw std::logic_error("Sound output requires valid image dimensions");
    }
}

DriverSoundOutput::~DriverSoundOutput()
{
    if (m_playing)
    {
        driver_mute();
    }
}

bool DriverSoundOutput::play(const SoundSample &sample)
{
    require_open();
    if (!ready())
    {
        return false;
    }
    if (m_mode <= SOUNDFLAG_BEEP)
    {
        return true;
    }

    if (m_source == Source::RENDER)
    {
        m_source = image_sound_source();
        if (m_source == Source::OVERLAY && m_scaled && (m_x_dots <= 0 || m_y_dots <= 0))
        {
            throw std::logic_error("Sound output requires valid image dimensions");
        }
    }
    const int tone{frequency(sample)};
    if (m_record)
    {
        write_record(fmt::format("{}\n", tone));
    }
    if (driver_sound_on(tone))
    {
        m_playing = true;
        m_deadline = m_now() + m_tone_duration;
    }
    return true;
}

void DriverSoundOutput::update()
{
    if (m_playing && m_now() >= m_deadline)
    {
        driver_sound_off();
        m_playing = false;
    }
}

bool DriverSoundOutput::ready() const
{
    return !m_finished && !m_playing;
}

void DriverSoundOutput::begin_orbit()
{
    require_open();
    if (m_record)
    {
        write_record(fmt::format("time={}\n", m_timestamp()));
    }
}

void DriverSoundOutput::mute()
{
    require_open();
    driver_mute();
    m_playing = false;
    m_deadline = {};
}

bool DriverSoundOutput::finish()
{
    if (m_playing)
    {
        return false;
    }
    if (!m_finished)
    {
        m_finished = true;
        if (m_file.is_open())
        {
            m_file.close();
            if (m_file.fail())
            {
                throw std::runtime_error(fmt::format("Can't close {}.", m_path.string()));
            }
        }
    }
    return true;
}

int DriverSoundOutput::frequency(const SoundSample &sample) const
{
    double tone{};
    switch (m_source)
    {
    case Source::OVERLAY:
        if (m_scaled)
        {
            // The legacy scaled formula maps both Y and Z modes to Y.
            tone =
                m_mode == SOUNDFLAG_X ? std::trunc(sample.x * 1000 / m_x_dots) : std::trunc(sample.y * 1000 / m_y_dots);
        }
        else
        {
            tone = m_mode == SOUNDFLAG_X ? sample.x : m_mode == SOUNDFLAG_Y ? sample.y : sample.x + sample.y;
        }
        break;

    case Source::STICKY_ORBIT:
        break;

    case Source::ORBIT:
        tone = (m_mode == SOUNDFLAG_X ? sample.x : m_mode == SOUNDFLAG_Y ? sample.y : sample.z) * 100;
        break;

    case Source::RESTORED_IMAGE:
        tone = std::trunc(sample.x * 3000 / m_colors);
        break;

    default:
        throw std::logic_error("Unknown sound sample source");
    }
    tone += m_base_hertz;
    if (!std::isfinite(tone) || tone < std::numeric_limits<int>::min() || tone > std::numeric_limits<int>::max())
    {
        throw std::logic_error("Sound frequency is outside the integer range");
    }
    return static_cast<int>(tone);
}

void DriverSoundOutput::require_open() const
{
    if (m_finished)
    {
        throw std::logic_error("Sound output session is finished");
    }
}

void DriverSoundOutput::write_record(const std::string_view record)
{
    if (!m_file.is_open())
    {
        m_path = get_checked_save_path(WriteFile::SOUND, "sound001.txt");
        if (m_path.empty())
        {
            throw std::runtime_error("Sound output path is empty");
        }
        m_file.clear();
        m_file.open(m_path);
        if (!m_file.is_open())
        {
            throw std::runtime_error(fmt::format("Can't open {} for writing.", m_path.string()));
        }
    }
    m_file << record;
    m_file.flush();
    if (!m_file)
    {
        throw std::runtime_error(fmt::format("Can't write {}.", m_path.string()));
    }
}

} // namespace id::ui
