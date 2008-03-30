// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include <algorithm>
#include <sstream>
#include <string>

#include <string.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "strcpy.h"

#include "AbstractInput.h"
#include "ant.h"
#include "drivers.h"
#include "fihelp.h"
#include "fracsubr.h"
#include "realdos.h"

class Ant : public AbstractInputContext
{
public:
	Ant(AbstractDriver *driver) : AbstractInputContext(),
		m_last_xdots(0),
		m_last_ydots(0),
		m_max_ants(0),
		m_wrap(false),
		m_wait(0),
		m_rule_len(0),
		_type(ANTTYPE_MOVE_COLOR),
		_driver(driver)
	{
		std::fill(&m_incx[0], &m_incx[DIRS], static_cast<int *>(0));
		std::fill(&m_incy[0], &m_incy[DIRS], static_cast<int *>(0));
		std::fill(&m_rule[0], &m_rule[MAX_ANTS], 0);
	}
	~Ant()
	{
		free_storage();
	}

	int Compute();
	virtual bool ProcessWaitingKey(int key);
	virtual bool ProcessIdle();

private:
	// ant types
	enum AntType
	{
		ANTTYPE_MOVE_COLOR	= 0,
		ANTTYPE_MOVE_RULE	= 1
	};

	enum
	{
		DIRS = 4,
		MAX_ANTS = 256,
		INNER_LOOP = 100
	};
	// possible value of idir e relative movement in the 4 directions
	// for x 0, 1, 0, -1
	// for y 1, 0, -1, 0
	//
	int *m_incx[DIRS];         // tab for 4 directions
	int *m_incy[DIRS];
	int m_last_xdots;
	int m_last_ydots;
	int m_max_ants;
	char m_rule[MAX_ANTS];
	bool m_wrap;
	long m_wait;
	int m_rule_len;
	AntType _type;
	AbstractDriver *_driver;

	void initialize_increments();
	void free_storage();
	void set_wait();
	void turk_mite1(long maxpts);
	void turk_mite2(long maxptr);

	void SetAntType();
	void SetMaxAnts();
	void SetRandomSeedForReproducibility();

	// Generate Random Number 0 <= r < n
	static int random_number(long n)
	{
		return int((long(rand())*n) >> 15);
	}
};

bool Ant::ProcessIdle()
{
	return true;
}

bool Ant::ProcessWaitingKey(int key)
{
	return true;
}

void Ant::set_wait()
{
	while (true)
	{
		show_temp_message(str(boost::format("Delay %4ld ") % m_wait));

		int kbdchar = _driver->get_key();
		switch (kbdchar)
		{
		case IDK_CTL_RIGHT_ARROW:
		case IDK_CTL_UP_ARROW:
			m_wait += 100;
			break;
		case IDK_RIGHT_ARROW:
		case IDK_UP_ARROW:
			m_wait += 10;
			break;
		case IDK_CTL_DOWN_ARROW:
		case IDK_CTL_LEFT_ARROW:
			m_wait -= 100;
			break;
		case IDK_LEFT_ARROW:
		case IDK_DOWN_ARROW:
			m_wait -= 10;
			break;
		default:
			clear_temp_message();
			return;
		}
		if (m_wait < 0)
		{
			m_wait = 0;
		}
	}
}

