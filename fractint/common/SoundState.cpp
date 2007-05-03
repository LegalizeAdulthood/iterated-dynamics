#include <time.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"
#include "externs.h"
#include "SoundState.h"
#include "drivers.h"

SoundState g_sound_state;

SoundState::SoundState()
	: m_fp(NULL)
{
	initialize();
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
		/* must not then call soundoff(), else indexes out of synch */
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
