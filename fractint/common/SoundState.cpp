#include <fstream>
#include <sstream>
#include <string>

#include <string.h>
#include <time.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "id.h"
#include "externs.h"
#include "helpdefs.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "realdos.h"

#include "SoundState.h"
#include "CommandParser.h"
#include "UIChoices.h"

enum
{
	KEYON = 0x20     // 0010 0000 key-on bit in regs b0 - b8 
};

class SoundStateImpl : public SoundState
{
public:
	SoundStateImpl();
	virtual ~SoundStateImpl();
	virtual void initialize();
	virtual bool open();
	virtual void close();
	virtual void tone(int tone);
	virtual void write_time();
	virtual int get_parameters();
	virtual void orbit(int x, int y);
	virtual void orbit(double x, double y, double z);

	virtual std::string parameter_text() const;
	virtual int parse_sound(const cmd_context &context);
	virtual int parse_hertz(const cmd_context &context);
	virtual int parse_attack(const cmd_context &context);
	virtual int parse_decay(const cmd_context &context);
	virtual int parse_release(const cmd_context &context);
	virtual int parse_sustain(const cmd_context &context);
	virtual int parse_volume(const cmd_context &context);
	virtual int parse_wave_type(const cmd_context &context);
	virtual int parse_attenuation(const cmd_context &context);
	virtual int parse_polyphony(const cmd_context &context);
	virtual int parse_scale_map(const cmd_context &context);

	virtual int flags() const				{ return _flags; }
	virtual int base_hertz() const			{ return _base_hertz; }
	virtual int fm_volume() const			{ return _fm_volume; }

	virtual void set_flags(int flags)		{ _flags = flags; }
	virtual void silence_xyz()				{ _flags &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); }
	virtual void set_speaker_beep()			{ _flags = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; }

private:
	enum
	{
		NUM_OCTAVES = 12
	};

	int _flags;
	int _base_hertz;				// sound=x/y/z hertz value 
	int _fm_attack;
	int _fm_decay;
	int _fm_release;
	int _fm_sustain;
	int _fm_volume;
	int _fm_wave_type;
	int _note_attenuation;
	int _polyphony;
	int _scale_map[NUM_OCTAVES];	// array for mapping notes to a (user defined) scale 

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
	int buzzer_pc_speaker(int tone)	{ return 0; }
	int sleep_ms(int delay)		{ return 0; }

	std::ofstream _fp;
	int _menu_count;
	int _fm_offset[NUM_CHANNELS];
	int _fm_channel;

	std::string _sound_save_name;
};

static SoundStateImpl s_sound_state;
SoundState &g_sound_state(s_sound_state);

static char offvoice = 0;
static unsigned char fmtemp[9];/*temporary vars used to store value used
   to mask keyoff bit in OPL registers as they're write only and the one
   with the keyoff bit also has a frequency multiplier in it fmtemp needed
   to make sure note stays the same when in the release portion of the
   envelope*/

