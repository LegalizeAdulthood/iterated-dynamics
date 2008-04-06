#include <fstream>
#include <string>
#include <vector>

#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fpu.h"
#include "lsys.h"
#include "miscres.h"
#include "realdos.h"
#include "StopMessage.h"

#include "MathUtil.h"

enum
{
	MAXRULES = 27, // this limits rules to 25
	MAX_LSYS_LINE_LEN = 255 // this limits line length to 255
};

// The number by which to multiply sines, cosines and other
// values with magnitudes less than or equal to 1.
// sins and coss are a 3/29 bit fixed-point scheme (so the
// range is +/- 2, with good accuracy.  The range is to
// avoid overflowing when the aspect ratio is taken into
// account.
//
static const double FIXEDLT1 = 536870912.0;

static const double ANGLE2DOUBLE = (2.0*MathUtil::Pi/4294967296.0);

static const long FIXEDMUL = 524288L;

// Take an FP number and turn it into a
// 16/16-bit fixed-point number.
//
inline long FIXEDPT(double x)
{
	return long(FIXEDMUL*x);
}

struct lsys_cmd
{
	void (*function)(lsys_turtle_state_l *);
	long n;
	char ch;
};

static std::vector<long> s_sins;
static std::vector<long> s_coss;

int g_max_angle;

static char *ruleptrs[MAXRULES];
static lsys_cmd *rules2[MAXRULES];
static bool s_loaded = false;

static int read_l_system_file(std::string &item);
static void free_rules_mem();
static int rule_present(char symbol);
static int save_rule(char *, char **);
static int append_rule(char *rule, int index);
static void free_l_cmds();
static lsys_cmd *find_size(lsys_cmd *, lsys_turtle_state_l *, lsys_cmd **, int);
static lsys_cmd *draw_lsysi(lsys_cmd *command, lsys_turtle_state_l *ts, lsys_cmd **rules, int depth);
static int lsysi_find_scale(lsys_cmd *command, lsys_turtle_state_l *ts, lsys_cmd **rules, int depth);
static lsys_cmd *lsysi_size_transform(char *s, lsys_turtle_state_l *ts);
static lsys_cmd *lsysi_draw_transform(char *s, lsys_turtle_state_l *ts);
static void lsysi_sin_cos();
static void lsysi_slash(lsys_turtle_state_l *cmd);
static void lsysi_backslash(lsys_turtle_state_l *cmd);
static void lsysi_at(lsys_turtle_state_l *cmd);
static void lsysi_pipe(lsys_turtle_state_l *cmd);
static void lsysi_size_dm(lsys_turtle_state_l *cmd);
static void lsysi_size_gf(lsys_turtle_state_l *cmd);
static void lsysi_draw_d(lsys_turtle_state_l *cmd);
static void lsysi_draw_m(lsys_turtle_state_l *cmd);
static void lsysi_draw_g(lsys_turtle_state_l *cmd);
static void lsysi_draw_f(lsys_turtle_state_l *cmd);
static void lsysi_draw_c(lsys_turtle_state_l *cmd);
static void lsysi_draw_gt(lsys_turtle_state_l *cmd);
static void lsysi_draw_lt(lsys_turtle_state_l *cmd);

bool is_pow2(int n)
{
	return n == (n & -n);
}

LDBL get_number(char **str)
{
	char numstr[30];
	LDBL ret;
	int i;
	bool root = false;
	bool inverse = false;
	strcpy(numstr, "");
	(*str)++;
	switch (**str)
	{
	case 'q':
		root = true;
		(*str)++;
		break;
	case 'i':
		inverse = true;
		(*str)++;
		break;
	}
	switch (**str)
	{
	case 'q':
		root = true;
		(*str)++;
		break;
	case 'i':
		inverse = true;
		(*str)++;
		break;
	}
	i = 0;
	while ((**str <= '9' && **str >= '0') || **str == '.')
	{
		numstr[i++] = **str;
		(*str)++;
	}
	(*str)--;
	numstr[i] = 0;
	ret = atof(numstr);
	if (ret <= 0.0) // this is a sanity check, JCO 8/31/94
	{
		return 0;
	}
	if (root)
	{
		ret = sqrtl(ret);
	}
	if (inverse)
	{
		ret = 1.0/ret;
	}
	return ret;
}

