#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"
#include "prototyp.h"

#include "Cellular.h"
#include "drivers.h"
#include "fracsubr.h"
#include "miscfrac.h"
#include "realdos.h"
#include "resume.h"
#include "StopMessage.h"

// cellular type
enum
{
	BAD_T			= 1,
	BAD_MEM			= 2,
	STRING1			= 3,
	STRING2			= 4,
	TABLEK			= 5,
	TYPEKR			= 6,
	RULELENGTH		= 7,
	INTERUPT		= 8,
	CELLULAR_DONE	= 10
};

static S16 s_r;
static S16 s_k_1;
static S16 s_rule_digits;
static BYTE *s_cell_array[2] = { 0, 0 };
static int s_last_screen_flag;

static void set_cellular_palette();

// standalone engine for "cellular"
// Originally coded by Ken Shirriff.
//	Modified beyond recognition by Jonathan Osuch.
//	Original or'd the neighborhood, changed to sum the neighborhood
//	Changed prompts and error messages
//	Added CA types
//	Set the palette to some standard? CA g_colors
//	Changed *s_cell_array to near and used dstack so put_line and get_line
//		could be used all the time
//	Made space bar generate next screen
//	Increased string/rule size to 16 digits and added CA types 9/20/92
//
static void abort_cellular(int err, int t)
{
	int i;
	switch (err)
	{
	case BAD_T:
		{
			stop_message(STOPMSG_NORMAL, str(boost::format("Bad t=%d, aborting\n") % t));
		}
		break;
	case BAD_MEM:
		stop_message(STOPMSG_NORMAL, "Insufficient free memory for calculation");
		break;
	case STRING1:
		stop_message(STOPMSG_NORMAL, "String can be a maximum of 16 digits");
		break;
	case STRING2:
		{
			static char msg[] = {"Make string of 0's through  's" };
			msg[27] = (char)(s_k_1 + '0'); // turn into a character value
			stop_message(STOPMSG_NORMAL, msg);
		}
		break;
	case TABLEK:
		{
			static char msg[] = {"Make Rule with 0's through  's" };
			msg[27] = (char)(s_k_1 + '0'); // turn into a character value
			stop_message(STOPMSG_NORMAL, msg);
		}
		break;
	case TYPEKR:
		stop_message(STOPMSG_NORMAL, "Type must be 21, 31, 41, 51, 61, 22, 32, 42, 23, 33, 24, 25, 26, 27");
		break;
	case RULELENGTH:
		{
			static char msg[] = {"Rule must be    digits long" };
			i = s_rule_digits/10;
			if (i == 0)
			{
				msg[14] = (char)(s_rule_digits + '0');
			}
			else
			{
				msg[13] = (char) (i + '0');
				msg[14] = (char) ((s_rule_digits % 10) + '0');
			}
			stop_message(STOPMSG_NORMAL, msg);
		}
		break;
	case INTERUPT:
		stop_message(STOPMSG_NORMAL, "Interrupted, can't resume");
		break;
	case CELLULAR_DONE:
		break;
	}
	delete[] s_cell_array[0];
	delete[] s_cell_array[1];
}