/*
 * This program is not intended to explain all the aspects of FM sound
 * generation on Sound Blaster cards.  The chips are too complicated for
 * that.  This program is just to demonstrate how to generate a tone and
 * control the left and right channels.  For more information on the FM
 * synthesizer chip, contact Yamaha.
 *
 * Here's a brief description of FM:  Each sound is created by two operator
 * cells (called "slots" in the Yamaha documentation), a modulator and a
 * carrier.  When FM synthesis was invented, the output value of the
 * modulator affected the frequency of the carrier.  In the Yamaha chips, the
 * modulator output actually affects the phase of the carrier instead of
 * frequency, but this has a similar  effect.
 *
 * Normally the modulator and carrier would probably be connected in series
 * for complex sounds.  For this program, I wanted a pure sine wave, so I
 * connected them in parallel and turned the modulator output down all the
 * way and used just the carrier.
 *
 * Sound Blaster 1.0 - 2.0 cards have one OPL-2 FM synthesis chip at
 * addresses 2x8 and 2x9 (base + 8 and base + 9).  Sound Blaster Pro version
 * 1 cards (CT-1330) achieve stereo FM with two OPL-2 chips, one for each
 * speaker.  The left channel FM chip is at addresses 2x0 and 2x1.  The right
 * is at 2x2 and 2x3.  Addresses 2x8 and 2x9 address both chips
 * simultaneously, thus maintaining compatibility with the monaural Sound
 * Blaster cards.  The OPL-2 contains 18 operator cells which make up the
 * nine 2-operator channels.  Since the CT-1330 SB Pro has two OPL-2 chips,
 * it is therefore capable of generating 9 voices in each speaker.
 *
 * Sound Blaster Pro version 2 (CT-1600) and Sound Blaster 16 cards have one
 * OPL-3 stereo FM chip at addresses 2x0 - 2x3.  The OPL-3 is separated into
 * two "banks."  Ports 2x0 and 2x1 control bank 0, while 2x2 and 2x3 control
 * bank 1.  Each bank can generate nine 2-operator voices.  However, when the
 * OPL-3 is reset, it switches into OPL-2 mode.  It must be put into OPL-3
 * mode to use the voices in bank 1 or the stereo features.  For stereo
 * control, each channel may be sent to the left, the right, or both
 * speakers, controlled by two bits in registers C0H - C8H.
 *
 * The FM chips are controlled through a set of registers.  The following
 * table shows how operator cells and channels are related, and the register
 * offsets to use.
 *
 * FUNCTION  MODULATOR-  -CARRIER--  MODULATOR-  -CARRIER--  MODULATOR-  -CARRIER--
 * OP CELL    1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
 * CHANNEL    1   2   3   1   2   3   4   5   6   4   5   6   7   8   9   7   8   9
 * OFFSET    00  01  02  03  04  05  08  09  0A  0B  0C  0D  10  11  12  13  14  15
 *static unsigned char fm_offset[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
 * An example will make the use of this table clearer:  suppose you want to
 * set the attenuation of both of the operators of channel 4.  The KSL/TOTAL LEVEL
 * registers (which set the attenuation) are 40H - 55H.  The modulator for
 * channel 4 is op cell 7, and the carrier for channel 4 is op cell 10.  The
 * offsets for the modulator and carrier cells are 08H and 0BH, respectively.
 * Therefore, to set the attenuation of the modulator, you would output a
 * value to register 40H + 08H == 48H, and to set the carrier's attenuation,
 * you would output to register 40H + 0BH == 4BH.
 *
 * In this program, I used just channel 1, so the registers I used were 20H,
 * 40H, 60H, etc., and 23H, 43H, 63H, etc.
 *
 * The frequencies of each channel are controlled with a frequency number and
 * a multiplier.  The modulator and carrier of a channel both get the same
 * frequency number, but they may be given different multipliers.  Frequency
 * numbers are programmed in registers A0H - A8H (low 8 bits) and B0H - B8H
 * (high 2 bits).  Those registers control entire channels (2 operators), not
 * individual operator cells.  To turn a note on, the key-on bit in the
 * appropriate channel register is set.  Since these registers deal with
 * channels, you do not use the offsets listed in the table above.  Instead,
 * add (channel-1) to A0H or B0H.  For example, to turn channel 1 on,
 * program the frequency number in registers A0H and B0H, and set the key-on
 * bit to 1 in register B0H.  For channel 3, use registers A2H and B2H.
 *
 * Bits 2 - 4 in registers B0H - B8H are the block (octave) number for the
 * channel.
 *
 * Multipliers for each operator cell are programmed through registers 20H -
 * 35H.  The table below shows what multiple number to program into the
 * register to get the desired multiplier.  The multiple number goes into
 * bits 0 - 3 in the register.  Note that it's a bit strange at the end.
 *
 *   multiple number     multiplier        multiple number     multiplier
 *          0                1/2                   8               8
 *          1                 1                    9               9
 *          2                 2                    10              10
 *          3                 3                    11              10
 *          4                 4                    12              12
 *          5                 5                    13              12
 *          6                 6                    14              15
 *          7                 7                    15              15
 *
 * This equation shows how to calculate the required frequency number (to
 * program into registers A0H - A8H and B0H - B8H) to get the desired
 * frequency:
 *                fn=long(f) * 1048576/b/m /50000L
 * where f is the frequency in Hz,
 *       b is the block (octave) number between 0 and 7 inclusive, and
 *       m is the multiple number between 0 and 15 inclusive.
 */