static std::string l_system_error(const char *message, int line, const char *symbol)
{
	return str(boost::format(std::string("Error:  ") + message + "\n") % line % symbol);
}

static std::string l_system_error(const char *message)
{
	return std::string("Error: ") + message + "\n";
}

static int read_l_system_file(std::string &item)
{
	int c;
	char **rulind;
	int err = 0;
	int linenum;
	bool check = false;
	char inline1[MAX_LSYS_LINE_LEN + 1];
	char fixed[MAX_LSYS_LINE_LEN + 1];
	char *word;
	std::ifstream infile;
	std::string message;

	if (!find_file_item(g_l_system_filename, item, infile, ITEMTYPE_L_SYSTEM))
	{
		return -1;
	}
	while ((c = infile.get()) != '{')
	{
		if (c == EOF)
		{
			return -1;
		}
	}
	g_max_angle = 0;
	for (linenum = 0; linenum < MAXRULES; ++linenum)
	{
		ruleptrs[linenum] = 0;
	}
	rulind = &ruleptrs[1];
	linenum = 0;
	message = "";

	while (file_gets(inline1, MAX_LSYS_LINE_LEN, infile) > -1)  // Max line length chars
	{
		linenum++;
		word = strchr(inline1, ';');
		if (word != 0) // strip comment
		{
			*word = 0;
		}
		strlwr(inline1);

		if (int(strspn(inline1, " \t\n")) < int(strlen(inline1))) // not a blank line
		{
			word = strtok(inline1, " =\t\n");
			if (!strcmp(word, "axiom"))
			{
				if (save_rule(strtok(0, " \t\n"), &ruleptrs[0]))
				{
					message += l_system_error("out of memory");
					++err;
					break;
				}
				check = true;
			}
			else if (!strcmp(word, "angle"))
			{
				g_max_angle = char(atoi(strtok(0, " \t\n")));
				check = true;
			}
			else if (!strcmp(word, "}"))
			{
				break;
			}
			else if (!word[1])
			{
				char *temp;
				int index;
				int memerr = 0;

				if (strchr("+-/\\@|!c<>][", *word))
				{
					message += l_system_error("Syntax error line %d: Redefined reserved symbol %s", linenum, word);
					++err;
					break;
				}
				temp = strtok(0, " =\t\n");
				index = rule_present(*word);

				if (!index)
				{
					strcpy(fixed, word);
					if (temp)
					{
						strcat(fixed, temp);
					}
					memerr = save_rule(fixed, rulind++);
				}
				else if (temp)
				{
					strcpy(fixed, temp);
					memerr = append_rule(fixed, index);
				}
				if (memerr)
				{
					message += l_system_error("out of memory");
					++err;
					break;
				}
				check = true;
			}
			else
			{
				if (err < 6)
				{
					message += l_system_error("Syntax error line %d: %s", linenum, word);
					++err;
				}
			}
			if (check)
			{
				check = false;
				word = strtok(0, " \t\n");
				if (word != 0)
				{
					if (err < 6)
					{
						message += l_system_error("Extra text after command line %d: %s", linenum, word);
						++err;
					}
				}
			}
		}
	}
	infile.close();
	if (!ruleptrs[0] && err < 6)
	{
		message += l_system_error("no axiom");
		++err;
	}
	if ((g_max_angle < 3 || g_max_angle > 50) && err < 6)
	{
		message += l_system_error("illegal or missing angle");
		++err;
	}
	if (err)
	{
		stop_message(STOPMSG_NORMAL, message);
		return -1;
	}
	*rulind = 0;
	return 0;
}

