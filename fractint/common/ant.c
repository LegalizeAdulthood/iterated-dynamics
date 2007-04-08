/* The Ant Automaton is based on an article in Scientific American, July 1994.
 * The original Fractint implementation was by Tim Wegner in Fractint 19.0.
 * This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
 * tables for speed, and adds a second ant type, multiple ants, and random
 * rules.
 *
 * Revision history:
 * 20 Mar 95 LG/FC First release of table driven version
 * 31 Mar 95 LG/FC Fixed a bug that writes one pixel off the screen
 * 31 Mar 95 LG/FC Changed ant type 1 to produce the same pattern as the
 *                   original implementation (they were mirrored on the
 *                   x axis)
 * 04 Apr 95 TW    Added wrap option and further modified the code to match
 *                   the original algorithm. It now matches exactly.
 * 10 Apr 95 TW    Suffix array does not contain enough memory. Crashes at
 *                   over 1024x768. Changed to extraseg.
 * 12 Apr 95 TW    Added maxants range check.
 */
#include <string.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "drivers.h"

#define RANDOM(n)       ((int)((long)((long)rand()*(long)(n)) >> 15)) /* Generate Random
                                                                         * Number 0 <= r < n */
#define MAX_ANTS        256
#define XO              (xdots/2)
#define YO              (ydots/2)
#define DIRS            4
#define INNER_LOOP      100

/* possible value of idir e relative movement in the 4 directions
 * for x 0, 1, 0, -1
 * for y 1, 0, -1, 0
 */
static int *s_incx[DIRS];         /* tab for 4 directions */
static int *s_incy[DIRS];
static int s_last_xdots = 0;
static int s_last_ydots = 0;

static void set_wait(long *wait)
{
	char msg[30];
	int kbdchar;

	while (1)
	{
		sprintf(msg, "Delay %4ld", *wait);
		while ((int) strlen(msg) < 15)
		{
			strcat(msg, " ");
		}
		msg[15] = '\0';
		showtempmsg(msg);
		kbdchar = driver_get_key();
		switch (kbdchar)
		{
		case FIK_CTL_RIGHT_ARROW:
		case FIK_CTL_UP_ARROW:
			*wait += 100;
			break;
		case FIK_RIGHT_ARROW:
		case FIK_UP_ARROW:
			*wait += 10;
			break;
		case FIK_CTL_DOWN_ARROW:
		case FIK_CTL_LEFT_ARROW:
			*wait -= 100;
			break;
		case FIK_LEFT_ARROW:
		case FIK_DOWN_ARROW:
			*wait -= 10;
			break;
		default:
			cleartempmsg();
			return;
		}
		if (*wait < 0)
		{
			*wait = 0;
		}
	}
}

/* turkmite from scientific american july 1994 pag 91
 * Tweaked by Luciano Genero & Fulvio Cappelli
 */