SoundStateImpl::SoundStateImpl()
	: _fp(),
	_fm_channel(0),
	_sound_save_name("sound001.txt")
{
	initialize();
	for (int i = 0; i < NUM_CHANNELS; i++)
	{
		_fm_offset[i] = 0;
	}
}

SoundStateImpl::~SoundStateImpl()
{
}

void SoundStateImpl::initialize()
{
	_base_hertz = 440;						// basic hertz rate          
	_fm_volume = 63;						// full volume on soundcard o/p 
	_note_attenuation = ATTENUATE_NONE;	// no attenuation of hi notes 
	_fm_attack = 5;						// fast attack     
	_fm_decay = 10;						// long decay      
	_fm_sustain = 13;						// fairly high sustain level   
	_fm_release = 5;						// short release   
	_fm_wave_type = 0;						// sin wave 
	_polyphony = 0;						// no polyphony    
	for (int i = 0; i < NUM_OCTAVES; i++)
	{
		_scale_map[i] = i + 1;				// straight mapping of notes in octave 
	}
}

// open sound file 
bool SoundStateImpl::open()
{
	if ((g_orbit_save & ORBITSAVE_SOUND) != 0 && !_fp)
	{
		_fp.open(_sound_save_name.c_str(), std::ios::out);
		if (!_fp.is_open())
		{
			stop_message(STOPMSG_NORMAL, "Can't open " + _sound_save_name);
		}
		else
		{
			update_save_name(_sound_save_name);
		}
	}
	return _fp != 0;
}

