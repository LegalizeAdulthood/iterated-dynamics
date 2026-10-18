// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/SoundOutput.h>

#include <engine/NullSoundOutput.h>
#include <misc/ValueSaver.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::misc;
using namespace testing;

namespace id::test
{

namespace
{

class RecordingSoundOutput : public SoundOutput
{
public:
    bool play(const SoundSample &sample) override;
    void update() override;
    bool ready() const override;
    void begin_orbit() override;
    void mute() override;
    bool finish() override;

    void set_ready(bool ready);
    void release_on_update();
    void complete_on_update();

    std::vector<SoundSample> samples;
    std::vector<std::size_t> orbit_offsets;
    int updates{};
    int releases{};
    int mutes{};
    int finalizations{};

private:
    void require_open() const;

    bool m_ready{true};
    bool m_playing{};
    bool m_release_due{};
    bool m_releasing{};
    bool m_completion_due{};
    bool m_finished{};
};

bool RecordingSoundOutput::play(const SoundSample &sample)
{
    require_open();
    if (!ready())
    {
        return false;
    }
    samples.push_back(sample);
    m_playing = true;
    return true;
}

void RecordingSoundOutput::update()
{
    ++updates;
    if (m_release_due && m_playing)
    {
        m_playing = false;
        m_release_due = false;
        m_releasing = true;
        ++releases;
    }
    if (m_completion_due)
    {
        m_releasing = false;
        m_completion_due = false;
    }
}

bool RecordingSoundOutput::ready() const
{
    return m_ready && !m_playing && !m_releasing && !m_finished;
}

void RecordingSoundOutput::begin_orbit()
{
    require_open();
    orbit_offsets.push_back(samples.size());
}

void RecordingSoundOutput::mute()
{
    m_playing = false;
    m_release_due = false;
    m_releasing = false;
    m_completion_due = false;
    ++mutes;
}

bool RecordingSoundOutput::finish()
{
    if (m_playing || m_releasing)
    {
        return false;
    }
    if (!m_finished)
    {
        m_finished = true;
        ++finalizations;
    }
    return true;
}

void RecordingSoundOutput::set_ready(const bool ready)
{
    m_ready = ready;
}

void RecordingSoundOutput::release_on_update()
{
    m_release_due = true;
}

void RecordingSoundOutput::complete_on_update()
{
    m_completion_due = true;
}

void RecordingSoundOutput::require_open() const
{
    if (m_finished)
    {
        throw std::logic_error("Sound output session is finished");
    }
}

} // namespace

TEST(TestNullSoundOutput, acceptsSamplesWithoutPlayback)
{
    NullSoundOutput output;

    output.begin_orbit();
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.play({1.0, 2.0, 3.0}));
    EXPECT_TRUE(output.play({4.0, 5.0, 6.0}));
    output.update();
    output.mute();
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.play({7.0, 8.0, 9.0}));
    EXPECT_TRUE(output.finish());
    EXPECT_TRUE(output.finish());
}

TEST(TestSoundOutputBinding, defaultsToNullSoundOutput)
{
    ASSERT_NE(nullptr, g_sound);
    EXPECT_NE(nullptr, dynamic_cast<NullSoundOutput *>(g_sound.get()));
}

TEST(TestSoundOutputBinding, scopedReplacementOwnsAndRestoresOutput)
{
    const SoundOutputPtr original{g_sound};
    std::shared_ptr<RecordingSoundOutput> output{std::make_shared<RecordingSoundOutput>()};
    const std::weak_ptr<SoundOutput> lifetime{output};
    {
        const ValueSaver<SoundOutputPtr> binding{g_sound, output};
        EXPECT_EQ(output, g_sound);
        g_sound->begin_orbit();
        EXPECT_TRUE(g_sound->play({1.0, 2.0, 3.0}));
        EXPECT_THAT(output->samples, ElementsAre(FieldsAre(1.0, 2.0, 3.0)));
        output.reset();
        EXPECT_FALSE(lifetime.expired());
        g_sound->mute();
        EXPECT_TRUE(g_sound->finish());
    }
    EXPECT_EQ(original, g_sound);
    EXPECT_TRUE(lifetime.expired());
}

class TestSoundOutput : public Test
{
protected:
    std::shared_ptr<RecordingSoundOutput> m_output{std::make_shared<RecordingSoundOutput>()};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, m_output};
};

TEST_F(TestSoundOutput, unusedSampleChannelsAreZero)
{
    EXPECT_TRUE(g_sound->play({4.0}));

    EXPECT_THAT(m_output->samples, ElementsAre(FieldsAre(4.0, 0.0, 0.0)));
}

