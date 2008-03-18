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

class SoundState
{
public:
	virtual ~SoundState() { }
	virtual void initialize() = 0;
	virtual bool open() = 0;
	virtual void close() = 0;
	virtual void tone(int tone) = 0;
	virtual void write_time() = 0;
	virtual int get_parameters() = 0;
	virtual void orbit(int x, int y) = 0;
	virtual void orbit(double x, double y, double z) = 0;

	virtual std::string parameter_text() const = 0;
	virtual int parse_sound(const cmd_context &context) = 0;
	virtual int parse_hertz(const cmd_context &context) = 0;
	virtual int parse_attack(const cmd_context &context) = 0;
	virtual int parse_decay(const cmd_context &context) = 0;
	virtual int parse_release(const cmd_context &context) = 0;
	virtual int parse_sustain(const cmd_context &context) = 0;
	virtual int parse_volume(const cmd_context &context) = 0;
	virtual int parse_wave_type(const cmd_context &context) = 0;
	virtual int parse_attenuation(const cmd_context &context) = 0;
	virtual int parse_polyphony(const cmd_context &context) = 0;
	virtual int parse_scale_map(const cmd_context &context) = 0;

	virtual int flags() const = 0;
	virtual int base_hertz() const = 0;
	virtual int fm_volume() const = 0;

	virtual void set_flags(int flags) = 0;
	virtual void silence_xyz() = 0;
	virtual void set_speaker_beep() = 0;
};

extern SoundState &g_sound_state;

#endif