void SoundStateImpl::close()
{
	if ((_flags & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP) // close sound write file 
	{
		if (_fp)
		{
			_fp.close();
		}
	}
}

/*
	This routine plays a tone in the speaker and optionally writes a file
	if the g_orbit_save variable is turned on
*/
void SoundStateImpl::tone(int tone)
{
	if ((g_orbit_save & ORBITSAVE_SOUND) != 0)
	{
		if (open())
		{
			_fp << boost::format("%-d\n") % tone;
		}
	}
	static int s_tab_or_help = 0;			// kludge for sound and tab or help key press 
	s_tab_or_help = 0;
	if (!driver_key_pressed())  // driver_key_pressed calls driver_sound_off() if TAB or F1 pressed 
	{
		// must not then call sound_off(), else indexes out of synch 
		if (driver_sound_on(tone))
		{
			wait_until(0, g_orbit_delay);
			if (!s_tab_or_help) // kludge because wait_until() calls driver_key_pressed 
			{
				driver_sound_off();
			}
		}
	}
}

void SoundStateImpl::write_time()
{
	if (open())
	{
		_fp << boost::format("time=%-ld\n") % (long(clock())*1000/CLK_TCK);
	}
}

int SoundStateImpl::sound_on(int freq)
{
	/* wrapper to previous fractint snd routine, uses fm synth or midi if
	available and selected */
	// Returns a 1 if sound is turned on, a 0 if not. 
	unsigned int mult;
	// clip to 5 Khz to match the limits set in asm routine that drives pc speaker 
	// and get rid of really silly bass notes too 
	if (freq > 5000 || freq < 20)
	{
		return 0;
	}

	// convert tone to note number for midi 
	double logbase = log(8.176);
	int note = int(12 * (log(double(freq)) - logbase)/log(2.0) + 0.5);

	int oct = (note/12) * 12; // round to nearest octave 
	int chrome = note % 12; // extract which note in octave it was 
	note = oct + _scale_map[chrome]; // remap using scale mapping array 

	if (_flags & SOUNDFLAG_QUANTIZED)
	{
		freq = int(exp((double(note)/12.0)*log(2.0))*8.176);
	}
	// pitch quantize note for FM and speaker 

	// fm flag set 
	if (_flags & SOUNDFLAG_OPL3_FM)
	{
		double temp_freq = double(freq)*1048576.0;
		unsigned int block = 0;
		mult = 1;
		unsigned int fn = int(temp_freq/(1 << block)/mult/50000.0);
		while (fn > 1023) // fn must have ten bit value so tweak mult and block until it fits 
		{
			if (block < 8)
			{
				block++;  // go up an octave 
			}
			else
			{
				mult++; // if we're out of octaves then increment the multiplier
				block = 0; // reset the block 
			}
			fn = int(temp_freq/((1 << block)*mult*50000.0));
		}

		// then send the right values to the fm registers 
		fm(0x23 + _fm_offset[_fm_channel], 0x20 | (mult & 0xF));
		// 0x20 sets sustained envelope, low nibble is multiply number 
		fm(0xA0 + _fm_channel, (fn & 0xFF));
		// save next to use as keyoff value 
		fmtemp[_fm_channel] = (unsigned char) (((fn >> 8) & 0x3) | (block << 2));
		fm(0xB0 + _fm_channel, fmtemp[_fm_channel] | KEYON);
		// then increment the channel number ready for the next note 
		if (++_fm_channel >= 9)
		{
			_fm_channel = 0;
		}
		// May have changed some parameters, put them in the registers. 
		initialize();
	}

	if (_flags & SOUNDFLAG_SPEAKER)
	{
		tone(freq);
	}
	return 1;
}

void SoundStateImpl::sound_off()
{
	if (_flags & SOUNDFLAG_OPL3_FM)// switch off old note 
	{
		if (offvoice >= 0)
		{
			fm(0xB0 + offvoice, fmtemp[offvoice]);
		}
		offvoice++;
		/* then increment channel number (letting old note die away properly prevents
		nasty clicks between notes as OPL has no zero crossing logic and switches
		frequencies immediately thus creating an easily audible glitch, especially 
		in bass notes... also allows chords :-) */
		if(offvoice >= 9)
		{
			offvoice = 0;
		}
	}
	if (_flags & SOUNDFLAG_SPEAKER)
	{
		driver_mute(); // shut off pc speaker 
	}
}

void SoundStateImpl::mute()
{
	// shut everything up routine 
	int i;
	if (_flags & SOUNDFLAG_OPL3_FM)
	{
		for (i=0;i<=8;i++)
		{
			fm(0xB0 + i, fmtemp[i]);
			fm(0x83 + _fm_offset[i], 0xFF);
		}
	}
	if (_flags & SOUNDFLAG_SPEAKER)
	{
		driver_mute(); // shut off pc speaker 
	}
	_fm_channel = 0;
	offvoice = (char) -_polyphony;
}

void SoundStateImpl::buzzer(int tone)
{
	if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_BEEP)
	{
		if (_flags & SOUNDFLAG_SPEAKER)
		{
			buzzer_pc_speaker(tone);
		}

		if (_flags & SOUNDFLAG_OPL3_FM)
		{
			int oldsoundflag = _flags;
			sleep_ms(1); // to allow quiet timer calibration first time 
			_flags &= ~SOUNDFLAG_SPEAKER; /*switch off sound_on stuff for pc spkr */
			switch(tone)
			{
			case 0:
				sound_on(1047);
				sleep_ms(1000);
				sound_off();
				sound_on(1109);
				sleep_ms(1000);
				sound_off();
				sound_on(1175);
				sleep_ms(1000);
				sound_off();
				break;
			case 1:
				sound_on(2093);
				sleep_ms(1000);
				sound_off();
				sound_on(1976);
				sleep_ms(1000);
				sound_off();
				sound_on(1857);
				sleep_ms(1000);
				sound_off();
				break;
			default:
				sound_on(40);
				sleep_ms(5000);
				sound_off();
				break;
			}
			_flags = oldsoundflag;
			mute(); /*switch off all currently sounding notes*/
		}

		/* must try better FM equiv..
		maybe some nicer noises like ping, burp, and bong :-) */
	}
}