static void
turk_mite1(int maxtur, int rule_len, char *ru, long maxpts, long wait)
{
	int color, ix, iy, idir, pixel, i;
	int kbdchar, step, antwrap;
	int x[MAX_ANTS + 1], y[MAX_ANTS + 1];
	int next_col[MAX_ANTS + 1], rule[MAX_ANTS + 1], dir[MAX_ANTS + 1];
	long count;
	antwrap = ((param[4] == 0) ? 0 : 1);
	step = (int) wait;
	if (step == 1)
	{
		wait = 0;
	}
	else
	{
		step = 0;
	}
	if (rule_len == 0)
	{
		/* random rule */
		for (color = 0; color < MAX_ANTS; color++)
		{
			/* init the rules and colors for the
			* turkmites: 1 turn left, -1 turn right */
			rule[color] = 1 - (RANDOM(2)*2);
			next_col[color] = color + 1;
		}
		/* close the cycle */
		next_col[color] = 0;
	}
	else
	{
		/* user defined rule */
		for (color = 0; color < rule_len; color++)
		{
			/* init the rules and colors for the
			* turkmites: 1 turn left, -1 turn right */
			rule[color] = (ru[color]*2) - 1;
			next_col[color] = color + 1;
		}
		/* repeats to last color */
		for (color = rule_len; color < MAX_ANTS; color++)
		{
			/* init the rules and colors for the
			* turkmites: 1 turn left, -1 turn right */
			rule[color] = rule[color % rule_len];
			next_col[color] = color + 1;
		}
		/* close the cycle */
		next_col[color] = 0;
	}
	for (color = maxtur; color; color--)
	{
		/* init the various turmites N.B. non usa
		* x[0], y[0], dir[0] */
		if (rule_len)
		{
			dir[color] = 1;
			x[color] = XO;
			y[color] = YO;
		}
		else
		{
			dir[color] = RANDOM(DIRS);
			x[color] = RANDOM(xdots);
			y[color] = RANDOM(ydots);
		}
	}
	maxpts = maxpts / (long) INNER_LOOP;
	for (count = 0; count < maxpts; count++)
	{
		/* check for a key only every inner_loop times */
		kbdchar = driver_key_pressed();
		if (kbdchar || step)
		{
			int done = 0;
			if (kbdchar == 0)
			{
				kbdchar = driver_get_key();
			}
			switch (kbdchar)
			{
			case FIK_SPACE:
				step = 1 - step;
				break;
			case FIK_ESC:
				done = 1;
				break;
			case FIK_RIGHT_ARROW:
			case FIK_UP_ARROW:
			case FIK_DOWN_ARROW:
			case FIK_LEFT_ARROW:
			case FIK_CTL_RIGHT_ARROW:
			case FIK_CTL_UP_ARROW:
			case FIK_CTL_DOWN_ARROW:
			case FIK_CTL_LEFT_ARROW:
				set_wait(&wait);
				break;
			default:
				done = 1;
				break;
			}
			if (done)
			{
				goto exit_ant;
			}
			if (driver_key_pressed())
			{
				driver_get_key();
			}
		}
		for (i = INNER_LOOP; i; i--)
		{
			if (wait > 0 && step == 0)
			{
				for (color = maxtur; color; color--)
				{                   /* move the various turmites */
					ix = x[color];   /* temp vars */
					iy = y[color];
					idir = dir[color];

					pixel = getcolor(ix, iy);
					g_put_color(ix, iy, 15);
					sleep_ms(wait);
					g_put_color(ix, iy, next_col[pixel]);
					idir += rule[pixel];
					idir &= 3;
					if (antwrap == 0)
					{
						if ((idir == 0 && iy == ydots - 1) ||
							(idir == 1 && ix == xdots - 1) ||
							(idir == 2 && iy == 0) ||
							(idir == 3 && ix == 0))
						{
							goto exit_ant;
						}
					}
					x[color] = s_incx[idir][ix];
					y[color] = s_incy[idir][iy];
					dir[color] = idir;
				}
			}
			else
			{
				for (color = maxtur; color; color--)
				{                   /* move the various turmites without delay */
					ix = x[color];   /* temp vars */
					iy = y[color];
					idir = dir[color];
					pixel = getcolor(ix, iy);
					g_put_color(ix, iy, next_col[pixel]);
					idir += rule[pixel];
					idir &= 3;
					if (antwrap == 0)
					{
						if ((idir == 0 && iy == ydots - 1) ||
							(idir == 1 && ix == xdots - 1) ||
							(idir == 2 && iy == 0) ||
							(idir == 3 && ix == 0))
						{
							goto exit_ant;
						}
					}
					x[color] = s_incx[idir][ix];
					y[color] = s_incy[idir][iy];
					dir[color] = idir;
				}
			}
		}
	}
exit_ant:
	return;
}