int l_system()
{
	int order;
	char **rulesc;
	lsys_cmd **sc;
	bool stackoflow = false;

	if (!s_loaded && l_load())
	{
		return -1;
	}

	g_overflow = false;                // reset integer math overflow flag

	order = int(g_parameters[P1_REAL]);
	if (order <= 0)
	{
		order = 0;
	}
	if (g_user_float_flag)
	{
		g_overflow = true;
	}
	else
	{
		lsys_turtle_state_l ts;

		ts.stackoflow = false;
		ts.max_angle = g_max_angle;
		ts.dmaxangle = char(g_max_angle - 1);

		sc = rules2;
		for (rulesc = ruleptrs; *rulesc; rulesc++)
		{
			*sc++ = lsysi_size_transform(*rulesc, &ts);
		}
		*sc = 0;

		lsysi_sin_cos();
		if (lsysi_find_scale(rules2[0], &ts, &rules2[1], order))
		{
			ts.realangle = 0;
			ts.angle = 0;
			ts.reverse = 0;

			free_l_cmds();
			sc = rules2;
			for (rulesc = ruleptrs; *rulesc; rulesc++)
			{
				*sc++ = lsysi_draw_transform(*rulesc, &ts);
			}
			*sc = 0;

			// !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
			ts.curcolor = 15;
			if (ts.curcolor > g_colors)
			{
				ts.curcolor = char(g_colors-1);
			}
			draw_lsysi(rules2[0], &ts, &rules2[1], order);
		}
		stackoflow = ts.stackoflow;
	}

	if (stackoflow)
	{
		stop_message(STOPMSG_NORMAL, "insufficient memory, try a lower order");
	}
	else if (g_overflow)
	{
		lsys_turtle_state_fp ts;

		g_overflow = false;

		ts.stackoflow = false;
		ts.max_angle = g_max_angle;
		ts.dmaxangle = char(g_max_angle - 1);

		sc = rules2;
		for (rulesc = ruleptrs; *rulesc; rulesc++)
		{
			*sc++ = lsysf_size_transform(*rulesc, &ts);
		}
		*sc = 0;

		lsysf_sin_cos();
		if (lsysf_find_scale(rules2[0], &ts, &rules2[1], order))
		{
			ts.realangle = 0;
			ts.angle = 0;
			ts.reverse = 0;

			free_l_cmds();
			sc = rules2;
			for (rulesc = ruleptrs; *rulesc; rulesc++)
			{
				*sc++ = lsysf_draw_transform(*rulesc, &ts);
			}
			*sc = 0;

			// !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
			ts.curcolor = 15;
			if (ts.curcolor > g_colors)
			{
				ts.curcolor = char(g_colors-1);
			}
			draw_lsysf(rules2[0], &ts, &rules2[1], order);
		}
		g_overflow = false;
	}
	free_rules_mem();
	free_l_cmds();
	s_loaded = false;
	return 0;
}

int l_load()
{
	if (read_l_system_file(g_l_system_name))  // error occurred
	{
		free_rules_mem();
		s_loaded = false;
		return -1;
	}
	s_loaded = true;
	return 0;
}

static void free_rules_mem()
{
	for (int i = 0; i < MAXRULES; ++i)
	{
		delete[] ruleptrs[i];
	}
}

static int rule_present(char symbol)
{
	int i;
	for (i = 1; i < MAXRULES && ruleptrs[i] && *ruleptrs[i] != symbol; i++)
	{
		;
	}
	return (i < MAXRULES && ruleptrs[i]) ? i : 0;
}

static int save_rule(char *rule, char **saveptr)
{
	int i;
	i = int(strlen(rule)) + 1;
	char *tmpfar = new char[i];
	if (tmpfar == 0)
	{
		return -1;
	}
	*saveptr = tmpfar;
	while (--i >= 0)
	{
		*(tmpfar++) = *(rule++);
	}
	return 0;
}

static int append_rule(char *rule, int index)
{
	char *sav = ruleptrs[index];
	char *old = sav;

	int i;
	for (i = 0; *(old++); i++)
	{
		;
	}
	int j = int(strlen(rule)) + 1;
	char *dst = new char[i + j];
	if (dst == 0)
	{
		return -1;
	}

	old = sav;
	ruleptrs[index] = dst;
	while (i-- > 0)
	{
		*(dst++) = *(old++);
	}
	while (j-- > 0)
	{
		*(dst++) = *(rule++);
	}
	delete[] sav;
	return 0;
}

static void free_l_cmds()
{
	lsys_cmd **sc = rules2;

	while (*sc)
	{
		delete[] (*sc++);
	}
}

// integer specific routines
static void lsysi_plus(lsys_turtle_state_l *cmd)
{
	if (cmd->reverse)
	{
		if (++cmd->angle == cmd->max_angle)
		{
			cmd->angle = 0;
		}
	}
	else
	{
		if (cmd->angle)
		{
			cmd->angle--;
		}
		else
		{
			cmd->angle = cmd->dmaxangle;
		}
	}
}

