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
		NUM_CHANNELS = 9
	};
	void old_orbit(int x, int y);
	void new_orbit(int x, int y);
	int sound_on(int freq);
	void sound_off();
	int get_music_parameters();
	int get_scale_map();
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