TEST_F(TestSoundOutput, rejectionHasNoOutputSideEffects)
{
    m_output->set_ready(false);
    g_sound->begin_orbit();

    EXPECT_FALSE(g_sound->play({1.0, 2.0, 3.0}));
    EXPECT_FALSE(g_sound->play({1.0, 2.0, 3.0}));

    EXPECT_THAT(m_output->samples, IsEmpty());
    EXPECT_THAT(m_output->orbit_offsets, ElementsAre(0));
    EXPECT_EQ(0, m_output->updates);
    EXPECT_EQ(0, m_output->releases);
    EXPECT_EQ(0, m_output->mutes);
    EXPECT_EQ(0, m_output->finalizations);
}

TEST_F(TestSoundOutput, retryAcceptsSampleOnceAndPreservesOrder)
{
    m_output->set_ready(false);
    g_sound->begin_orbit();
    EXPECT_FALSE(g_sound->play({1.0, 2.0, 3.0}));
    m_output->set_ready(true);
    ASSERT_TRUE(g_sound->play({1.0, 2.0, 3.0}));
    EXPECT_FALSE(g_sound->play({4.0, 5.0, 6.0}));
    m_output->release_on_update();
    m_output->complete_on_update();
    g_sound->update();
    g_sound->begin_orbit();
    ASSERT_TRUE(g_sound->play({4.0, 5.0, 6.0}));

    EXPECT_THAT(m_output->samples, ElementsAre(FieldsAre(1.0, 2.0, 3.0), FieldsAre(4.0, 5.0, 6.0)));
    EXPECT_THAT(m_output->orbit_offsets, ElementsAre(0, 1));
}

TEST_F(TestSoundOutput, readinessQueryDoesNotServicePlayback)
{
    ASSERT_TRUE(g_sound->play({1.0, 2.0, 3.0}));
    m_output->release_on_update();
    m_output->complete_on_update();

    EXPECT_FALSE(g_sound->ready());
    EXPECT_FALSE(g_sound->ready());
    EXPECT_EQ(0, m_output->updates);
    EXPECT_EQ(0, m_output->releases);

    g_sound->update();

    EXPECT_TRUE(g_sound->ready());
    EXPECT_EQ(1, m_output->releases);
}

TEST_F(TestSoundOutput, normalCompletionDrainsReleaseTailWithoutMuting)
{
    ASSERT_TRUE(g_sound->play({1.0, 2.0, 3.0}));
    EXPECT_FALSE(g_sound->finish());
    m_output->release_on_update();

    g_sound->update();
    g_sound->update();

    EXPECT_EQ(1, m_output->releases);
    EXPECT_FALSE(g_sound->finish());
    EXPECT_EQ(0, m_output->finalizations);
    m_output->complete_on_update();
    g_sound->update();
    EXPECT_TRUE(g_sound->finish());
    EXPECT_TRUE(g_sound->finish());
    EXPECT_EQ(1, m_output->finalizations);
    EXPECT_EQ(0, m_output->mutes);
}

TEST_F(TestSoundOutput, mutePreservesSamplesAndClearsStaleRelease)
{
    ASSERT_TRUE(g_sound->play({1.0, 2.0, 3.0}));
    m_output->release_on_update();
    g_sound->mute();
    ASSERT_TRUE(g_sound->ready());
    ASSERT_TRUE(g_sound->play({4.0, 5.0, 6.0}));

    g_sound->update();

    EXPECT_EQ(0, m_output->releases);
    EXPECT_FALSE(g_sound->ready());
    EXPECT_FALSE(g_sound->finish());
    EXPECT_EQ(1, m_output->mutes);
    EXPECT_THAT(m_output->samples, ElementsAre(FieldsAre(1.0, 2.0, 3.0), FieldsAre(4.0, 5.0, 6.0)));
}

TEST_F(TestSoundOutput, muteCancelsReleaseTailBeforeFinalization)
{
    ASSERT_TRUE(g_sound->play({1.0, 2.0, 3.0}));
    m_output->release_on_update();
    g_sound->update();
    EXPECT_FALSE(g_sound->finish());

    g_sound->mute();

    EXPECT_TRUE(g_sound->finish());
    EXPECT_EQ(1, m_output->releases);
    EXPECT_EQ(1, m_output->mutes);
    EXPECT_EQ(1, m_output->finalizations);
    EXPECT_THAT(m_output->samples, ElementsAre(FieldsAre(1.0, 2.0, 3.0)));
}

TEST_F(TestSoundOutput, completedSessionRejectsFurtherOutput)
{
    EXPECT_TRUE(g_sound->finish());

    EXPECT_FALSE(g_sound->ready());
    EXPECT_THROW(g_sound->play({1.0, 2.0, 3.0}), std::logic_error);
    EXPECT_THROW(g_sound->begin_orbit(), std::logic_error);
}

} // namespace id::test