int SoundStateImpl::get_parameters()
{
	// routine to get sound settings  
	int old_soundflag = _flags;
	int old_orbit_delay = g_orbit_delay;
	bool old_start_show_orbit = g_start_show_orbit;

get_sound_restart:
	{
		_menu_count = 0;
		UIChoices dialog(HELPSOUND, "Sound Control Screen", 255);
		const char *soundmodes[] = { "off", "beep", "x", "y", "z" };
		dialog.push("Sound (off, beep, x, y, z)", soundmodes, NUM_OF(soundmodes), _flags & SOUNDFLAG_ORBITMASK);
		dialog.push("Use PC internal speaker?", (_flags & SOUNDFLAG_SPEAKER) != 0);
		dialog.push("Use soundcard output?", (_flags & SOUNDFLAG_OPL3_FM) != 0);
		dialog.push("Quantize note pitch ?", (_flags & SOUNDFLAG_QUANTIZED) != 0);
		dialog.push("Orbit delay in ms (0 = none)", g_orbit_delay);
		dialog.push("Base Hz Value", _base_hertz);
		dialog.push("Show orbits?", g_start_show_orbit);
		dialog.push("");
		dialog.push("Press F6 for FM synth parameters, F7 for scale mappings");
		dialog.push("Press F4 to reset to default values");

		int i = dialog.prompt();
		if (i < 0)
		{
			_flags = old_soundflag;
			g_orbit_delay = old_orbit_delay;
			g_start_show_orbit = old_start_show_orbit;
			return -1; /*escaped */
		}

		int k = -1;
		_flags = dialog.values(++k).uval.ch.val;
		_flags += (dialog.values(++k).uval.ch.val * SOUNDFLAG_SPEAKER);
		_flags += (dialog.values(++k).uval.ch.val * SOUNDFLAG_OPL3_FM);
		_flags += (dialog.values(++k).uval.ch.val * SOUNDFLAG_QUANTIZED);
		g_orbit_delay = dialog.values(++k).uval.ival;
		_base_hertz = dialog.values(++k).uval.ival;
		g_start_show_orbit = (dialog.values(++k).uval.ch.val != 0);

		// now do any intialization needed and check for soundcard 
		if ((_flags & SOUNDFLAG_OPL3_FM) && !(old_soundflag & SOUNDFLAG_OPL3_FM))
		{
			initfm();
		}

		switch (i)
		{
		case IDK_F6:
			get_music_parameters();// see below, for controlling fmsynth 
			goto get_sound_restart;
			break;

		case IDK_F7:
			get_scale_map();// see below, for setting scale mapping 
			goto get_sound_restart;
			break;

		case IDK_F4:
			_flags = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; // reset to default 
			g_orbit_delay = 0;
			_base_hertz = 440;
			g_start_show_orbit = false;
			goto get_sound_restart;
			break;
		}

		return (_flags != old_soundflag
				&& ((_flags & SOUNDFLAG_ORBITMASK) > 1
					|| (old_soundflag & SOUNDFLAG_ORBITMASK) > 1)) ? 1 : 0;
	}
}

int SoundStateImpl::get_scale_map()
{
	_menu_count++;

get_map_restart:
	{
		UIChoices dialog(FIHELP_MUSIC, "Scale Mapping Screen", 255);
		dialog.push("Scale map C (1)", _scale_map[0]);
		dialog.push("Scale map C#(2)", _scale_map[1]);
		dialog.push("Scale map D (3)", _scale_map[2]);
		dialog.push("Scale map D#(4)", _scale_map[3]);
		dialog.push("Scale map E (5)", _scale_map[4]);
		dialog.push("Scale map F (6)", _scale_map[5]);
		dialog.push("Scale map F#(7)", _scale_map[6]);
		dialog.push("Scale map G (8)", _scale_map[7]);
		dialog.push("Scale map G#(9)", _scale_map[8]);
		dialog.push("Scale map A (10)", _scale_map[9]);
		dialog.push("Scale map A#(11)", _scale_map[10]);
		dialog.push("Scale map B (12)", _scale_map[11]);
		dialog.push("");
		dialog.push("Press F6 for FM synth parameters");
		dialog.push("Press F4 to reset to default values");

		int i = dialog.prompt();
		if (i < 0)
		{
			return -1;
		}

		for (int j = 0; j < 12; j++)
		{
			_scale_map[j] = abs(dialog.values(j).uval.ival);
			if (_scale_map[j] > 12)
			{
				_scale_map[j] = 12;
			}
		}

		if (i == IDK_F6 && _menu_count == 1)
		{
			get_music_parameters();// see below, for controling fmsynth 
			goto get_map_restart;
		}
		else if (i == IDK_F6 && _menu_count == 2)
		{
			_menu_count--;
		}

		if (i == IDK_F4)
		{
			for (int j = 0; j <= 11; j++)
			{
				_scale_map[j] = j + 1;
			}
			goto get_map_restart;
		}
	}

	return 0;
}

