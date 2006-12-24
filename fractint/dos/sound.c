/* sound functions,

 To use sound in fractint there are three basic functions:
 soundon(freq) starts a voice sounding with the given frequency (in Hz)
 soundoff() releases a voice 
 mute() turns off all voices at once
 polyhony controls how many voices are allowed to sound at once (up to nine)

 for more gory detals see below:

 RB 20/3/99 */



/* sound.c largely pinched from fmsample.c as distributed with the creative labs */
/* SDK.... see below.. RB */

/* -------------------------------------------------------------------------- */
/*                                                                            */
/* (C) Copyright Creative Technology Ltd 1994-1996. All right reserved        */
/*                                                                            */
/* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      */
/* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      */
/* PURPOSE.                                                                   */
/*                                                                            */
/* You have a royalty-free right to use, modify, reproduce and                */
/* distribute the Sample Files (and/or any modified version) in               */
/* any way you find useful, provided that you agree that                      */
/* Creative has no warranty obligations or liability for any Sample Files.    */
/*                                                                            */
/*----------------------------------------------------------------------------*/

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
 *static unsigned char fm_offset[9] = {0,1,2,8,9,10,16,17,18};
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


#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "externs.h"
#include "helpdefs.h"

#define STEREO         /* Define this for SBPro CT-1330 or later card. */
#define OPL3           /* Also define this for SBPro CT-1600 or later card. */
#define KEYON    0x20     /* 0010 0000 key-on bit in regs b0 - b8 */
#define LEFT     0x10
#define RIGHT    0x20
#define KEYOFF   0xDF     /* 1101 1111 key-off */

 /* These are offsets from the base I/O address. */
#define FM       8        /* SB (mono) ports (e.g. 228H and 229H) */
#define PROFM1   0        /* On CT-1330, this is left OPL-2.  On CT-1600 and */
                          /* later cards, it's OPL-3 bank 0. */
#define PROFM2   2        /* On CT-1330, this is right OPL-2.  On CT-1600 and */
                          /* later cards, it's OPL-3 bank 1. */

int menu2;
unsigned int IOport;        /* Sound Blaster port address */
static int get_music_parms(void);
static int get_scale_map(void);

static int base16(char **str, unsigned *val)
/* Takes a double pointer to a string, interprets the characters as a
 * base-16 number, and advances the pointer.
 * Returns 0 if successful, 1 if not.
 */
{
   char c;
   int digit;
   *val = 0;

   while ( **str != ' ') {
      c = (char)toupper(**str);
      if (c >= '0' && c <= '9')
         digit = c - '0';
      else if (c >= 'A' && c <= 'F')
         digit = c - 'A' + 10;
      else
         return 1;          /* error in string */

      *val = *val * 16 + digit;
      (*str)++;
   }
   return 0;
}

static unsigned ReadBlasterEnv(unsigned *port)
/* Gets the Blaster environment statement and stores the values in the
 * variables whose addresses were passed to it.
 * Returns:
 *   0  if successful
 *   1  if there was an error reading the port address.
 */
 /* RB trimmed down just to grab the ioport as that's all we're interested in */
{
   char     *env;
   env = getenv("BLASTER");
   while (*env) {
      switch(toupper( *env )) {
         case 'A':
            env++;
            if (base16(&env, port))     /* interpret port value as hex */
               return 1;       /* error */
            break;
         default:
            env++;
            break;
      }
   return(0);
   }

   return 1; /*RB if we got here then there was no port value*/
}

static void FMoutput(unsigned port, int reg, int val)
/* This outputs a value to a specified FM register at a specified FM port. */
{
   outp(port, reg);
   sleepms(1);         /* need to wait at least 3.3 microsec */
   outp(port+1, val);
   sleepms(1);         /* need to wait at least 23 microsec */
}

static void fm(int reg, int val)
/* This function outputs a value to a specified FM register at the Sound
 * Blaster (mono) port address.
 */
{
   FMoutput(IOport+FM, reg, val);
}

#if 0
static void Profm1(int reg, int val)
/* This function outputs a value to a specified FM register at the Sound
 * Blaster Pro left FM port address (or OPL-3 bank 0).
 */
{
   FMoutput(IOport+PROFM1, reg, val);
}
#endif

static void Profm2(int reg, int val)
/* This function outputs a value to a specified FM register at the Sound
 * Blaster Pro right FM port address (or OPL-3 bank 1).
 */
{
   FMoutput(IOport+PROFM2, reg, val);
}