// This is the same as lsys_doplus, except max_angle is a power of 2.
static void lsysi_plus_pow2(lsys_turtle_state_l *cmd)
{
	if (cmd->reverse)
	{
		cmd->angle++;
		cmd->angle &= cmd->dmaxangle;
	}
	else
	{
		cmd->angle--;
		cmd->angle &= cmd->dmaxangle;
	}
}

static void lsysi_minus(lsys_turtle_state_l *cmd)
{
	if (cmd->reverse)
	{
		if (cmd->angle)
		{
			cmd->angle--;
		}
		else
		{
			cmd->angle = cmd->dmaxangle;
		}
	}
	else
	{
		if (++cmd->angle == cmd->max_angle)
		{
			cmd->angle = 0;
		}
	}
}

static void lsysi_minus_pow2(lsys_turtle_state_l *cmd)
{
	cmd->reverse ? cmd->angle-- : cmd->angle++;
	cmd->angle &= cmd->dmaxangle;
}

static void lsysi_slash(lsys_turtle_state_l *cmd)
{
	if (cmd->reverse)
	{
		cmd->realangle -= cmd->num;
	}
	else
	{
		cmd->realangle += cmd->num;
	}
}

static void lsysi_backslash(lsys_turtle_state_l *cmd)
{
	if (cmd->reverse)
	{
		cmd->realangle += cmd->num;
	}
	else
	{
		cmd->realangle -= cmd->num;
	}
}

static void lsysi_at(lsys_turtle_state_l *cmd)
{
	cmd->size = multiply(cmd->size, cmd->num, 19);
}

static void lsysi_pipe(lsys_turtle_state_l *cmd)
{
	cmd->angle += cmd->angle + char(cmd->max_angle/2);
	cmd->angle %= cmd->max_angle;
}

static void lsysi_pipe_pow2(lsys_turtle_state_l *cmd)
{
	cmd->angle += cmd->max_angle >> 1;
	cmd->angle &= cmd->dmaxangle;
}

static void lsysi_exclamation(lsys_turtle_state_l *cmd)
{
	cmd->reverse = ! cmd->reverse;
}

static void lsysi_size_dm(lsys_turtle_state_l *cmd)
{
	double angle = double(cmd->realangle)*ANGLE2DOUBLE;
	double s;
	double c;
	long fixedsin;
	long fixedcos;

	FPUsincos(&angle, &s, &c);
	fixedsin = long(s*FIXEDLT1);
	fixedcos = long(c*FIXEDLT1);

	cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));

	if (cmd->xpos > cmd->x_max)
	{
		cmd->x_max = cmd->xpos;
	}
	if (cmd->ypos > cmd->y_max)
	{
		cmd->y_max = cmd->ypos;
	}
	if (cmd->xpos < cmd->x_min)
	{
		cmd->x_min = cmd->xpos;
	}
	if (cmd->ypos < cmd->y_min)
	{
		cmd->y_min = cmd->ypos;
	}
}

static void lsysi_size_gf(lsys_turtle_state_l *cmd)
{
	cmd->xpos = cmd->xpos + (multiply(cmd->size, s_coss[int(cmd->angle)], 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, s_sins[int(cmd->angle)], 29));
	// xpos += size*coss[angle];
	// ypos += size*sins[angle];
	if (cmd->xpos > cmd->x_max)
	{
		cmd->x_max = cmd->xpos;
	}
	if (cmd->ypos > cmd->y_max)
	{
		cmd->y_max = cmd->ypos;
	}
	if (cmd->xpos < cmd->x_min)
	{
		cmd->x_min = cmd->xpos;
	}
	if (cmd->ypos < cmd->y_min)
	{
		cmd->y_min = cmd->ypos;
	}
}