int SoundStateImpl::get_music_parameters()
{
	_menu_count++;

get_music_restart:
	{
		UIChoices dialog(FIHELP_MUSIC, "FM Synth Card Control Screen", 255);
		dialog.push("Polyphony 1..9", _polyphony + 1);
		dialog.push("Wave type 0..7", _fm_wave_type);
		dialog.push("Note attack time   0..15", _fm_attack);
		dialog.push("Note decay time    0..15", _fm_decay);
		dialog.push("Note sustain level 0..15", _fm_sustain);
		dialog.push("Note release time  0..15", _fm_release);
		dialog.push("Soundcard volume?  0..63", _fm_volume);
		const char *attenuation_modes[] = { "none", "low", "mid", "high" };
		dialog.push("Hi pitch attenuation", attenuation_modes, NUM_OF(attenuation_modes), _note_attenuation);
		dialog.push("");
		dialog.push("Press F7 for scale mappings");
		dialog.push("Press F4 to reset to default values");

		int i = dialog.prompt();
		if (i < 0)
		{
			return -1;
		}

		int k = -1;
		_polyphony = abs(dialog.values(++k).uval.ival - 1);
		if (_polyphony > 8)
		{
			_polyphony = 8;
		}
		_fm_wave_type =  (dialog.values(++k).uval.ival) & 0x07;
		_fm_attack =  (dialog.values(++k).uval.ival) & 0x0F;
		_fm_decay =  (dialog.values(++k).uval.ival) & 0x0F;
		_fm_sustain = (dialog.values(++k).uval.ival) & 0x0F;
		_fm_release = (dialog.values(++k).uval.ival) & 0x0F;
		_fm_volume = (dialog.values(++k).uval.ival) & 0x3F;
		_note_attenuation = dialog.values(++k).uval.ch.val;
		if (_flags & SOUNDFLAG_OPL3_FM)
		{
			initfm();
		}

		if (i == IDK_F7 && _menu_count == 1)
		{
			get_scale_map();// see above, for setting scale mapping 
			goto get_music_restart;
		}
		else if (i == IDK_F7 && _menu_count == 2)
		{
			_menu_count--;
		}

		if (i == IDK_F4)
		{
			_polyphony = 0;
			_fm_wave_type = 0;
			_fm_attack = 5;
			_fm_decay = 10;
			_fm_sustain = 13;
			_fm_release = 5;
			_fm_volume = 63;
			_note_attenuation = 0;
			if (_flags & SOUNDFLAG_OPL3_FM)
			{
				initfm();
			}
			goto get_music_restart;
		}
	}

	return 0;
}

void SoundStateImpl::old_orbit(int i, int j)
{
	if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X) // sound = x 
	{
		tone(int(i*1000/g_x_dots + _base_hertz));
	}
	else if ((_flags & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X) // sound = y or z 
	{
		tone(int(j*1000/g_y_dots + _base_hertz));
	}
	else if (g_orbit_delay > 0)
	{
		wait_until(0, g_orbit_delay);
	}
}

void SoundStateImpl::new_orbit(int i, int j)
{
	if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X) // sound = x 
	{
		tone(int(i + _base_hertz));
	}
	else if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y) // sound = y 
	{
		tone(int(j + _base_hertz));
	}
	else if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z) // sound = z 
	{
		tone(int(i + j + _base_hertz));
	}
	else if (g_orbit_delay > 0)
	{
		wait_until(0, g_orbit_delay);
	}
}

