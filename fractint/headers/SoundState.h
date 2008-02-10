#if !defined(SOUND_STATE_H)
#define SOUND_STATE_H

#include "CommandParser.h"

// note attenuation values 
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

class SoundStateImpl;

class SoundState
{
public:
	SoundState();
	~SoundState();
	void initialize();
	bool open();
	void close();
	void tone(int tone);
	void write_time();
	int get_parameters();
	void orbit(int x, int y);
	void orbit(double x, double y, double z);

	std::string parameter_text() const;
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

	int flags() const;
	int base_hertz() const;
	int fm_volume() const;

	void set_flags(int flags);
	void silence_xyz();
	void set_speaker_beep();

private:
	SoundStateImpl *_impl;
};

extern SoundState g_sound_state;

#endif
