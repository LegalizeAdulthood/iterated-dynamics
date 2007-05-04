#if !defined(SOUND_STATE_H)
#define SOUND_STATE_H

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
	int parse_command(const cmd_context &context);

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

private:
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
