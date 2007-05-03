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
	FILE *m_fp;
};

extern SoundState g_sound_state;

#endif
