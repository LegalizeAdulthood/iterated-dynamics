#include <time.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"
#include "externs.h"
#include "helpdefs.h"
#include "fihelp.h"
#include "SoundState.h"
#include "drivers.h"

#define KEYON    0x20     /* 0010 0000 key-on bit in regs b0 - b8 */

SoundState g_sound_state;

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
 *                fn=(long)f * 1048576 / b / m /50000L
 * where f is the frequency in Hz,
 *       b is the block (octave) number between 0 and 7 inclusive, and
 *       m is the multiple number between 0 and 15 inclusive.
 */

SoundState::SoundState()
	: m_fp(NULL),
	m_fm_channel(0)
{
	initialize();
	for (int i = 0; i < NUM_CHANNELS; i++)
	{
		m_fm_offset[i] = 0;
	}
}

void SoundState::initialize()
{
	m_base_hertz = 440;						/* basic hertz rate          */
	m_fm_volume = 63;						/* full volume on soundcard o/p */
	m_note_attenuation = ATTENUATE_NONE;	/* no attenuation of hi notes */
	m_fm_attack = 5;						/* fast attack     */
	m_fm_decay = 10;						/* long decay      */
	m_fm_sustain = 13;						/* fairly high sustain level   */
	m_fm_release = 5;						/* short release   */
	m_fm_wave_type = 0;						/* sin wave */
	m_polyphony = 0;						/* no polyphony    */
	for (int i = 0; i < NUM_OCTAVES; i++)
	{
		m_scale_map[i] = i + 1;				/* straight mapping of notes in octave */
	}
}

/* open sound file */
int SoundState::open()
{
	static char soundname[] = {"sound001.txt"};
	if ((g_orbit_save & ORBITSAVE_SOUND) != 0 && m_fp == NULL)
	{
		m_fp = fopen(soundname, "w");
		if (m_fp == NULL)
		{
			stop_message(0, "Can't open SOUND*.TXT");
		}
		else
		{
			update_save_name(soundname);
		}
	}
	return m_fp != NULL;
}