/* this one ignore the color of the current cell is more like a white ant */
static void
turk_mite2(int maxtur, int rule_len, char *ru, long maxpts, long wait)
{
	int color, ix, iy, idir, pixel, dir[MAX_ANTS + 1], i;
	int kbdchar, step, antwrap;
	int x[MAX_ANTS + 1], y[MAX_ANTS + 1];
	int rule[MAX_ANTS + 1], rule_mask;
	long count;

	antwrap = ((param[4] == 0) ? 0 : 1);

	step = (int) wait;
	if (step == 1)
	{
		wait = 0;
	}
	else
	{
		step = 0;
	}
	if (rule_len == 0)
	{
		/* random rule */
		for (color = MAX_ANTS - 1; color; color--)
		{
			/* init the various turmites N.B. don't use
			* x[0], y[0], dir[0] */
			dir[color] = RANDOM(DIRS);
			rule[color] = (rand() << RANDOM(2)) | RANDOM(2);
			x[color] = RANDOM(xdots);
			y[color] = RANDOM(ydots);
		}
	}
	else
	{
		/* the same rule the user wants for every
		* turkmite (max rule_len = 16 bit) */
		rule_len = min(rule_len, 8*sizeof(int));
		for (i = 0, rule[0] = 0; i < rule_len; i++)
		{
			rule[0] = (rule[0] << 1) | ru[i];
		}
		for (color = MAX_ANTS - 1; color; color--)
		{
			/* init the various turmites N.B. non usa
			* x[0], y[0], dir[0] */
			dir[color] = 0;
			rule[color] = rule[0];
			x[color] = XO;
			y[color] = YO;
		}
	}
	/* use this rule when a black pixel is found */
	rule[0] = 0;
	rule_mask = 1;
	maxpts = maxpts / (long) INNER_LOOP;
	for (count = 0; count < maxpts; count++)
	{
		/* check for a key only every inner_loop times */
		kbdchar = driver_key_pressed();
		if (kbdchar || step)
		{
			int done = 0;
			if (kbdchar == 0)
			{
				kbdchar = driver_get_key();
			}
			switch (kbdchar)
			{
			case FIK_SPACE:
				step = 1 - step;
				break;
			case FIK_ESC:
				done = 1;
				break;
			case FIK_RIGHT_ARROW:
			case FIK_UP_ARROW:
			case FIK_DOWN_ARROW:
			case FIK_LEFT_ARROW:
			case FIK_CTL_RIGHT_ARROW:
			case FIK_CTL_UP_ARROW:
			case FIK_CTL_DOWN_ARROW:
			case FIK_CTL_LEFT_ARROW:
				set_wait(&wait);
				break;
			default:
				done = 1;
				break;
			}
			if (done)
			{
				goto exit_ant;
			}
			if (driver_key_pressed())
			{
				driver_get_key();
			}
		}
		for (i = INNER_LOOP; i; i--)
		{
			for (color = maxtur; color; color--)
			{                      /* move the various turmites */
				ix = x[color];      /* temp vars */
				iy = y[color];
				idir = dir[color];
				pixel = getcolor(ix, iy);
				g_put_color(ix, iy, 15);

				if (wait > 0 && step == 0)
				{
					sleep_ms(wait);
				}

				if (rule[pixel] & rule_mask)
				{
					/* turn right */
					idir--;
					g_put_color(ix, iy, 0);
				}
				else
				{
					/* turn left */
					idir++;
					g_put_color(ix, iy, color);
				}
				idir &= 3;
				if (antwrap == 0)
				{
					if ((idir == 0 && iy == ydots - 1) ||
						(idir == 1 && ix == xdots - 1) ||
						(idir == 2 && iy == 0) ||
						(idir == 3 && ix == 0))
					{
						goto exit_ant;
					}
				}
				x[color] = s_incx[idir][ix];
				y[color] = s_incy[idir][iy];
				dir[color] = idir;
			}
			rule_mask = _rotl(rule_mask, 1);
		}
	}
exit_ant:
	return;
}