int cellular()
{
	S16 start_row;
	S16 filled, notfilled;
	U16 cell_table[32];
	U16 init_string[16];
	U16 kr, k;
	U32 lnnmbr;
	U16 i, twor;
	S16 t, t2;
	S32 randparam;
	double n;
	set_cellular_palette();

	randparam = (S32)g_parameters[0];
	lnnmbr = (U32)g_parameters[3];
	kr = U16(g_parameters[2]);
	switch (kr)
	{
	case 21:
	case 31:
	case 41:
	case 51:
	case 61:
	case 22:
	case 32:
	case 42:
	case 23:
	case 33:
	case 24:
	case 25:
	case 26:
	case 27:
		break;
	default:
		abort_cellular(TYPEKR, 0);
		return -1;
		// break;
	}

	s_r = (S16)(kr % 10); // Number of nearest neighbors to sum
	k = U16(kr/10); // Number of different states, k = 3 has states 0, 1, 2
	s_k_1 = (S16)(k - 1); // Highest state value, k = 3 has highest state value of 2
	s_rule_digits = (S16)((s_r*2 + 1)*s_k_1 + 1); // Number of digits in the rule

	if (!g_use_fixed_random_seed && (randparam == -1))
	{
		--g_random_seed;
	}
	if (randparam != 0 && randparam != -1)
	{
		n = g_parameters[0];
		std::string buf = str(boost::format("%.16g") % n); // # of digits in initial string
		t = S16(buf.length());
		if (t > 16 || t <= 0)
		{
			abort_cellular(STRING1, 0);
			return -1;
		}
		for (i = 0; i < 16; i++)
		{
			init_string[i] = 0; // zero the array
		}
		t2 = (S16) ((16 - t)/2);
		for (i = 0; i < U16(t); i++)  // center initial string in array
		{
			init_string[i + t2] = U16(buf[i] - '0'); // change character to number
			if (init_string[i + t2] > U16(s_k_1))
			{
				abort_cellular(STRING2, 0);
				return -1;
			}
		}
	}

	srand(g_random_seed);
	if (!g_use_fixed_random_seed)
	{
		++g_random_seed;
	}

// generate rule table from parameter 1
	n = g_parameters[1];
	if (n == 0)  // calculate a random rule
	{
		n = rand() % int(k);
		for (i = 1; i < U16(s_rule_digits); i++)
		{
			n *= 10;
			n += rand() % int(k);
		}
		g_parameters[1] = n;
	}
	std::string buf = str(boost::format(precision_format("g", s_rule_digits)) % n);
	t = S16(buf.length());
	if (s_rule_digits < t || t < 0)  // leading 0s could make t smaller
	{
		abort_cellular(RULELENGTH, 0);
		return -1;
	}
	for (i = 0; i < U16(s_rule_digits); i++) // zero the table
	{
		cell_table[i] = 0;
	}
	for (i = 0; i < U16(t); i++)  // reverse order
	{
		cell_table[i] = U16(buf[t-i-1] - '0'); // change character to number
		if (cell_table[i] > U16(s_k_1))
		{
			abort_cellular(TABLEK, 0);
			return -1;
		}
	}

	start_row = 0;
	s_cell_array[0] = new BYTE[g_x_stop + 1];
	s_cell_array[1] = new BYTE[g_x_stop + 1];
	if (s_cell_array[0] == 0 || s_cell_array[1] == 0)
	{
		abort_cellular(BAD_MEM, 0);
		return -1;
	}

	// g_next_screen_flag toggled by space bar in fractint.cpp, 1 for continuous
	// 0 to stop on next screen
	filled = 0;
	notfilled = (S16)(1-filled);
	if (g_resuming && !g_next_screen_flag && !s_last_screen_flag)
	{
		start_resume();
		get_resume(sizeof(start_row), &start_row);
		end_resume();
		get_line(start_row, 0, g_x_stop, s_cell_array[filled]);
	}
	else if (g_next_screen_flag && !s_last_screen_flag)
	{
		start_resume();
		end_resume();
		get_line(g_y_stop, 0, g_x_stop, s_cell_array[filled]);
		g_parameters[3] += g_y_stop + 1;
		start_row = -1; // after 1st iteration its = 0
	}
	else
	{
		if (g_use_fixed_random_seed || randparam == 0 || randparam == -1)
		{
			for (g_col = 0; g_col <= g_x_stop; g_col++)
			{
				s_cell_array[filled][g_col] = BYTE(rand() % int(k));
			}
		}
		else
		{
			for (g_col = 0; g_col <= g_x_stop; g_col++)  // Clear from end to end
			{
				s_cell_array[filled][g_col] = 0;
			}
			i = 0;
			for (g_col = (g_x_stop-16)/2; g_col < (g_x_stop + 16)/2; g_col++)  // insert initial
			{
				s_cell_array[filled][g_col] = (BYTE)init_string[i++];    // string
			}
		} // end of if not random
		s_last_screen_flag = (lnnmbr != 0) ? 1 : 0;
		put_line(start_row, 0, g_x_stop, s_cell_array[filled]);
	}
	start_row++;

	// This section calculates the starting line when it is not zero
	// This section can't be resumed since no screen output is generated
	// calculates the (lnnmbr - 1) generation
	if (s_last_screen_flag)  // line number != 0 & not resuming & not continuing
	{
		U32 big_row;
		for (big_row = (U32)start_row; big_row < lnnmbr; big_row++)
		{
			thinking(1, "Cellular thinking (higher start row takes longer)");
			if (g_use_fixed_random_seed || randparam == 0 || randparam == -1)
			{
				// Use a random border
				for (i = 0; i <= U16(s_r); i++)
				{
						s_cell_array[notfilled][i] = BYTE(rand() % int(k));
						s_cell_array[notfilled][g_x_stop-i] = BYTE(rand() % int(k));
				}
			}
			else
			{
				// Use a zero border
				for (i = 0; i <= U16(s_r); i++)
				{
					s_cell_array[notfilled][i] = 0;
					s_cell_array[notfilled][g_x_stop-i] = 0;
				}
			}

			t = 0; // do first cell
			for (twor = U16(s_r + s_r), i = 0; i <= twor; i++)
			{
				t = (S16)(t + (S16)s_cell_array[filled][i]);
			}
			if (t > s_rule_digits || t < 0)
			{
				thinking(0, 0);
				abort_cellular(BAD_T, t);
				return -1;
			}
			s_cell_array[notfilled][s_r] = (BYTE)cell_table[t];

			// use a rolling sum in t
			for (g_col = s_r + 1; g_col < g_x_stop - s_r; g_col++)  // now do the rest
			{
				t = (S16)(t + s_cell_array[filled][g_col + s_r] - s_cell_array[filled][g_col - s_r - 1]);
				if (t > s_rule_digits || t < 0)
				{
					thinking(0, 0);
					abort_cellular(BAD_T, t);
					return -1;
				}
				s_cell_array[notfilled][g_col] = (BYTE) cell_table[t];
			}

			filled = notfilled;
			notfilled = (S16)(1 - filled);
			if (driver_key_pressed())
			{
				thinking(0, 0);
				abort_cellular(INTERUPT, 0);
				return -1;
			}
		}
		start_row = 0;
		thinking(0, 0);
		s_last_screen_flag = 0;
	}

contloop:
	// This section does all the work
	for (g_row = start_row; g_row <= g_y_stop; g_row++)
	{
		if (g_use_fixed_random_seed || randparam == 0 || randparam == -1)
		{
			// Use a random border
			for (i = 0; i <= U16(s_r); i++)
			{
				s_cell_array[notfilled][i] = BYTE(rand() % int(k));
				s_cell_array[notfilled][g_x_stop-i] = BYTE(rand() % int(k));
			}
		}
		else
		{
			// Use a zero border
			for (i = 0; i <= U16(s_r); i++)
			{
				s_cell_array[notfilled][i] = 0;
				s_cell_array[notfilled][g_x_stop-i] = 0;
			}
		}

		t = 0; // do first cell
		for (twor = U16(s_r + s_r), i = 0; i <= twor; i++)
		{
			t = (S16)(t + (S16)s_cell_array[filled][i]);
		}
		if (t > s_rule_digits || t < 0)
		{
			thinking(0, 0);
			abort_cellular(BAD_T, t);
			return -1;
		}
		s_cell_array[notfilled][s_r] = (BYTE)cell_table[t];

		// use a rolling sum in t
		for (g_col = s_r + 1; g_col < g_x_stop - s_r; g_col++)  // now do the rest
		{
			t = (S16)(t + s_cell_array[filled][g_col + s_r] - s_cell_array[filled][g_col - s_r - 1]);
			if (t > s_rule_digits || t < 0)
			{
				thinking(0, 0);
				abort_cellular(BAD_T, t);
				return -1;
			}
			s_cell_array[notfilled][g_col] = (BYTE)cell_table[t];
		}

		filled = notfilled;
		notfilled = (S16)(1-filled);
		put_line(g_row, 0, g_x_stop, s_cell_array[filled]);
		if (driver_key_pressed())
		{
			abort_cellular(CELLULAR_DONE, 0);
			alloc_resume(10, 1);
			put_resume(sizeof(g_row), &g_row);
			return -1;
		}
	}
	if (g_next_screen_flag)
	{
		g_parameters[3] += g_y_stop + 1;
		start_row = 0;
		goto contloop;
	}
	abort_cellular(CELLULAR_DONE, 0);
	return 1;
}

bool cellular_setup()
{
	if (!g_resuming)
	{
		g_next_screen_flag = false;
	}
	timer_engine(g_current_fractal_specific->calculate_type);
	return false;
}

static void set_cellular_palette()
{
	// map= not specified
	if (!g_.MapDAC() || g_.ColorState() == COLORSTATE_DEFAULT)
	{
		// TODO: revisit magic numbers for COLOR_CHANNEL_MAX
		BYTE red[3]		= { 42, 0, 0 };
		BYTE green[3]	= { 10, 35, 10 };
		BYTE blue[3]	= { 13, 12, 29 };
		BYTE yellow[3]	= { 60, 58, 18 };
		BYTE brown[3]	= { 42, 21, 0 };
		BYTE black[3]	= { 0, 0, 0 };

		g_.DAC().Set(0, black[0], black[1], black[2]);
		g_.DAC().Set(1, red[0], red[1], red[2]);
		g_.DAC().Set(2, green[0], green[1], green[2]);
		g_.DAC().Set(3, blue[0], blue[1], blue[2]);
		g_.DAC().Set(4, yellow[0], yellow[1], yellow[2]);
		g_.DAC().Set(5, brown[0], brown[1], brown[2]);

		load_dac();
	}
}