void SoundState::close()
{
	if ((m_flags & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP) /* close sound write file */
	{
		if (m_fp)
		{
			fclose(m_fp);
		}
		m_fp = NULL;
	}
}

/*
	This routine plays a tone in the speaker and optionally writes a file
	if the g_orbit_save variable is turned on
*/
void SoundState::tone(int tone)
{
	if ((g_orbit_save & ORBITSAVE_SOUND) != 0)
	{
		if (open())
		{
			fprintf(m_fp, "%-d\n", tone);
		}
	}
	static int s_tab_or_help = 0;			/* kludge for sound and tab or help key press */
	s_tab_or_help = 0;
	if (!driver_key_pressed())  /* driver_key_pressed calls driver_sound_off() if TAB or F1 pressed */
	{
		/* must not then call sound_off(), else indexes out of synch */
		if (driver_sound_on(tone))
		{
			wait_until(0, g_orbit_delay);
			if (!s_tab_or_help) /* kludge because wait_until() calls driver_key_pressed */
			{
				driver_sound_off();
			}
		}
	}
}

void SoundState::write_time()
{
	if (open())
	{
		fprintf(m_fp, "time=%-ld\n", (long)clock()*1000/CLK_TCK);
	}
}

int SoundState::sound_on(int freq)
{
	/* wrapper to previous fractint snd routine, uses fm synth or midi if
	available and selected */
	/* Returns a 1 if sound is turned on, a 0 if not. */
	unsigned int mult;
	/* clip to 5 Khz to match the limits set in asm routine that drives pc speaker */
	/* and get rid of really silly bass notes too */
	if (freq > 5000 || freq < 20)
	{
		return 0;
	}

	/* convert tone to note number for midi */
	double logbase = log(8.176);
	int note = (int)(12 * (log(static_cast<double>(freq)) - logbase) / log(2.0) + 0.5);

	int oct = (note / 12) * 12; /* round to nearest octave */
	int chrome = note % 12; /* extract which note in octave it was */
	note = oct + m_scale_map[chrome]; /* remap using scale mapping array */

	if (m_flags & 64)
	{
		freq = (int) (exp(((double) note/12.0)*log(2.0))*8.176);
	}
	/* pitch quantize note for FM and speaker */

	/* fm flag set */
	if (m_flags & 16)
	{
		double temp_freq = (double)freq * (double)1048576;
		unsigned int block = 0;
		mult = 1;
		unsigned int fn = (int)(temp_freq / (1 << block) / mult / 50000.0);
		while (fn > 1023) /* fn must have ten bit value so tweak mult and block until it fits */
		{
			if (block < 8)
			{
				block++;  /* go up an octave */
			}
			else
			{
				mult++; /* if we're out of octaves then increment the multiplier*/
				block = 0; /* reset the block */
			}
			fn = (int) (temp_freq/((1 << block)*mult*50000.0));
		}

		/* then send the right values to the fm registers */
		fm(0x23 + m_fm_offset[m_fm_channel], 0x20 | (mult & 0xF));
		/* 0x20 sets sustained envelope, low nibble is multiply number */
		fm(0xA0 + m_fm_channel, (fn & 0xFF));
		/* save next to use as keyoff value */
		fmtemp[m_fm_channel] = (unsigned char) (((fn >> 8) & 0x3) | (block << 2));
		fm(0xB0 + m_fm_channel, fmtemp[m_fm_channel] | KEYON);
		/* then increment the channel number ready for the next note */
		if (++m_fm_channel >= 9)
		{
			m_fm_channel = 0;
		}
		/* May have changed some parameters, put them in the registers. */
		initialize();
	}

	if (m_flags & 8)
	{
		tone(freq); /* pc spkr flag set */
	}
	return 1;
}

void SoundState::sound_off()
{
	if (m_flags & 16)/* switch off old note */
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
	if (m_flags & 8)
	{
		driver_mute(); /* shut off pc speaker */
	}
}

void SoundState::mute()
{
	/* shut everything up routine */
	int i;
	if (m_flags & 16)
	{
		for (i=0;i<=8;i++)
		{
			fm(0xB0 + i, fmtemp[i]);
			fm(0x83 + m_fm_offset[i], 0xFF);
		}
	}
	if (m_flags & 8)
	{
		driver_mute(); /* shut off pc speaker */
	}
	m_fm_channel = 0;
	offvoice = (char) -m_polyphony;
}

void SoundState::buzzer(int tone)
{
	if ((m_flags & 7) == 1)
	{
		if (m_flags & 8)
		{
			buzzerpcspkr(tone);
		}

		if (m_flags & 16)
		{
			int oldsoundflag = m_flags;
			sleepms(1); /* to allow quiet timer calibration first time */
			m_flags = m_flags & 0xf7; /*switch off sound_on stuff for pc spkr */
			switch(tone)
			{
			case 0:
				sound_on(1047);
				sleepms(1000);
				sound_off();
				sound_on(1109);
				sleepms(1000);
				sound_off();
				sound_on(1175);
				sleepms(1000);
				sound_off();
				break;
			case 1:
				sound_on(2093);
				sleepms(1000);
				sound_off();
				sound_on(1976);
				sleepms(1000);
				sound_off();
				sound_on(1857);
				sleepms(1000);
				sound_off();
				break;
			default:
				sound_on(40);
				sleepms(5000);
				sound_off();
				break;
			}
			m_flags = oldsoundflag;
			mute(); /*switch off all currently sounding notes*/
		}

		/* must try better FM equiv..
		maybe some nicer noises like ping, burp, and bong :-) */
	}
}

int SoundState::get_parameters()
{
	/* routine to get sound settings  */
	char *choices[15];
	struct full_screen_values uvalues[15];
	int old_soundflag = m_flags;
	int old_orbit_delay = g_orbit_delay;
	char old_start_showorbit = g_start_show_orbit;

	/* m_flags bits 0..7 used as thus:
	bit 0, 1, 2 controls sound beep/off and x, y, z
	(0 == off 1 == beep, 2 == x, 3 == y, 4 == z)
	bit 3 controls PC speaker
	bit 4 controls sound card OPL3 FM sound
	bit 5 controls midi output
	bit 6 controls pitch quantise
	bit 7 free! */
get_sound_restart:
	m_menu_count = 0;
	int k = -1;

	choices[++k] = "Sound (off, beep, x, y, z)";
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 4;
	uvalues[k].uval.ch.llen = 5;
	char *soundmodes[] = { "off", "beep", "x", "y", "z" };
	uvalues[k].uval.ch.list = soundmodes;
	uvalues[k].uval.ch.val = m_flags & 7;

	choices[++k] = "Use PC internal speaker?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = (m_flags & 8) ? 1 : 0;

	choices[++k] = "Use soundcard output?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = (m_flags & 16) ? 1 : 0;

	choices[++k] = "Quantize note pitch ?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = (m_flags & 64) ? 1 : 0;

	choices[++k] = "Orbit delay in ms (0 = none)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_orbit_delay;

	choices[++k] = "Base Hz Value";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_base_hertz;

	choices[++k] = "Show orbits?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_start_show_orbit;

	choices[++k] = "";
	uvalues[k].type = '*';

	choices[++k] = "Press F6 for FM synth parameters, F7 for scale mappings";
	uvalues[k].type = '*';

	choices[++k] = "Press F4 to reset to default values";
	uvalues[k].type = '*';

	int i = full_screen_prompt_help(HELPSOUND, "Sound Control Screen", k + 1, choices, uvalues, 255, NULL);
	if (i < 0)
	{
		m_flags = old_soundflag;
		g_orbit_delay = old_orbit_delay;
		g_start_show_orbit = old_start_showorbit;
		return -1; /*escaped */
	}

	k = -1;

	m_flags = uvalues[++k].uval.ch.val;
	m_flags = m_flags + (uvalues[++k].uval.ch.val * 8);
	m_flags = m_flags + (uvalues[++k].uval.ch.val * 16);
	m_flags = m_flags + (uvalues[++k].uval.ch.val * 64);
	g_orbit_delay = uvalues[++k].uval.ival;
	m_base_hertz = uvalues[++k].uval.ival;
	g_start_show_orbit = (char) uvalues[++k].uval.ch.val;

	/* now do any intialization needed and check for soundcard */
	if ((m_flags & 16) && !(old_soundflag & 16))
	{
		initfm();
	}

	if (i == FIK_F6)
	{
		get_music_parameters();/* see below, for controling fmsynth */
		goto get_sound_restart;
	}

	if (i == FIK_F7)
	{
		get_scale_map();/* see below, for setting scale mapping */
		goto get_sound_restart;
	}

	if (i == FIK_F4)
	{
		m_flags = 9; /* reset to default */
		g_orbit_delay = 0;
		m_base_hertz = 440;
		g_start_show_orbit = 0;
		goto get_sound_restart;
	}

	if (m_flags != old_soundflag && ((m_flags & 7) > 1 || (old_soundflag & 7) > 1))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int SoundState::get_scale_map()
{
	char *choices[15];
	struct full_screen_values uvalues[15];
	int k;
	int i, j;

	m_menu_count++;

get_map_restart:
	k = -1;

	choices[++k] = "Scale map C (1)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[0];

	choices[++k] = "Scale map C#(2)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[1];

	choices[++k] = "Scale map D (3)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[2];

	choices[++k] = "Scale map D#(4)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[3];

	choices[++k] = "Scale map E (5)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[4];

	choices[++k] = "Scale map F (6)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[5];

	choices[++k] = "Scale map F#(7)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[6];

	choices[++k] = "Scale map G (8)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[7];

	choices[++k] = "Scale map G#(9)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[8];

	choices[++k] = "Scale map A (10)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[9];

	choices[++k] = "Scale map A#(11)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[10];

	choices[++k] = "Scale map B (12)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_scale_map[11];

	choices[++k] = "";
	uvalues[k].type = '*';

	choices[++k] = "Press F6 for FM synth parameters";
	uvalues[k].type = '*';

	choices[++k] = "Press F4 to reset to default values";
	uvalues[k].type = '*';

	i = full_screen_prompt_help(HELPMUSIC, "Scale Mapping Screen", k + 1, choices, uvalues, 255, NULL);
	if (i < 0)
	{
		return -1;
	}

	k = -1;

	for (j = 0; j <= 11; j++)
	{
		m_scale_map[j] = abs(uvalues[++k].uval.ival);
		if (m_scale_map[j] > 12)
		{
			m_scale_map[j] = 12;
		}
	}

	if (i == FIK_F6 && m_menu_count == 1)
	{
		get_music_parameters();/* see below, for controling fmsynth */
		goto get_map_restart;
	}
	else if (i == FIK_F6 && m_menu_count == 2)
	{
		m_menu_count--;
	}

	if (i == FIK_F4)
	{
		for (j = 0; j <= 11; j++)
		{
			m_scale_map[j] = j + 1;
		}
		goto get_map_restart;
	}

	return 0;
}

int SoundState::get_music_parameters()
{
	char *attenmodes[] = { "none", "low", "mid", "high" };
	char *choices[11];
	struct full_screen_values uvalues[11];
	int i;

	m_menu_count++;
	get_music_restart:
	int k = -1;

	choices[++k] = "Polyphony 1..9";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_polyphony + 1;

	choices[++k] = "Wave type 0..7";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_fm_wave_type;

	choices[++k] = "Note attack time   0..15";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_fm_attack;

	choices[++k] = "Note decay time    0..15";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_fm_decay;

	choices[++k] = "Note sustain level 0..15";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_fm_sustain;

	choices[++k] = "Note release time  0..15";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_fm_release;

	choices[++k] = "Soundcard volume?  0..63";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = m_fm_volume ;

	choices[++k] = "Hi pitch attenuation";
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 4;
	uvalues[k].uval.ch.llen = 4;
	uvalues[k].uval.ch.list = attenmodes;
	uvalues[k].uval.ch.val = m_note_attenuation;

	choices[++k] = "";
	uvalues[k].type = '*';
	choices[++k] = "Press F7 for scale mappings";
	uvalues[k].type = '*';
	choices[++k] = "Press F4 to reset to default values";
	uvalues[k].type = '*';

	i = full_screen_prompt_help(HELPMUSIC, "FM Synth Card Control Screen", k + 1, choices, uvalues, 255, NULL);
	if (i < 0)
	{
		return -1;
	}

	k = -1;
	m_polyphony = abs(uvalues[++k].uval.ival - 1);
	if (m_polyphony > 8)
	{
		m_polyphony = 8;
	}
	m_fm_wave_type =  (uvalues[++k].uval.ival) & 0x07;
	m_fm_attack =  (uvalues[++k].uval.ival) & 0x0F;
	m_fm_decay =  (uvalues[++k].uval.ival) & 0x0F;
	m_fm_sustain = (uvalues[++k].uval.ival) & 0x0F;
	m_fm_release = (uvalues[++k].uval.ival) & 0x0F;
	m_fm_volume = (uvalues[++k].uval.ival) & 0x3F;
	m_note_attenuation = uvalues[++k].uval.ch.val;
	if (m_flags & 16)
	{
		initfm();
	}

	if (i == FIK_F7 && m_menu_count == 1)
	{
		get_scale_map();/* see above, for setting scale mapping */
		goto get_music_restart;
	}
	else if (i == FIK_F7 && m_menu_count == 2)
	{
		m_menu_count--;
	}

	if (i == FIK_F4)
	{
		m_polyphony = 0;
		m_fm_wave_type = 0;
		m_fm_attack = 5;
		m_fm_decay = 10;
		m_fm_sustain = 13;
		m_fm_release = 5;
		m_fm_volume = 63;
		m_note_attenuation = 0;
		if (m_flags & 16)
		{
			initfm();
		}
		goto get_music_restart;
	}

	return 0;
}