void free_ant_storage(void)
{
	if (s_incx[0])
	{
		free(s_incx[0]);
		s_incx[0] = NULL;
	}
}

int ant(void)
{
	int maxants, type, i;
	int oldhelpmode, rule_len;
	long maxpts, wait;
	char rule[MAX_ANTS];

	if (xdots != s_last_xdots || ydots != s_last_ydots)
	{
		int *storage = (int *) malloc((xdots + 2)*sizeof(int)*DIRS + (ydots + 2)*sizeof(int)*DIRS);
		int *y_storage = storage + (xdots + 2)*DIRS;

		s_last_xdots = xdots;
		s_last_ydots = ydots;

		free_ant_storage();	/* free old memory */
		for (i = 0; i < DIRS; i++)
		{
			s_incx[i] = storage;
			storage += xdots + 2;
			s_incy[i] = y_storage;
			y_storage += ydots + 2;
		}
	}

	/* In this vectors put all the possible point that the ants can visit.
	* Wrap them from a side to the other insted of simply end calculation
	*/
	for (i = 0; i < xdots; i++)
	{
		s_incx[0][i] = i;
		s_incx[2][i] = i;
	}

	for (i = 0; i < xdots; i++)
	{
		s_incx[3][i] = i + 1;
	}
	s_incx[3][xdots-1] = 0; /* wrap from right of the screen to left */

	for (i = 1; i < xdots; i++)
	{
		s_incx[1][i] = i - 1;
	}
	s_incx[1][0] = xdots-1; /* wrap from left of the screen to right */

	for (i = 0; i < ydots; i++)
	{
		s_incy[1][i] = i;
		s_incy[3][i] = i;
	}
	for (i = 0; i < ydots; i++)
	{
		s_incy[0][i] = i + 1;
	}
	s_incy[0][ydots - 1] = 0;      /* wrap from the top of the screen to the
									* bottom */
	for (i = 1; i < ydots; i++)
	{
		s_incy[2][i] = i - 1;
	}
	s_incy[2][0] = ydots - 1;      /* wrap from the bottom of the screen to the
									* top */
	oldhelpmode = helpmode;
	helpmode = ANTCOMMANDS;
	maxpts = (long) param[1];
	maxpts = labs(maxpts);
	wait = abs(g_orbit_delay);
	sprintf(rule, "%.17g", param[0]);
	rule_len = (int) strlen(rule);
	if (rule_len > 1)
	{                            /* if rule_len == 0 random rule */
		for (i = 0; i < rule_len; i++)
		{
			rule[i] = (rule[i] != '1') ? (char) 0 : (char) 1;
		}
	}
	else
	{
		rule_len = 0;
	}

	/* set random seed for reproducibility */
	if ((!g_random_flag) && param[5] == 1)
	{
		--g_random_seed;
	}
	if (param[5] != 0 && param[5] != 1)
	{
		g_random_seed = (int)param[5];
	}

	srand(g_random_seed);
	if (!g_random_flag)
	{
		++g_random_seed;
	}

	maxants = (int) param[2];
	if (maxants < 1)             /* if maxants == 0 maxants random */
	{
		maxants = 2 + RANDOM(MAX_ANTS - 2);
	}
	else if (maxants > MAX_ANTS)
	{
		param[2] = maxants = MAX_ANTS;
	}
	type = (int) param[3] - 1;
	if (type < ANTTYPE_MOVE_COLOR || type > ANTTYPE_MOVE_RULE)
	{
		type = RANDOM(2);         /* if parma[3] == 0 choose a random type */
	}
	switch (type)
	{
	case ANTTYPE_MOVE_COLOR:
		turk_mite1(maxants, rule_len, rule, maxpts, wait);
		break;
	case ANTTYPE_MOVE_RULE:
		turk_mite2(maxants, rule_len, rule, maxpts, wait);
		break;
	}
	helpmode = oldhelpmode;
	return 0;
}