// turkmite from scientific american july 1994 pag 91
// Tweaked by Luciano Genero & Fulvio Cappelli
//
void Ant::turk_mite1(long maxpts)
{
	int step = int(m_wait);
	if (step == 1)
	{
		m_wait = 0;
	}
	else
	{
		step = 0;
	}
	int color;
	int next_col[MAX_ANTS + 1];
	int rule[MAX_ANTS + 1];
	if (m_rule_len == 0)
	{
		// random rule
		for (color = 0; color < MAX_ANTS; color++)
		{
			// init the rules and colors for the
			// turkmites: 1 turn left, -1 turn right
			rule[color] = 1 - (random_number(2)*2);
			next_col[color] = color + 1;
		}
		// close the cycle
		next_col[color] = 0;
	}
	else
	{
		// user defined rule
		for (color = 0; color < m_rule_len; color++)
		{
			// init the rules and colors for the
			// turkmites: 1 turn left, -1 turn right
			rule[color] = (m_rule[color]*2) - 1;
			next_col[color] = color + 1;
		}
		// repeats to last color
		for (color = m_rule_len; color < MAX_ANTS; color++)
		{
			// init the rules and colors for the
			// turkmites: 1 turn left, -1 turn right
			rule[color] = rule[color % m_rule_len];
			next_col[color] = color + 1;
		}
		// close the cycle
		next_col[color] = 0;
	}
	int x[MAX_ANTS + 1];
	int y[MAX_ANTS + 1];
	int dir[MAX_ANTS + 1];
	for (color = m_max_ants; color; color--)
	{
		// init the various turmites N.B. non usa
		// x[0], y[0], dir[0]
		if (m_rule_len)
		{
			dir[color] = 1;
			x[color] = g_x_dots/2;
			y[color] = g_y_dots/2;
		}
		else
		{
			dir[color] = random_number(DIRS);
			x[color] = random_number(g_x_dots);
			y[color] = random_number(g_y_dots);
		}
	}
	maxpts /= long(INNER_LOOP);
	long count;
	for (count = 0; count < maxpts; count++)
	{
		// TODO: refactor to IInputContext
		// check for a key only every inner_loop times
		int kbdchar = _driver->key_pressed();
		if (kbdchar || step)
		{
			bool done = false;
			if (kbdchar == 0)
			{
				kbdchar = _driver->get_key();
			}
			switch (kbdchar)
			{
			case IDK_SPACE:
				step = 1 - step;
				break;
			case IDK_ESC:
				done = true;
				break;
			case IDK_RIGHT_ARROW:
			case IDK_UP_ARROW:
			case IDK_DOWN_ARROW:
			case IDK_LEFT_ARROW:
			case IDK_CTL_RIGHT_ARROW:
			case IDK_CTL_UP_ARROW:
			case IDK_CTL_DOWN_ARROW:
			case IDK_CTL_LEFT_ARROW:
				set_wait();
				break;
			default:
				done = true;
				break;
			}
			if (done)
			{
				return;
			}
			if (_driver->key_pressed())
			{
				_driver->get_key();
			}
		}
		for (int i = INNER_LOOP; i; i--)
		{
			int ix;
			int iy;
			int idir;
			int pixel;
			if (m_wait > 0 && step == 0)
			{
				for (color = m_max_ants; color; color--)
				{                   // move the various turmites
					ix = x[color];   // temp vars
					iy = y[color];
					idir = dir[color];

					pixel = get_color(ix, iy);
					g_plot_color_put_color(ix, iy, 15);
					sleep_ms(m_wait);
					g_plot_color_put_color(ix, iy, next_col[pixel]);
					idir += rule[pixel];
					idir &= 3;
					if (!m_wrap)
					{
						if ((idir == 0 && iy == g_y_dots - 1) ||
							(idir == 1 && ix == g_x_dots - 1) ||
							(idir == 2 && iy == 0) ||
							(idir == 3 && ix == 0))
						{
							return;
						}
					}
					x[color] = m_incx[idir][ix];
					y[color] = m_incy[idir][iy];
					dir[color] = idir;
				}
			}
			else
			{
				for (color = m_max_ants; color; color--)
				{                   // move the various turmites without delay
					ix = x[color];   // temp vars
					iy = y[color];
					idir = dir[color];
					pixel = get_color(ix, iy);
					g_plot_color_put_color(ix, iy, next_col[pixel]);
					idir += rule[pixel];
					idir &= 3;
					if (!m_wrap)
					{
						if ((idir == 0 && iy == g_y_dots - 1) ||
							(idir == 1 && ix == g_x_dots - 1) ||
							(idir == 2 && iy == 0) ||
							(idir == 3 && ix == 0))
						{
							return;
						}
					}
					x[color] = m_incx[idir][ix];
					y[color] = m_incy[idir][iy];
					dir[color] = idir;
				}
			}
		}
	}
}