/*  here endeth the straight stuff from creative labs, RB */

static unsigned char fmtemp[9];/*temporary vars used to store value used
   to mask keyoff bit in OPL registers as they're write only and the one
   with the keyoff bit also has a frequency multiplier in it fmtemp needed
   to make sure note stays the same when in the release portion of the
   envelope*/

/* offsets of registers for each channel in the fm synth */
static unsigned char fm_offset[9] = {0,1,2,8,9,10,16,17,18};
static char fm_channel=0;/* keeps track of the channel number being used in
   polyphonic operation */
int fm_attack;
int fm_decay;
int fm_sustain;
int fm_release;
int fm_vol;
int fm_wavetype;
int polyphony;
int hi_atten;
static char offvoice = 0;

void initsndvars(void)
{
int k, b;
int tmp_atten = hi_atten; /* stupid kludge needed to make attenuation work */
   if (hi_atten == 1)     /* should be:  00 -> none    but is:  00 -> none */
      tmp_atten = 2;      /*             01 -> low              01 -> mid  */
   if (hi_atten == 2)     /*             10 -> mid              10 -> low  */
      tmp_atten = 1;      /*             11 -> high             11 -> high */

   for(k=0;k<9;k++) {
   /***************************************
     * Set parameters for the carrier cells *
    ***************************************/

      fm(0x43+fm_offset[k],(63-fm_vol) | (tmp_atten << 6));
     /* volume decrease with pitch (D7-D6),
    * attenuation (D5-D0)
    */
      b = ((15 - fm_attack)*0x10) | (15 - fm_decay);
      fm(0x63+fm_offset[k],b);
      /* attack (D7-D4) and decay (D3-D0) */
      b = ((15 - fm_sustain) * 0x10) | (15 - fm_release);
      fm(0x83+fm_offset[k],b);
      /* high sustain level (D7-D4=0), release rate (D3-D0) */ 
      fm(0xE3+fm_offset[k],fm_wavetype);
   }
}

int initfm(void)
{
int k;
   k = ReadBlasterEnv(&IOport);
   if (k == 1) { /* BLASTER environment variable not set */
      static char msg[] = {"No sound hardware or Blaster variable not set"};
      soundflag = (soundflag & 0xef); /* 1110 1111 */
      stopmsg(0,msg);
      return(0); /* no card found */
   }
   fm(1,0x00);       /* must initialize this to zero */
   Profm2(5, 1);  /* set to OPL3 mode, necessary for stereo */
   Profm2(4, 0);  /* make sure 4-operator mode is disabled */
   fm(1,0x20);   /* set bit five to allow wave shape other than sine */
   for(k=0;k<9;k++) {
     fm(0xC0+k,LEFT | RIGHT | 1);     /* set both channels, parallel connection */

    /*****************************************
     * Set parameters for the modulator cells *
    *****************************************/
/* these don't change once they are set */
     fm(0x20+fm_offset[k],0x21);
     /* sustained envelope type, frequency multiplier=1    */
     fm(0x40+fm_offset[k],0x3f);
     /* maximum attenuation, no volume decrease with pitch */
    /* Since the modulator signal is attenuated as much as possible, these
    * next two values shouldn't have any effect.
    */
     fm(0x60+fm_offset[k],0x44);
     /* slow attack and decay */
     fm(0x80+fm_offset[k],0x05); 
     /* high sustain level, slow release rate */
   }
   initsndvars();
   fm_channel = 0;
   offvoice = (char)(-polyphony);
   return(1); /*all sucessfully initialised ok */
}


