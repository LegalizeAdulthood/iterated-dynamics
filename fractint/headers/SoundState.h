#if !defined(SOUND_STATE_H)
#define SOUND_STATE_H

#include "CommandParser.h"

/* note attenuation values */
enum AttenuateType
{
	ATTENUATE_NONE		= 0,
	ATTENUATE_LOW		= 1,
	ATTENUATE_MIDDLE	= 2,
	ATTENUATE_HIGH		= 3
};

enum SoundFlagType
{
	SOUNDFLAG_OFF		= 0,
	SOUNDFLAG_BEEP		= 1,
	SOUNDFLAG_X			= 2,
	SOUNDFLAG_Y			= 3,
	SOUNDFLAG_Z			= 4,
	SOUNDFLAG_ORBITMASK = 0x07,
	SOUNDFLAG_SPEAKER	= 0x08,
	SOUNDFLAG_OPL3_FM	= 0x10,
	SOUNDFLAG_MIDI		= 0x20,
	SOUNDFLAG_QUANTIZED = 0x40,
	SOUNDFLAG_MASK		= 0x7F
};

class SoundState
{
public:
	enum
	{
		NUM_OCTAVES = 12
	};

	SoundState();
	void initialize();
	int open();
	void close();
	void tone(int tone);
	void write_time();
	int get_parameters();
	void orbit(int x, int y);
	void orbit(double x, double y, double z);

	const char *parameter_text() const;
	int parse_sound(const cmd_context &context);
	int parse_hertz(const cmd_context &context);
	int parse_attack(const cmd_context &context);
	int parse_decay(const cmd_context &context);
	int parse_release(const cmd_context &context);
	int parse_sustain(const cmd_context &context);
	int parse_volume(const cmd_context &context);
	int parse_wave_type(const cmd_context &context);
	int parse_attenuation(const cmd_context &context);
	int parse_polyphony(const cmd_context &context);
	int parse_scale_map(const cmd_context &context);

	int flags() const				{ return m_flags; }
	int base_hertz() const			{ return m_base_hertz; }
	int fm_volume() const			{ return m_fm_volume; }

	void set_flags(int flags)		{ m_flags = flags; }
	void silence_xyz()				{ m_flags &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); }
	void set_speaker_beep()			{ m_flags = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; }

private:
	int m_flags;
	int m_base_hertz;				/* sound=x/y/z hertz value */
	int m_fm_attack;
	int m_fm_decay;
	int m_fm_release;
	int m_fm_sustain;
	int m_fm_volume;
	int m_fm_wave_type;
	int m_note_attenuation;
	int m_polyphony;
	int m_scale_map[NUM_OCTAVES];	/* array for mapping notes to a (user defined) scale */

	enum
	{
		NUM_CHANNELS = 9,
		DEFAULT_FM_RELEASE = 5,
		DEFAULT_FM_SUSTAIN = 13,
		DEFAULT_FM_DECAY = 10,
		DEFAULT_FM_ATTACK = 5,
		DEFAULT_FM_WAVE_TYPE = 0,
		DEFAULT_POLYPHONY = 0,
		DEFAULT_FM_VOLUME = 63,
		DEFAULT_BASE_HERTZ = 440
	};
	void old_orbit(int x, int y);
	void new_orbit(int x, int y);
	int sound_on(int freq);
	void sound_off();
	int get_music_parameters();
	int get_scale_map();
	bool default_scale_map() const;

	// TODO: these functions need to be migrated to the driver for sound support
	void mute();
	void buzzer(int tone);
	void initfm() {}
	int fm(int, int)			{ return 0; }
	int buzzerpcspkr(int tone)	{ return 0; }
	int sleepms(int delay)		{ return 0; }

	FILE *m_fp;
	int m_menu_count;
	int m_fm_offset[NUM_CHANNELS];
	int m_fm_channel;
};

extern SoundState g_sound_state;

#endif