// this one ignore the color of the current cell is more like a white ant
void Ant::turk_mite2(long maxpts)
{
	int step = int(m_wait);
	if (step == 1)
	{
		m_wait = 0;
	}
	else
	{
		step = 0;
	}
	int dir[MAX_ANTS + 1];
	int x[MAX_ANTS + 1];
	int y[MAX_ANTS + 1];
	int rule[MAX_ANTS + 1];
	if (m_rule_len == 0)
	{
		// random rule
		for (int color = MAX_ANTS - 1; color; color--)
		{
			// init the various turmites N.B. don't use
			// x[0], y[0], dir[0]
			dir[color] = random_number(DIRS);
			rule[color] = (rand() << random_number(2)) | random_number(2);
			x[color] = random_number(g_x_dots);
			y[color] = random_number(g_y_dots);
		}
	}
	else
	{
		// the same rule the user wants for every
		// turkmite (max m_rule_len = 16 bit)
		m_rule_len = std::min(m_rule_len, 8*int(sizeof(int)));
		rule[0] = 0;
		for (int i = 0; i < m_rule_len; i++)
		{
			rule[0] = (rule[0] << 1) | m_rule[i];
		}
		for (int color = MAX_ANTS - 1; color; color--)
		{
			// init the various turmites N.B. non usa
			// x[0], y[0], dir[0]
			dir[color] = 0;
			rule[color] = rule[0];
			x[color] = g_x_dots/2;
			y[color] = g_y_dots/2;
		}
	}
	// use this rule when a black pixel is found
	rule[0] = 0;
	int rule_mask = 1;
	maxpts /= long(INNER_LOOP);
	for (int count = 0; count < maxpts; count++)
	{
		// check for a key only every inner_loop times
		int kbdchar = _driver->key_pressed();
		if (kbdchar || step)
		{
			bool done = false;
			if (kbdchar == 0)
			{
				kbdchar = _driver->get_key();
			}
			switch (kbdchar)
			{
			case IDK_SPACE:
				step = 1 - step;
				break;
			case IDK_ESC:
				done = true;
				break;
			case IDK_RIGHT_ARROW:
			case IDK_UP_ARROW:
			case IDK_DOWN_ARROW:
			case IDK_LEFT_ARROW:
			case IDK_CTL_RIGHT_ARROW:
			case IDK_CTL_UP_ARROW:
			case IDK_CTL_DOWN_ARROW:
			case IDK_CTL_LEFT_ARROW:
				set_wait();
				break;
			default:
				done = true;
				break;
			}
			if (done)
			{
				return;
			}
			if (_driver->key_pressed())
			{
				_driver->get_key();
			}
		}
		for (int i = INNER_LOOP; i; i--)
		{
			for (int color = m_max_ants; color; color--)
			{                      // move the various turmites
				int ix = x[color];      // temp vars
				int iy = y[color];
				int idir = dir[color];
				int pixel = get_color(ix, iy);
				g_plot_color_put_color(ix, iy, 15);

				if (m_wait > 0 && step == 0)
				{
					sleep_ms(m_wait);
				}

				if (rule[pixel] & rule_mask)
				{
					// turn right
					idir--;
					g_plot_color_put_color(ix, iy, 0);
				}
				else
				{
					// turn left
					idir++;
					g_plot_color_put_color(ix, iy, color);
				}
				idir &= 3;
				if (!m_wrap)
				{
					if ((idir == 0 && iy == g_y_dots - 1) ||
						(idir == 1 && ix == g_x_dots - 1) ||
						(idir == 2 && iy == 0) ||
						(idir == 3 && ix == 0))
					{
						return;
					}
				}
				x[color] = m_incx[idir][ix];
				y[color] = m_incy[idir][iy];
				dir[color] = idir;
			}
			rule_mask = _rotl(rule_mask, 1);
		}
	}
}

void Ant::free_storage()
{
	for (int i = 0; i < DIRS; i++)
	{
		delete[] m_incx[i];
		delete[] m_incy[i];
		m_incx[i] = 0;
		m_incy[i] = 0;
	}
}