int soundon(int freq)
{
 /* wrapper to previous fractint snd routine, uses fm synth or midi if
    available and selected */
/* Returns a 1 if sound is turned on, a 0 if not. */
 int note,oct,chrome;
 unsigned int block,mult,fn; 
 double logbase = log(8.176);

 /* clip to 5 Khz to match the limits set in asm routine that drives pc speaker*/
   if (freq > 5000) return(0);
   if(freq<20) return(0);/* and get rid of really silly bass notes too */

 /* convert tone to note number for midi */
   note =(int)(12 * (log(freq) - logbase) / log(2.0) + 0.5);

   oct = (note / 12) * 12; /* round to nearest octave */
   chrome = note % 12; /* extract which note in octave it was */
   note = oct + scale_map[chrome]; /* remap using scale mapping array */

   if (soundflag & 64)
      freq=(int)(exp(((double)note/12.0)*log(2.0))*8.176);
         /* pitch quantize note for FM and speaker */

   if (soundflag & 16) { /* fm flag set */
      double temp_freq = (double)freq * (double)1048576;
      block = 0;
      mult = 1;
      fn=(int)(temp_freq / (1 << block) / mult / 50000.0);
      while(fn >1023) { /* fn must have ten bit value so tweak mult and block until it fits */
         if (block < 8) block++;  /* go up an octave */
         else {
            mult++; /* if we're out of octaves then increment the multiplier*/
            block = 0; /* reset the block */
         }
         fn=(int)(temp_freq / (1 << block) / mult / 50000.0);
      }
   
/*    printf("on: fn = %i chn= %i  blk = %i  mlt = %i ofs = %i ",fn,fm_channel,block,mult,fm_offset[fm_channel]);
     getch(); */
   /* then send the right values to the fm registers */
     fm(0x23+fm_offset[fm_channel],0x20 | (mult & 0xF));
   /* 0x20 sets sustained envelope, low nibble is multiply number */
     fm(0xA0+fm_channel,(fn & 0xFF));
   /* save next to use as keyoff value */
     fmtemp[fm_channel] = (unsigned char)(((fn >> 8) & 0x3) | (block << 2));
     fm(0xB0+fm_channel,fmtemp[fm_channel] | KEYON);
   /* then increment the channel number ready for the next note */
 /*     printf(" fmtemp = %i \n ",fmtemp[fm_channel]); */
     if (++fm_channel >= 9) fm_channel = 0;
   /* May have changed some parameters, put them in the registers. */
     initsndvars();
   }

   if (soundflag & 8) snd(freq); /* pc spkr flag set */
   return(1);
}


void soundoff(void)
{
   if (soundflag & 16) {/* switch off old note */
      if(offvoice >= 0){
/*        printf("off: ofv= %i tmp = %i \n",offvoice,fmtemp[offvoice]);
        getch(); */     
        fm(0xB0+offvoice,fmtemp[offvoice]);
       }
      offvoice++;
 /* then increment channel number (letting old note die away properly prevents
 nasty clicks between notes as OPL has no zero crossing logic and switches
 frequencies immediately thus creating an easily audible glitch, especially 
 in bass notes... also allows chords :-) */
      if(offvoice >= 9) offvoice = 0;
   }
   if (soundflag & 8) nosnd(); /* shut off pc speaker */
}


void mute()
{
/* shut everything up routine */
int i;
if(soundflag & 16)
  for (i=0;i<=8;i++) {
    fm(0xB0+i,fmtemp[i]);
    fm(0x83+fm_offset[i],0xFF);
  }
if (soundflag & 8) nosnd(); /* shut off pc speaker */
fm_channel = 0;
offvoice = (char)(-polyphony);
}


void buzzer(int tone)
{
 if((soundflag & 7) == 1) {

   if (soundflag & 8) buzzerpcspkr(tone);

   if (soundflag & 16) {
     int oldsoundflag = soundflag;
     sleepms(1); /* to allow quiet timer calibration first time */
     soundflag = soundflag & 0xf7; /*switch off soundon stuff for pc spkr */
     switch(tone) {
     case 0:
       soundon(1047);
       sleepms(1000);
       soundoff();
       soundon(1109);
       sleepms(1000);
       soundoff();
       soundon(1175);
       sleepms(1000);
       soundoff();
       break;
     case 1:
       soundon(2093);
       sleepms(1000);
       soundoff();
       soundon(1976);
       sleepms(1000);
       soundoff();
       soundon(1857);
       sleepms(1000);
       soundoff();
       break;
     default:
       soundon(40);
       sleepms(5000);
       soundoff();
       break;
     }
     soundflag = oldsoundflag;
     mute(); /*switch off all currently sounding notes*/
 
   }

   /* must try better FM equiv..
      maybe some nicer noises like ping, burp, and bong :-) */

 }
}

#define LOADCHOICES(X)     {\
   static char tmp[] = { X };\
   strcpy(ptr,(char *)tmp);\
   choices[++k]= ptr;\
   ptr += sizeof(tmp);\
   }