static void lsysi_draw_d(lsys_turtle_state_l *cmd)
{
	double angle = double(cmd->realangle)*ANGLE2DOUBLE;
	double s;
	double c;
	long fixedsin;
	long fixedcos;
	int lastx;
	int lasty;

	FPUsincos(&angle, &s, &c);
	fixedsin = long(s*FIXEDLT1);
	fixedcos = long(c*FIXEDLT1);

	lastx = int(cmd->xpos >> 19);
	lasty = int(cmd->ypos >> 19);
	cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
	driver_draw_line(lastx, lasty, int(cmd->xpos >> 19), int(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_draw_m(lsys_turtle_state_l *cmd)
{
	double angle = double(cmd->realangle)*ANGLE2DOUBLE;
	double s;
	double c;
	long fixedsin;
	long fixedcos;

	FPUsincos(&angle, &s, &c);
	fixedsin = long(s*FIXEDLT1);
	fixedcos = long(c*FIXEDLT1);

	cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
}

static void lsysi_draw_g(lsys_turtle_state_l *cmd)
{
	cmd->xpos = cmd->xpos + (multiply(cmd->size, s_coss[int(cmd->angle)], 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, s_sins[int(cmd->angle)], 29));
	// xpos += size*coss[angle];
	// ypos += size*sins[angle];
}

static void lsysi_draw_f(lsys_turtle_state_l *cmd)
{
	int lastx = int(cmd->xpos >> 19);
	int lasty = int(cmd->ypos >> 19);
	cmd->xpos = cmd->xpos + (multiply(cmd->size, s_coss[int(cmd->angle)], 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, s_sins[int(cmd->angle)], 29));
	// xpos += size*coss[angle];
	// ypos += size*sins[angle];
	driver_draw_line(lastx, lasty, int(cmd->xpos >> 19), int(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_draw_c(lsys_turtle_state_l *cmd)
{
	cmd->curcolor = char(int(cmd->num) % g_colors);
}

static void lsysi_draw_gt(lsys_turtle_state_l *cmd)
{
	cmd->curcolor -= char(cmd->num);
	cmd->curcolor %= g_colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = char(g_colors-1);
	}
}

static void lsysi_draw_lt(lsys_turtle_state_l *cmd)
{
	cmd->curcolor += char(cmd->num);
	cmd->curcolor %= g_colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = 1;
	}
}

static lsys_cmd *find_size(lsys_cmd *command, lsys_turtle_state_l *ts, lsys_cmd **rules, int depth)
{
	lsys_cmd **rulind;
	bool tran;

	if (g_overflow)     // integer math routines overflowed
	{
		return 0;
	}

	while (command->ch && command->ch != ']')
	{
		if (! (ts->counter++))
		{
			// let user know we're not dead
			if (thinking(1, "L-System thinking (higher orders take longer)"))
			{
				ts->counter--;
				return 0;
			}
		}
		tran = false;
		if (depth)
		{
			for (rulind = rules; *rulind; rulind++)
			{
				if ((*rulind)->ch == command->ch)
				{
					tran = true;
					if (find_size((*rulind) + 1, ts, rules, depth-1) == 0)
					{
						return 0;
					}
				}
			}
		}
		if (!depth || !tran)
		{
			if (command->function)
			{
				ts->num = command->n;
				(*command->function)(ts);
			}
			else if (command->ch == '[')
			{
				char saveang;
				char saverev;
				long savesize;
				long savex;
				long savey;
				unsigned long saverang;

				saveang = ts->angle;
				saverev = ts->reverse;
				savesize = ts->size;
				saverang = ts->realangle;
				savex = ts->xpos;
				savey = ts->ypos;
				command = find_size(command + 1, ts, rules, depth);
				if (command == 0)
				{
					return 0;
				}
				ts->angle = saveang;
				ts->reverse = saverev;
				ts->size = savesize;
				ts->realangle = saverang;
				ts->xpos = savex;
				ts->ypos = savey;
			}
		}
		command++;
	}
	return command;
}

static int
lsysi_find_scale(lsys_cmd *command, lsys_turtle_state_l *ts, lsys_cmd **rules, int depth)
{
	double locaspect = g_screen_aspect_ratio*g_x_dots/g_y_dots;
	ts->aspect = FIXEDPT(locaspect);
	ts->xpos = 0;
	ts->ypos = 0;
	ts->x_min = 0;
	ts->x_max = 0;
	ts->y_max = 0;
	ts->y_min = 0;
	ts->realangle = 0;
	ts->angle = 0;
	ts->reverse = 0;
	ts->counter = 0;
	ts->size = FIXEDPT(1L);
	lsys_cmd *fsret = find_size(command, ts, rules, depth);
	thinking(0, 0); // erase thinking message if any
	double x_min = double(ts->x_min)/FIXEDMUL;
	double x_max = double(ts->x_max)/FIXEDMUL;
	double y_min = double(ts->y_min)/FIXEDMUL;
	double y_max = double(ts->y_max)/FIXEDMUL;
	if (fsret == 0)
	{
		return 0;
	}
	float horiz = (x_max == x_min) ? 1.0e37f : float((g_x_dots-10)/(x_max-x_min));
	float vert = (y_max == y_min) ? 1.0e37f : float((g_y_dots-6) /(y_max-y_min));
	double locsize = (vert < horiz) ? vert : horiz;

	ts->xpos = (horiz == 1e37) ?
		FIXEDPT(g_x_dots/2) : FIXEDPT((g_x_dots-locsize*(x_max + x_min))/2);
	ts->ypos = (vert == 1e37) ?
		FIXEDPT(g_y_dots/2) : FIXEDPT((g_y_dots-locsize*(y_max + y_min))/2);
	ts->size = FIXEDPT(locsize);

	return 1;
}

static lsys_cmd *
draw_lsysi(lsys_cmd *command, lsys_turtle_state_l *ts, lsys_cmd **rules, int depth)
{
	lsys_cmd **rulind;
	bool tran;

	if (g_overflow)     // integer math routines overflowed
	{
		return 0;
	}

	while (command->ch && command->ch != ']')
	{
		if (!(ts->counter++))
		{
			if (driver_key_pressed())
			{
				ts->counter--;
				return 0;
			}
		}
		tran = false;
		if (depth)
		{
			for (rulind = rules; *rulind; rulind++)
			{
				if ((*rulind)->ch == command->ch)
				{
					tran = true;
					if (draw_lsysi((*rulind) + 1, ts, rules, depth-1) == 0)
					{
						return 0;
					}
				}
			}
		}
		if (!depth || !tran)
		{
			if (command->function)
			{
				ts->num = command->n;
				(*command->function)(ts);
			}
			else if (command->ch == '[')
			{
				char saveang;
				char saverev;
				char savecolor;
				long savesize;
				long savex;
				long savey;
				unsigned long saverang;

				saveang = ts->angle;
				saverev = ts->reverse;
				savesize = ts->size;
				saverang = ts->realangle;
				savex = ts->xpos;
				savey = ts->ypos;
				savecolor = ts->curcolor;
				command = draw_lsysi(command + 1, ts, rules, depth);
				if (command == 0)
				{
					return 0;
				}
				ts->angle = saveang;
				ts->reverse = saverev;
				ts->size = savesize;
				ts->realangle = saverang;
				ts->xpos = savex;
				ts->ypos = savey;
				ts->curcolor = savecolor;
			}
		}
		command++;
	}
	return command;
}

static lsys_cmd *
lsysi_size_transform(char *s, lsys_turtle_state_l *ts)
{
	lsys_cmd *ret;
	lsys_cmd *doub;
	int maxval = 10;
	int n = 0;
	void (*function)(lsys_turtle_state_l *);
	long num;

	void (*plus)(lsys_turtle_state_l *) = (is_pow2(ts->max_angle)) ? lsysi_plus_pow2 : lsysi_plus;
	void (*minus)(lsys_turtle_state_l *) = (is_pow2(ts->max_angle)) ? lsysi_minus_pow2 : lsysi_minus;
	void (*pipe)(lsys_turtle_state_l *) = (is_pow2(ts->max_angle)) ? lsysi_pipe_pow2 : lsysi_pipe;

	void (*slash)(lsys_turtle_state_l *) =  lsysi_slash;
	void (*bslash)(lsys_turtle_state_l *) = lsysi_backslash;
	void (*at)(lsys_turtle_state_l *) =     lsysi_at;
	void (*dogf)(lsys_turtle_state_l *) =   lsysi_size_gf;

	ret = new lsys_cmd[maxval];
	if (ret == 0)
	{
		ts->stackoflow = true;
		return 0;
	}
	while (*s)
	{
		function = 0;
		num = 0;
		ret[n].ch = *s;
		switch (*s)
		{
		case '+': function = plus;            break;
		case '-': function = minus;           break;
		case '/': function = slash;           num = long(get_number(&s)*11930465L);    break;
		case '\\': function = bslash;         num = long(get_number(&s)*11930465L);    break;
		case '@': function = at;              num = FIXEDPT(get_number(&s));    break;
		case '|': function = pipe;            break;
		case '!': function = lsysi_exclamation;     break;
		case 'd':
		case 'm': function = lsysi_size_dm;   break;
		case 'g':
		case 'f': function = dogf;       break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
			break;
		}
		ret[n].function = function;
		ret[n].n = num;
		if (++n == maxval)
		{
			doub = new lsys_cmd[maxval*2];
			if (doub == 0)
			{
				delete[] ret;
				ts->stackoflow = true;
				return 0;
			}
			memcpy(doub, ret, maxval*sizeof(lsys_cmd));
			delete[] ret;
			ret = doub;
			maxval <<= 1;
		}
		s++;
	}
	ret[n].ch = 0;
	ret[n].function = 0;
	ret[n].n = 0;
	n++;

	doub = new lsys_cmd[n];
	if (doub == 0)
	{
		delete[] ret;
		ts->stackoflow = true;
		return 0;
	}
	memcpy(doub, ret, n*sizeof(lsys_cmd));
	delete[] ret;
	return doub;
}

static lsys_cmd *
lsysi_draw_transform(char *s, lsys_turtle_state_l *ts)
{
	lsys_cmd *ret;
	lsys_cmd *doub;
	int maxval = 10;
	int n = 0;
	void (*function)(lsys_turtle_state_l *);
	long num;

	void (*plus)(lsys_turtle_state_l *) = (is_pow2(ts->max_angle)) ? lsysi_plus_pow2 : lsysi_plus;
	void (*minus)(lsys_turtle_state_l *) = (is_pow2(ts->max_angle)) ? lsysi_minus_pow2 : lsysi_minus;
	void (*pipe)(lsys_turtle_state_l *) = (is_pow2(ts->max_angle)) ? lsysi_pipe_pow2 : lsysi_pipe;

	void (*slash)(lsys_turtle_state_l *) =  lsysi_slash;
	void (*bslash)(lsys_turtle_state_l *) = lsysi_backslash;
	void (*at)(lsys_turtle_state_l *) =     lsysi_at;
	void (*drawg)(lsys_turtle_state_l *) =  lsysi_draw_g;

	ret = new lsys_cmd[maxval];
	if (ret == 0)
	{
		ts->stackoflow = true;
		return 0;
	}
	while (*s)
	{
		function = 0;
		num = 0;
		ret[n].ch = *s;
		switch (*s)
		{
		case '+': function = plus;            break;
		case '-': function = minus;           break;
		case '/': function = slash;           num = long(get_number(&s)*11930465L);    break;
		case '\\': function = bslash;         num = long(get_number(&s)*11930465L);    break;
		case '@': function = at;              num = FIXEDPT(get_number(&s));    break;
		case '|': function = pipe;            break;
		case '!': function = lsysi_exclamation;     break;
		case 'd': function = lsysi_draw_d;    break;
		case 'm': function = lsysi_draw_m;    break;
		case 'g': function = drawg;           break;
		case 'f': function = lsysi_draw_f;    break;
		case 'c': function = lsysi_draw_c;    num = long(get_number(&s));    break;
		case '<': function = lsysi_draw_lt;   num = long(get_number(&s));    break;
		case '>': function = lsysi_draw_gt;   num = long(get_number(&s));    break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
			break;
		}
		ret[n].function = function;
		ret[n].n = num;
		if (++n == maxval)
		{
			doub = new lsys_cmd[maxval*2];
			if (doub == 0)
			{
				delete[] ret;
				ts->stackoflow = true;
				return 0;
			}
			memcpy(doub, ret, maxval*sizeof(lsys_cmd));
			delete[] ret;
			ret = doub;
			maxval <<= 1;
		}
		s++;
	}
	ret[n].ch = 0;
	ret[n].function = 0;
	ret[n].n = 0;
	n++;

	doub = new lsys_cmd[n];
	if (doub == 0)
	{
		delete[] ret;
		ts->stackoflow = true;
		return 0;
	}
	memcpy(doub, ret, n*sizeof(lsys_cmd));
	delete[] ret;
	return doub;
}

static void lsysi_sin_cos()
{
	double locaspect = g_screen_aspect_ratio*g_x_dots/g_y_dots;
	double twopimax = 2.0*MathUtil::Pi/g_max_angle;
	s_sins.resize(g_max_angle);
	s_coss.resize(g_max_angle);
	for (int i = 0; i < g_max_angle; i++)
	{
		double twopimaxi = i*twopimax;
		double s;
		double c;
		FPUsincos(&twopimaxi, &s, &c);
		s_sins[i] = long(s*FIXEDLT1);
		s_coss[i] = long((locaspect*c)*FIXEDLT1);
	}
}