void Ant::initialize_increments()
{
	// In this vectors put all the possible point that the ants can visit.
	// Wrap them from a side to the other insted of simply end calculation
	//
	for (int i = 0; i < g_x_dots; i++)
	{
		m_incx[0][i] = i;
		m_incx[2][i] = i;
	}
	for (int i = 0; i < g_x_dots; i++)
	{
		m_incx[3][i] = i + 1;
	}
	m_incx[3][g_x_dots-1] = 0; // wrap from right of the screen to left
	for (int i = 1; i < g_x_dots; i++)
	{
		m_incx[1][i] = i - 1;
	}
	m_incx[1][0] = g_x_dots-1; // wrap from left of the screen to right

	for (int i = 0; i < g_y_dots; i++)
	{
		m_incy[1][i] = i;
		m_incy[3][i] = i;
	}
	for (int i = 0; i < g_y_dots; i++)
	{
		m_incy[0][i] = i + 1;
	}
	m_incy[0][g_y_dots - 1] = 0;      // wrap from the top of the screen to the bottom
	for (int i = 1; i < g_y_dots; i++)
	{
		m_incy[2][i] = i - 1;
	}
	m_incy[2][0] = g_y_dots - 1;      // wrap from the bottom of the screen to the top
}

int Ant::Compute()
{
	if (g_x_dots != m_last_xdots || g_y_dots != m_last_ydots)
	{
		m_last_xdots = g_x_dots;
		m_last_ydots = g_y_dots;

		free_storage();	// free old memory
		for (int i = 0; i < DIRS; i++)
		{
			m_incx[i] = new int[g_x_dots];
			m_incy[i] = new int[g_y_dots];
		}
	}

	initialize_increments();

	HelpModeSaver saved_help(FIHELP_ANT_COMMANDS);
	long maxpts = std::abs(long(g_parameters[1]));
	m_wait = std::abs(g_orbit_delay);
	strcpy(m_rule, boost::format("%.17g") % g_parameters[0]);
	m_rule_len = int(strlen(m_rule));
	if (m_rule_len > 1)
	{                            // if m_rule_len == 0 random rule
		for (int i = 0; i < m_rule_len; i++)
		{
			m_rule[i] = (m_rule[i] != '1') ? 0 : 1;
		}
	}
	else
	{
		m_rule_len = 0;
	}

	SetRandomSeedForReproducibility();

	SetMaxAnts();
	SetAntType();
	m_wrap = (g_parameters[4] != 0);

	switch (_type)
	{
	case ANTTYPE_MOVE_COLOR:
		turk_mite1(maxpts);
		break;
	case ANTTYPE_MOVE_RULE:
		turk_mite2(maxpts);
		break;
	}
	return 0;
}

void Ant::SetRandomSeedForReproducibility()
{
	if (!g_use_fixed_random_seed && (g_parameters[5] == 1))
	{
		--g_random_seed;
	}
	if (g_parameters[5] != 0 && g_parameters[5] != 1)
	{
		g_random_seed = int(g_parameters[5]);
	}

	srand(g_random_seed);
	if (!g_use_fixed_random_seed)
	{
		++g_random_seed;
	}
}

void Ant::SetMaxAnts()
{
	m_max_ants = int(g_parameters[2]);
	if (m_max_ants < 1)             // if m_max_ants == 0 maxants random
	{
		m_max_ants = 2 + random_number(MAX_ANTS - 2);
	}
	else if (m_max_ants > MAX_ANTS)
	{
		m_max_ants = MAX_ANTS;
		g_parameters[2] = m_max_ants;

	}
}

void Ant::SetAntType()
{
	_type = AntType(int(g_parameters[3]) - 1);
	if (_type < ANTTYPE_MOVE_COLOR || _type > ANTTYPE_MOVE_RULE)
	{
		// if param[3] == 0 choose a random type
		_type = AntType(random_number(2));
	}
}

int ant()
{
	return Ant(DriverManager::current()).Compute();
}