int get_sound_params(void)
{
/* routine to get sound settings  */
static char o_hdg[] = {"Sound Control Screen"};
char *soundmodes[] = {s_off,s_beep,s_x,s_y,s_z};
int old_soundflag,old_orbit_delay;
char hdg[sizeof(o_hdg)];
char *choices[15];
char *ptr;
struct fullscreenvalues uvalues[15];
int k;
int i;
int oldhelpmode;
char old_start_showorbit;

oldhelpmode = helpmode;
old_soundflag = soundflag;
old_orbit_delay = orbit_delay;
old_start_showorbit = start_showorbit;

/* soundflag bits 0..7 used as thus:
   bit 0,1,2 controls sound beep/off and x,y,z
      (0 == off 1 == beep, 2 == x, 3 == y, 4 == z)
   bit 3 controls PC speaker
   bit 4 controls sound card OPL3 FM sound
   bit 5 controls midi output
   bit 6 controls pitch quantise
   bit 7 free! */
get_sound_restart:
   menu2 = 0;
   k = -1;
   strcpy(hdg,o_hdg);
   ptr = (char *)extraseg;

   LOADCHOICES("Sound (off, beep, x, y, z)");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 4;
   uvalues[k].uval.ch.llen = 5;
   uvalues[k].uval.ch.list = soundmodes;
   uvalues[k].uval.ch.val = soundflag&7;

   LOADCHOICES("Use PC internal speaker?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = (soundflag & 8)?1:0;

   LOADCHOICES("Use soundcard output?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = (soundflag & 16)?1:0;
/*
   LOADCHOICES("Midi...not implemented yet");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = (soundflag & 32)?1:0;
*/
   LOADCHOICES("Quantize note pitch ?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = (soundflag & 64)?1:0;

   LOADCHOICES("Orbit delay in ms (0 = none)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = orbit_delay;

   LOADCHOICES("Base Hz Value");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = basehertz;

   LOADCHOICES("Show orbits?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = start_showorbit;

   LOADCHOICES("");
   uvalues[k].type = '*';
   LOADCHOICES("Press F6 for FM synth parameters, F7 for scale mappings");
   uvalues[k].type = '*';
   LOADCHOICES("Press F4 to reset to default values");
   uvalues[k].type = '*';

   oldhelpmode = helpmode;
   helpmode = HELPSOUND;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,255,NULL);
   helpmode = oldhelpmode;
   if (i <0) {
      soundflag = old_soundflag;
      orbit_delay = old_orbit_delay;
      start_showorbit = old_start_showorbit;
      return(-1); /*escaped */
   }

   k = -1;

   soundflag = uvalues[++k].uval.ch.val;

   soundflag = soundflag + (uvalues[++k].uval.ch.val * 8);
   soundflag = soundflag + (uvalues[++k].uval.ch.val * 16);
 /*  soundflag = soundflag + (uvalues[++k].uval.ch.val * 32); */
   soundflag = soundflag + (uvalues[++k].uval.ch.val * 64);

   orbit_delay = uvalues[++k].uval.ival;
   basehertz = uvalues[++k].uval.ival;
   start_showorbit = (char)uvalues[++k].uval.ch.val;

   /* now do any intialization needed and check for soundcard */
   if ((soundflag & 16) && !(old_soundflag & 16)) {
     initfm();
   }

   if (i == F6) {
      get_music_parms();/* see below, for controling fmsynth */
      goto get_sound_restart;
   }

   if (i == F7) {
      get_scale_map();/* see below, for setting scale mapping */
      goto get_sound_restart;
   }

   if (i == F4) {
      soundflag = 9; /* reset to default */
      orbit_delay = 0;
      basehertz = 440;
      start_showorbit = 0;
      goto get_sound_restart;
   }

   if (soundflag != old_soundflag && ((soundflag&7) > 1 || (old_soundflag&7) > 1))
      return (1);
   else
      return (0);
}

static int get_scale_map(void)
{
static char o_hdg[] = {"Scale Mapping Screen"};
int oldhelpmode;
char hdg[sizeof(o_hdg)];
char *choices[15];
char *ptr;
struct fullscreenvalues uvalues[15];
int k;
int i, j;

   menu2++;
get_map_restart:
   k = -1;
   strcpy(hdg,o_hdg);
   ptr = (char *)extraseg;

   LOADCHOICES("Scale map C (1)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[0];

   LOADCHOICES("Scale map C#(2)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[1];

   LOADCHOICES("Scale map D (3)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[2];

   LOADCHOICES("Scale map D#(4)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[3];

   LOADCHOICES("Scale map E (5)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[4];

   LOADCHOICES("Scale map F (6)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[5];

   LOADCHOICES("Scale map F#(7)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[6];

   LOADCHOICES("Scale map G (8)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[7];

   LOADCHOICES("Scale map G#(9)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[8];

   LOADCHOICES("Scale map A (10)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[9];

   LOADCHOICES("Scale map A#(11)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[10];

   LOADCHOICES("Scale map B (12)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = scale_map[11];

   LOADCHOICES("");
   uvalues[k].type = '*';
   LOADCHOICES("Press F6 for FM synth parameters");
   uvalues[k].type = '*';
   LOADCHOICES("Press F4 to reset to default values");
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPMUSIC; 
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,255,NULL);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      return(-1);
   }

   k = -1;

   for(j=0;j<=11;j++) {
      scale_map[j] = abs(uvalues[++k].uval.ival);
      if (scale_map[j] > 12)
         scale_map[j] = 12;
   }

   if (i == F6 && menu2 == 1) {
      get_music_parms();/* see below, for controling fmsynth */
      goto get_map_restart;
   } else if (i == F6 && menu2 == 2) {
      menu2--;
   }

   if (i == F4) {
      for(j=0;j<=11;j++) scale_map[j] = j + 1;
      goto get_map_restart;
   }

   return (0);
}


static int get_music_parms(void)
{
static char o_hdg[] = {"FM Synth Card Control Screen"};
char *attenmodes[] = {s_none,s_low,s_mid,s_high};
int oldhelpmode;
char hdg[sizeof(o_hdg)];
char *choices[11];
char *ptr;
struct fullscreenvalues uvalues[11];
int k;
int i;

   menu2++;
get_music_restart:
   k = -1;
   strcpy(hdg,o_hdg);
   ptr = (char *)extraseg;

   LOADCHOICES("Polyphony 1..9");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = polyphony+1;

   LOADCHOICES("Wave type 0..7");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = fm_wavetype;

   LOADCHOICES("Note attack time   0..15");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = fm_attack;

   LOADCHOICES("Note decay time    0..15");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = fm_decay;

   LOADCHOICES("Note sustain level 0..15");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = fm_sustain;

   LOADCHOICES("Note release time  0..15");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = fm_release;

   LOADCHOICES("Soundcard volume?  0..63");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = fm_vol ;

   LOADCHOICES("Hi pitch attenuation");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 4;
   uvalues[k].uval.ch.llen = 4;
   uvalues[k].uval.ch.list = attenmodes;
   uvalues[k].uval.ch.val = hi_atten;

   LOADCHOICES("");
   uvalues[k].type = '*';
   LOADCHOICES("Press F7 for scale mappings");
   uvalues[k].type = '*';
   LOADCHOICES("Press F4 to reset to default values");
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPMUSIC;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,255,NULL);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      return(-1);
   }

   k = -1;
   polyphony = abs(uvalues[++k].uval.ival - 1);
   if (polyphony > 8) polyphony = 8;
   fm_wavetype =  (uvalues[++k].uval.ival)&0x07;
   fm_attack =  (uvalues[++k].uval.ival)&0x0F;
   fm_decay =  (uvalues[++k].uval.ival)&0x0F;
   fm_sustain = (uvalues[++k].uval.ival)&0x0F;
   fm_release = (uvalues[++k].uval.ival)&0x0F;
   fm_vol = (uvalues[++k].uval.ival)&0x3F;
   hi_atten = uvalues[++k].uval.ch.val;
   if (soundflag & 16) {
     initfm();
   }

   if (i == F7 && menu2 == 1) {
      get_scale_map();/* see above, for setting scale mapping */
      goto get_music_restart;
   } else if (i == F7 && menu2 == 2) {
      menu2--;
   }

   if (i == F4) {
      polyphony = 0;
      fm_wavetype = 0;
      fm_attack = 5;
      fm_decay = 10;
      fm_sustain = 13;
      fm_release = 5;
      fm_vol = 63;
      hi_atten = 0;
      if (soundflag & 16) {
        initfm();
      }
      goto get_music_restart;
   }

/*testsound(); */

   return (0);
}

/*testsound(void)
{
int i; 
for (i=100; (i<5000&&!keypressed()); i+=25) 
  { 
   soundon(i);
   sleepms(orbit_delay);
   soundoff();
   sleepms(orbit_delay);
}

}
*/