void SoundStateImpl::orbit(int i, int j)
{
	if (DEBUGMODE_OLD_ORBIT_SOUND == g_debug_mode)
	{
		old_orbit(i, j);
	}
	else
	{
		new_orbit(i, j);
	}
}

void SoundStateImpl::orbit(double x, double y, double z)
{
	if ((_flags & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
	{
		double value;
		switch (_flags & SOUNDFLAG_ORBITMASK)
		{
		case SOUNDFLAG_X: value = x; break;
		case SOUNDFLAG_Y: value = y; break;
		case SOUNDFLAG_Z: value = z; break;
		}
		tone(int(value*100 + _base_hertz));
	}
}

std::string SoundStateImpl::parameter_text() const
{
	std::ostringstream text;

	if (_base_hertz != DEFAULT_BASE_HERTZ)
	{
		text << " hertz=" << _base_hertz;
	}

	if (_flags != (SOUNDFLAG_BEEP | SOUNDFLAG_SPEAKER))
	{
		if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_OFF)
		{
			text << " sound=off";
		}
		else if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_BEEP)
		{
			text << " sound=beep";
		}
		else if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
		{
			text << " sound=x";
		}
		else if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
		{
			text << " sound=y";
		}
		else if ((_flags & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
		{
			text << " sound=z";
		}
		if ((_flags & SOUNDFLAG_ORBITMASK) && (_flags & SOUNDFLAG_ORBITMASK) <= SOUNDFLAG_Z)
		{
			if (_flags & SOUNDFLAG_SPEAKER)
			{
				text << "/pc";
			}
			if (_flags & SOUNDFLAG_OPL3_FM)
			{
				text << "/fm";
			}
			if (_flags & SOUNDFLAG_MIDI)
			{
				text << "/midi";
			}
			if (_flags & SOUNDFLAG_QUANTIZED)
			{
				text << "/quant";
			}
		}
	}

	if (_fm_volume != DEFAULT_FM_VOLUME)
	{
		text << " volume=" << _fm_volume;
	}

	switch (_note_attenuation)
	{
	case ATTENUATE_LOW:
		text << " attenuate=low";
		break;
	case ATTENUATE_MIDDLE:
		text << " attenuate=mid";
		break;
	case ATTENUATE_HIGH:
		text << " attenuate=high";
		break;
	}

	if (_polyphony != DEFAULT_POLYPHONY)
	{
		text << " polyphony=" << _polyphony + 1;
	}

	if (_fm_wave_type != DEFAULT_FM_WAVE_TYPE)
	{
		text << " wavetype=" << _fm_wave_type;
	}

	if (_fm_attack != DEFAULT_FM_ATTACK)
	{
		text << " attack=" << _fm_attack;
	}

	if (_fm_decay != DEFAULT_FM_DECAY)
	{
		text << " decay=" << _fm_decay;
	}

	if (_fm_sustain != DEFAULT_FM_SUSTAIN)
	{
		text << " sustain=" << _fm_sustain;
	}

	if (_fm_release != DEFAULT_FM_RELEASE)
	{
		text << " srelease=" << _fm_release;
	}

	if ((_flags & SOUNDFLAG_QUANTIZED) && !default_scale_map())  // quantize turned on 
	{
		text << " scalemap=" << _scale_map[0];
		for (int i = 1; i < NUM_OCTAVES; i++)
		{
			text << "/" << _scale_map[i];
		}
	}
	text << std::ends;

	return text.str().c_str();
}

bool SoundStateImpl::default_scale_map() const
{
	for (int i = 0; i < NUM_OCTAVES; i++)
	{
		if (_scale_map[i] != i + 1)
		{
			return false;
		}
	}
	return true;
}

int SoundStateImpl::parse_sound(const cmd_context &context)
{
	if (context.totparms > 5)
	{
		return bad_arg(context.curarg);
	}
	_flags = SOUNDFLAG_OFF; // start with a clean slate, add bits as we go 
	if (context.totparms == 1)
	{
		_flags = SOUNDFLAG_SPEAKER; // old command, default to PC speaker 
	}

	/* g_sound_flags is used as a bitfield... bit 0, 1, 2 used for whether sound
		is modified by an orbits x, y, or z component. and also to turn it on
		or off (0==off, 1==beep (or yes), 2==x, 3 == y, 4 == z),
		Bit 3 is used for flagging the PC speaker sound,
		Bit 4 for OPL3 FM soundcard output,
		Bit 5 will be for midi output (not yet),
		Bit 6 for whether the tone is quantised to the nearest 'proper' note
	(according to the western, even tempered system anyway) */

	if (context.charval[0] == 'n' || context.charval[0] == 'o')
	{
		_flags &= ~SOUNDFLAG_ORBITMASK;
	}
	else if ((strncmp(context.value, "ye", 2) == 0) || (context.charval[0] == 'b'))
	{
		_flags |= SOUNDFLAG_BEEP;
	}
	else if (context.charval[0] == 'x')
	{
		_flags |= SOUNDFLAG_X;
	}
	else if (context.charval[0] == 'y' && strncmp(context.value, "ye", 2) != 0)
	{
		_flags |= SOUNDFLAG_Y;
	}
	else if (context.charval[0] == 'z')
	{
		_flags |= SOUNDFLAG_Z;
	}
	else
	{
		return bad_arg(context.curarg);
	}
	if (context.totparms > 1)
	{
		_flags &= SOUNDFLAG_ORBITMASK; // reset options 
		for (int i = 1; i < context.totparms; i++)
		{
			// this is for 2 or more options at the same time 
			if (context.charval[i] == 'f')  // (try to)switch on opl3 fm synth 
			{
				if (driver_init_fm())
				{
					_flags |= SOUNDFLAG_OPL3_FM;
				}
				else
				{
					_flags &= ~SOUNDFLAG_OPL3_FM;
				}
			}
			else if (context.charval[i] == 'p')
			{
				_flags |= SOUNDFLAG_SPEAKER;
			}
			else if (context.charval[i] == 'm')
			{
				_flags |= SOUNDFLAG_MIDI;
			}
			else if (context.charval[i] == 'q')
			{
				_flags |= SOUNDFLAG_QUANTIZED;
			}
			else
			{
				return bad_arg(context.curarg);
			}
		}
	}
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_hertz(const cmd_context &context)
{
	_base_hertz = context.numval;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_volume(const cmd_context &context)
{
	_fm_volume = (context.numval > 63) ? 63 : context.numval;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_attenuation(const cmd_context &context)
{
	if (context.charval[0] == 'n')
	{
		_note_attenuation = ATTENUATE_NONE;
	}
	else if (context.charval[0] == 'l')
	{
		_note_attenuation = ATTENUATE_LOW;
	}
	else if (context.charval[0] == 'm')
	{
		_note_attenuation = ATTENUATE_MIDDLE;
	}
	else if (context.charval[0] == 'h')
	{
		_note_attenuation = ATTENUATE_HIGH;
	}
	else
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_polyphony(const cmd_context &context)
{
	if (context.numval > 9)
	{
		return bad_arg(context.curarg);
	}
	_polyphony = abs(context.numval-1);
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_wave_type(const cmd_context &context)
{
	_fm_wave_type = context.numval & 0x0F;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_attack(const cmd_context &context)
{
	_fm_attack = context.numval & 0x0F;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_decay(const cmd_context &context)
{
	_fm_decay = context.numval & 0x0F;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_sustain(const cmd_context &context)
{
	_fm_sustain = context.numval & 0x0F;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_release(const cmd_context &context)
{
	_fm_release = context.numval & 0x0F;
	return COMMANDRESULT_OK;
}

int SoundStateImpl::parse_scale_map(const cmd_context &context)
{
	int counter;
	if (context.totparms != context.intparms)
	{
		return bad_arg(context.curarg);
	}
	for (counter = 0; counter <= 11; counter++)
	{
		if ((context.totparms > counter) && (context.intval[counter] > 0)
			&& (context.intval[counter] < 13))
		{
			_scale_map[counter] = context.intval[counter];
		}
	}
	return COMMANDRESULT_OK;
}
