#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "lsys.h"
#include "drivers.h"
#include "MathUtil.h"

struct lsys_cmd
{
	void (*f)(struct lsys_turtle_state *);
	long n;
	char ch;
};

#define sins ((long *) g_box_y)
#define coss ((long *) g_box_y + 50) /* 50 after the start of sins */

int g_max_angle;

static char *ruleptrs[MAXRULES];
static struct lsys_cmd *rules2[MAXRULES];
static char loaded = 0;

static int _fastcall read_l_system_file(char *);
static void _fastcall free_rules_mem();
static int _fastcall rule_present(char symbol);
static int _fastcall save_rule(char *, char **);
static int _fastcall append_rule(char *rule, int index);
static void free_l_cmds();
static struct lsys_cmd *_fastcall find_size(struct lsys_cmd *, struct lsys_turtle_state *, struct lsys_cmd **, int);
static struct lsys_cmd *draw_lsysi(struct lsys_cmd *command, struct lsys_turtle_state *ts, struct lsys_cmd **rules, int depth);
static int lsysi_find_scale(struct lsys_cmd *command, struct lsys_turtle_state *ts, struct lsys_cmd **rules, int depth);
static struct lsys_cmd *lsysi_size_transform(char *s, struct lsys_turtle_state *ts);
static struct lsys_cmd *lsysi_draw_transform(char *s, struct lsys_turtle_state *ts);
static void _fastcall lsysi_sin_cos();
static void lsysi_slash(struct lsys_turtle_state *cmd);
static void lsysi_backslash(struct lsys_turtle_state *cmd);
static void lsysi_at(struct lsys_turtle_state *cmd);
static void lsysi_pipe(struct lsys_turtle_state *cmd);
static void lsysi_size_dm(struct lsys_turtle_state *cmd);
static void lsysi_size_gf(struct lsys_turtle_state *cmd);
static void lsysi_draw_d(struct lsys_turtle_state *cmd);
static void lsysi_draw_m(struct lsys_turtle_state *cmd);
static void lsysi_draw_g(struct lsys_turtle_state *cmd);
static void lsysi_draw_f(struct lsys_turtle_state *cmd);
static void lsysi_draw_c(struct lsys_turtle_state *cmd);
static void lsysi_draw_gt(struct lsys_turtle_state *cmd);
static void lsysi_draw_lt(struct lsys_turtle_state *cmd);

bool _fastcall is_pow2(int n)
{
	return n == (n & -n);
}

LDBL _fastcall get_number(char **str)
{
	char numstr[30];
	LDBL ret;
	int i;
	int root;
	int inverse;

	root = 0;
	inverse = 0;
	strcpy(numstr, "");
	(*str)++;
	switch (**str)
	{
	case 'q':
		root = 1;
		(*str)++;
		break;
	case 'i':
		inverse = 1;
		(*str)++;
		break;
	}
	switch (**str)
	{
	case 'q':
		root = 1;
		(*str)++;
		break;
	case 'i':
		inverse = 1;
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
	if (ret <= 0.0) /* this is a sanity check, JCO 8/31/94 */
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

static int _fastcall read_l_system_file(char *str)
{
	int c;
	char **rulind;
	int err = 0;
	int linenum;
	int check = 0;
	char inline1[MAX_LSYS_LINE_LEN + 1];
	char fixed[MAX_LSYS_LINE_LEN + 1];
	char *word;
	FILE *infile;
	char msgbuf[481]; /* enough for 6 full lines */

	if (find_file_item(g_l_system_filename, str, &infile, ITEMTYPE_L_SYSTEM) < 0)
	{
		return -1;
	}
	while ((c = fgetc(infile)) != '{')
	{
		if (c == EOF)
		{
			return -1;
		}
	}
	g_max_angle = 0;
	for (linenum = 0; linenum < MAXRULES; ++linenum)
	{
		ruleptrs[linenum] = NULL;
	}
	rulind = &ruleptrs[1];
	msgbuf[0] = (char)(linenum = 0);

	while (file_gets(inline1, MAX_LSYS_LINE_LEN, infile) > -1)  /* Max line length chars */
	{
		linenum++;
		word = strchr(inline1, ';');
		if (word != NULL) /* strip comment */
		{
			*word = 0;
		}
		strlwr(inline1);

		if ((int)strspn(inline1, " \t\n") < (int)strlen(inline1)) /* not a blank line */
		{
			word = strtok(inline1, " =\t\n");
			if (!strcmp(word, "axiom"))
			{
				if (save_rule(strtok(NULL, " \t\n"), &ruleptrs[0]))
				{
					strcat(msgbuf, "Error:  out of memory\n");
					++err;
					break;
				}
				check = 1;
			}
			else if (!strcmp(word, "angle"))
			{
				g_max_angle = (char)atoi(strtok(NULL, " \t\n"));
				check = 1;
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
					sprintf(&msgbuf[strlen(msgbuf)],
					"Syntax error line %d: Redefined reserved symbol %s\n", linenum, word);
					++err;
					break;
				}
				temp = strtok(NULL, " =\t\n");
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
					strcat(msgbuf, "Error:  out of memory\n");
					++err;
					break;
				}
				check = 1;
			}
			else
			{
				if (err < 6)
				{
					sprintf(&msgbuf[strlen(msgbuf)],
						"Syntax error line %d: %s\n", linenum, word);
					++err;
				}
			}
			if (check)
			{
				check = 0;
				word = strtok(NULL, " \t\n");
				if (word != NULL)
				{
					if (err < 6)
					{
						sprintf(&msgbuf[strlen(msgbuf)],
							"Extra text after command line %d: %s\n", linenum, word);
						++err;
					}
				}
			}
		}
	}
	fclose(infile);
	if (!ruleptrs[0] && err < 6)
	{
		strcat(msgbuf, "Error:  no axiom\n");
		++err;
	}
	if ((g_max_angle < 3 || g_max_angle > 50) && err < 6)
	{
		strcat(msgbuf, "Error:  illegal or missing angle\n");
		++err;
	}
	if (err)
	{
		msgbuf[strlen(msgbuf)-1] = 0; /* strip trailing \n */
		stop_message(0, msgbuf);
		return -1;
	}
	*rulind = NULL;
	return 0;
}

int l_system()
{
	int order;
	char **rulesc;
	struct lsys_cmd **sc;
	int stackoflow = 0;

	if ((!loaded) && l_load())
	{
		return -1;
	}

	g_overflow = 0;                /* reset integer math overflow flag */

	order = (int)g_parameters[0];
	if (order <= 0)
	{
		order = 0;
	}
	if (g_user_float_flag)
	{
		g_overflow = 1;
	}
	else
	{
		struct lsys_turtle_state ts;

		ts.stackoflow = 0;
		ts.max_angle = g_max_angle;
		ts.dmaxangle = (char)(g_max_angle - 1);

		sc = rules2;
		for (rulesc = ruleptrs; *rulesc; rulesc++)
		{
				*sc++ = lsysi_size_transform(*rulesc, &ts);
		}
		*sc = NULL;

		lsysi_sin_cos();
		if (lsysi_find_scale(rules2[0], &ts, &rules2[1], order))
		{
			ts.realangle = ts.angle = ts.reverse = 0;

			free_l_cmds();
			sc = rules2;
			for (rulesc = ruleptrs; *rulesc; rulesc++)
			{
				*sc++ = lsysi_draw_transform(*rulesc, &ts);
			}
			*sc = NULL;

			/* !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR */
			ts.curcolor = 15;
			if (ts.curcolor > g_colors)
			{
				ts.curcolor = (char)(g_colors-1);
			}
			draw_lsysi(rules2[0], &ts, &rules2[1], order);
		}
		stackoflow = ts.stackoflow;
	}

	if (stackoflow)
	{
		stop_message(0, "insufficient memory, try a lower order");
	}
	else if (g_overflow)
	{
		struct lsys_turtle_state_fp ts;

		g_overflow = 0;

		ts.stackoflow = 0;
		ts.max_angle = g_max_angle;
		ts.dmaxangle = (char)(g_max_angle - 1);

		sc = rules2;
		for (rulesc = ruleptrs; *rulesc; rulesc++)
		{
				*sc++ = lsysf_size_transform(*rulesc, &ts);
		}
		*sc = NULL;

		lsysf_sin_cos();
		if (lsysf_find_scale(rules2[0], &ts, &rules2[1], order))
		{
			ts.realangle = ts.angle = ts.reverse = 0;

			free_l_cmds();
			sc = rules2;
			for (rulesc = ruleptrs; *rulesc; rulesc++)
			{
				*sc++ = lsysf_draw_transform(*rulesc, &ts);
			}
			*sc = NULL;

			/* !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR */
			ts.curcolor = 15;
			if (ts.curcolor > g_colors)
			{
				ts.curcolor = (char)(g_colors-1);
			}
			draw_lsysf(rules2[0], &ts, &rules2[1], order);
		}
		g_overflow = 0;
	}
	free_rules_mem();
	free_l_cmds();
	loaded = 0;
	return 0;
}

int l_load()
{
	if (read_l_system_file(g_l_system_name))  /* error occurred */
	{
		free_rules_mem();
		loaded = 0;
		return -1;
	}
	loaded = 1;
	return 0;
}

static void _fastcall free_rules_mem()
{
	int i;
	for (i = 0; i < MAXRULES; ++i)
	{
		if (ruleptrs[i])
		{
			free(ruleptrs[i]);
		}
	}
}

static int _fastcall rule_present(char symbol)
{
	int i;

	for (i = 1; i < MAXRULES && ruleptrs[i] && *ruleptrs[i] != symbol; i++)
	{
		;
	}
	return (i < MAXRULES && ruleptrs[i]) ? i : 0;
}

static int _fastcall save_rule(char *rule, char **saveptr)
{
	int i;
	char *tmpfar;
	i = (int) strlen(rule) + 1;
	tmpfar = (char *) malloc(i);
	if (tmpfar == NULL)
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

static int _fastcall append_rule(char *rule, int index)
{
	char *dst;
	char *old;
	char *sav;
	int i;
	int j;

	old = sav = ruleptrs[index];
	for (i = 0; *(old++); i++)
	{
		;
	}
	j = (int) strlen(rule) + 1;
	dst = (char *)malloc((long)(i + j));
	if (dst == NULL)
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
	free(sav);
	return 0;
}

static void free_l_cmds()
{
	struct lsys_cmd **sc = rules2;

	while (*sc)
	{
		free(*sc++);
	}
}

/* integer specific routines */
static void lsysi_plus(struct lsys_turtle_state *cmd)
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

/* This is the same as lsys_doplus, except max_angle is a power of 2. */
static void lsysi_plus_pow2(struct lsys_turtle_state *cmd)
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

static void lsysi_minus(struct lsys_turtle_state *cmd)
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

static void lsysi_minus_pow2(struct lsys_turtle_state *cmd)
{
	cmd->reverse ? cmd->angle-- : cmd->angle++;
	cmd->angle &= cmd->dmaxangle;
}

static void lsysi_slash(struct lsys_turtle_state *cmd)
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

static void lsysi_backslash(struct lsys_turtle_state *cmd)
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

static void lsysi_at(struct lsys_turtle_state *cmd)
{
	cmd->size = multiply(cmd->size, cmd->num, 19);
}

static void lsysi_pipe(struct lsys_turtle_state *cmd)
{
	cmd->angle = (char)(cmd->angle + (char)(cmd->max_angle / 2));
	cmd->angle %= cmd->max_angle;
}

static void lsysi_pipe_pow2(struct lsys_turtle_state *cmd)
{
	cmd->angle += cmd->max_angle >> 1;
	cmd->angle &= cmd->dmaxangle;
}

static void lsysi_exclamation(struct lsys_turtle_state *cmd)
{
	cmd->reverse = ! cmd->reverse;
}

static void lsysi_size_dm(struct lsys_turtle_state *cmd)
{
	double angle = (double) cmd->realangle*ANGLE2DOUBLE;
	double s;
	double c;
	long fixedsin;
	long fixedcos;

	FPUsincos(&angle, &s, &c);
	fixedsin = (long) (s*FIXEDLT1);
	fixedcos = (long) (c*FIXEDLT1);

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

static void lsysi_size_gf(struct lsys_turtle_state *cmd)
{
	cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
	/* xpos += size*coss[angle]; */
	/* ypos += size*sins[angle]; */
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

static void lsysi_draw_d(struct lsys_turtle_state *cmd)
{
	double angle = (double) cmd->realangle*ANGLE2DOUBLE;
	double s;
	double c;
	long fixedsin;
	long fixedcos;
	int lastx;
	int lasty;

	FPUsincos(&angle, &s, &c);
	fixedsin = (long) (s*FIXEDLT1);
	fixedcos = (long) (c*FIXEDLT1);

	lastx = (int) (cmd->xpos >> 19);
	lasty = (int) (cmd->ypos >> 19);
	cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
	driver_draw_line(lastx, lasty, (int)(cmd->xpos >> 19), (int)(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_draw_m(struct lsys_turtle_state *cmd)
{
	double angle = (double) cmd->realangle*ANGLE2DOUBLE;
	double s;
	double c;
	long fixedsin;
	long fixedcos;

	FPUsincos(&angle, &s, &c);
	fixedsin = (long) (s*FIXEDLT1);
	fixedcos = (long) (c*FIXEDLT1);

	cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
}

static void lsysi_draw_g(struct lsys_turtle_state *cmd)
{
	cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
	/* xpos += size*coss[angle]; */
	/* ypos += size*sins[angle]; */
}

static void lsysi_draw_f(struct lsys_turtle_state *cmd)
{
	int lastx = (int) (cmd->xpos >> 19);
	int lasty = (int) (cmd->ypos >> 19);
	cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
	cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
	/* xpos += size*coss[angle]; */
	/* ypos += size*sins[angle]; */
	driver_draw_line(lastx, lasty, (int)(cmd->xpos >> 19), (int)(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_draw_c(struct lsys_turtle_state *cmd)
{
	cmd->curcolor = (char)(((int) cmd->num) % g_colors);
}

static void lsysi_draw_gt(struct lsys_turtle_state *cmd)
{
	cmd->curcolor = (char)(cmd->curcolor - (char)cmd->num);
	cmd->curcolor %= g_colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = (char)(g_colors-1);
	}
}

static void lsysi_draw_lt(struct lsys_turtle_state *cmd)
{
	cmd->curcolor = (char)(cmd->curcolor + (char)cmd->num);
	cmd->curcolor %= g_colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = 1;
	}
}

static struct lsys_cmd *_fastcall
find_size(struct lsys_cmd *command, struct lsys_turtle_state *ts, struct lsys_cmd **rules, int depth)
{
	struct lsys_cmd **rulind;
	int tran;

	if (g_overflow)     /* integer math routines overflowed */
	{
		return NULL;
	}

	if (stackavail() < 400)  /* leave some margin for calling subrtns */
	{
		ts->stackoflow = 1;
		return NULL;
	}

	while (command->ch && command->ch != ']')
	{
		if (! (ts->counter++))
		{
			/* let user know we're not dead */
			if (thinking(1, "L-System thinking (higher orders take longer)"))
			{
				ts->counter--;
				return NULL;
			}
		}
		tran = 0;
		if (depth)
		{
			for (rulind = rules; *rulind; rulind++)
			{
				if ((*rulind)->ch == command->ch)
				{
					tran = 1;
					if (find_size((*rulind) + 1, ts, rules, depth-1) == NULL)
					{
						return NULL;
					}
				}
			}
		}
		if (!depth || !tran)
		{
			if (command->f)
			{
				ts->num = command->n;
				(*command->f)(ts);
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
				if (command == NULL)
				{
					return NULL;
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
lsysi_find_scale(struct lsys_cmd *command, struct lsys_turtle_state *ts, struct lsys_cmd **rules, int depth)
{
	float horiz;
	float vert;
	double x_min;
	double x_max;
	double y_min;
	double y_max;
	double locsize;
	double locaspect;
	struct lsys_cmd *fsret;

	locaspect = g_screen_aspect_ratio*g_x_dots/g_y_dots;
	ts->aspect = FIXEDPT(locaspect);
	ts->xpos =
	ts->ypos =
	ts->x_min =
	ts->x_max =
	ts->y_max =
	ts->y_min =
	ts->realangle =
	ts->angle =
	ts->reverse =
	ts->counter = 0;
	ts->size = FIXEDPT(1L);
	fsret = find_size(command, ts, rules, depth);
	thinking(0, NULL); /* erase thinking message if any */
	x_min = (double) ts->x_min / FIXEDMUL;
	x_max = (double) ts->x_max / FIXEDMUL;
	y_min = (double) ts->y_min / FIXEDMUL;
	y_max = (double) ts->y_max / FIXEDMUL;
	if (fsret == NULL)
	{
		return 0;
	}
	horiz = (x_max == x_min) ? 1.0e37f : (float)((g_x_dots-10)/(x_max-x_min));
	vert  = (y_max == y_min) ? 1.0e37f : (float)((g_y_dots-6) /(y_max-y_min));
	locsize = (vert < horiz) ? vert : horiz;

	ts->xpos = (horiz == 1e37) ?
		FIXEDPT(g_x_dots/2) : FIXEDPT((g_x_dots-locsize*(x_max + x_min))/2);
	ts->ypos = (vert == 1e37) ?
		FIXEDPT(g_y_dots/2) : FIXEDPT((g_y_dots-locsize*(y_max + y_min))/2);
	ts->size = FIXEDPT(locsize);

	return 1;
}

static struct lsys_cmd *
draw_lsysi(struct lsys_cmd *command, struct lsys_turtle_state *ts, struct lsys_cmd **rules, int depth)
{
	struct lsys_cmd **rulind;
	int tran;

	if (g_overflow)     /* integer math routines overflowed */
	{
		return NULL;
	}

	if (stackavail() < 400)  /* leave some margin for calling subrtns */
	{
		ts->stackoflow = 1;
		return NULL;
	}


	while (command->ch && command->ch != ']')
	{
		if (!(ts->counter++))
		{
			if (driver_key_pressed())
			{
				ts->counter--;
				return NULL;
			}
		}
		tran = 0;
		if (depth)
		{
			for (rulind = rules; *rulind; rulind++)
			{
				if ((*rulind)->ch == command->ch)
				{
					tran = 1;
					if (draw_lsysi((*rulind) + 1, ts, rules, depth-1) == NULL)
					{
						return NULL;
					}
				}
			}
		}
		if (!depth || !tran)
		{
			if (command->f)
			{
				ts->num = command->n;
				(*command->f)(ts);
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
				if (command == NULL)
				{
					return NULL;
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

static struct lsys_cmd *
lsysi_size_transform(char *s, struct lsys_turtle_state *ts)
{
	struct lsys_cmd *ret;
	struct lsys_cmd *doub;
	int maxval = 10;
	int n = 0;
	void (*f)(lsys_turtle_state *);
	long num;

	void (*plus)(lsys_turtle_state *) = (is_pow2(ts->max_angle)) ? lsysi_plus_pow2 : lsysi_plus;
	void (*minus)(lsys_turtle_state *) = (is_pow2(ts->max_angle)) ? lsysi_minus_pow2 : lsysi_minus;
	void (*pipe)(lsys_turtle_state *) = (is_pow2(ts->max_angle)) ? lsysi_pipe_pow2 : lsysi_pipe;

	void (*slash)(lsys_turtle_state *) =  lsysi_slash;
	void (*bslash)(lsys_turtle_state *) = lsysi_backslash;
	void (*at)(lsys_turtle_state *) =     lsysi_at;
	void (*dogf)(lsys_turtle_state *) =   lsysi_size_gf;

	ret = (struct lsys_cmd *) malloc((long) maxval*sizeof(struct lsys_cmd));
	if (ret == NULL)
	{
		ts->stackoflow = 1;
		return NULL;
	}
	while (*s)
	{
		f = NULL;
		num = 0;
		ret[n].ch = *s;
		switch (*s)
		{
		case '+': f = plus;            break;
		case '-': f = minus;           break;
		case '/': f = slash;           num = (long) (get_number(&s)*11930465L);    break;
		case '\\': f = bslash;         num = (long) (get_number(&s)*11930465L);    break;
		case '@': f = at;              num = FIXEDPT(get_number(&s));    break;
		case '|': f = pipe;            break;
		case '!': f = lsysi_exclamation;     break;
		case 'd':
		case 'm': f = lsysi_size_dm;   break;
		case 'g':
		case 'f': f = dogf;       break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
		break;
		}
		ret[n].f = (void (*)(struct lsys_turtle_state *))f;
		ret[n].n = num;
		if (++n == maxval)
		{
			doub = (struct lsys_cmd *) malloc((long) maxval*2*sizeof(struct lsys_cmd));
			if (doub == NULL)
			{
				free(ret);
				ts->stackoflow = 1;
				return NULL;
			}
			memcpy(doub, ret, maxval*sizeof(struct lsys_cmd));
			free(ret);
			ret = doub;
			maxval <<= 1;
		}
		s++;
	}
	ret[n].ch = 0;
	ret[n].f = NULL;
	ret[n].n = 0;
	n++;

	doub = (struct lsys_cmd *) malloc((long) n*sizeof(struct lsys_cmd));
	if (doub == NULL)
	{
		free(ret);
		ts->stackoflow = 1;
		return NULL;
	}
	memcpy(doub, ret, n*sizeof(struct lsys_cmd));
	free(ret);
	return doub;
}

static struct lsys_cmd *
lsysi_draw_transform(char *s, struct lsys_turtle_state *ts)
{
	struct lsys_cmd *ret;
	struct lsys_cmd *doub;
	int maxval = 10;
	int n = 0;
	void (*f)(lsys_turtle_state *);
	long num;

	void (*plus)(lsys_turtle_state *) = (is_pow2(ts->max_angle)) ? lsysi_plus_pow2 : lsysi_plus;
	void (*minus)(lsys_turtle_state *) = (is_pow2(ts->max_angle)) ? lsysi_minus_pow2 : lsysi_minus;
	void (*pipe)(lsys_turtle_state *) = (is_pow2(ts->max_angle)) ? lsysi_pipe_pow2 : lsysi_pipe;

	void (*slash)(lsys_turtle_state *) =  lsysi_slash;
	void (*bslash)(lsys_turtle_state *) = lsysi_backslash;
	void (*at)(lsys_turtle_state *) =     lsysi_at;
	void (*drawg)(lsys_turtle_state *) =  lsysi_draw_g;

	ret = (struct lsys_cmd *) malloc((long) maxval*sizeof(struct lsys_cmd));
	if (ret == NULL)
	{
		ts->stackoflow = 1;
		return NULL;
	}
	while (*s)
	{
		f = NULL;
		num = 0;
		ret[n].ch = *s;
		switch (*s)
		{
		case '+': f = plus;            break;
		case '-': f = minus;           break;
		case '/': f = slash;           num = (long) (get_number(&s)*11930465L);    break;
		case '\\': f = bslash;         num = (long) (get_number(&s)*11930465L);    break;
		case '@': f = at;              num = FIXEDPT(get_number(&s));    break;
		case '|': f = pipe;            break;
		case '!': f = lsysi_exclamation;     break;
		case 'd': f = lsysi_draw_d;    break;
		case 'm': f = lsysi_draw_m;    break;
		case 'g': f = drawg;           break;
		case 'f': f = lsysi_draw_f;    break;
		case 'c': f = lsysi_draw_c;    num = (long) get_number(&s);    break;
		case '<': f = lsysi_draw_lt;   num = (long) get_number(&s);    break;
		case '>': f = lsysi_draw_gt;   num = (long) get_number(&s);    break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
		break;
		}
		ret[n].f = (void (*)(struct lsys_turtle_state *))f;
		ret[n].n = num;
		if (++n == maxval)
		{
			doub = (struct lsys_cmd *) malloc((long) maxval*2*sizeof(struct lsys_cmd));
			if (doub == NULL)
			{
				free(ret);
				ts->stackoflow = 1;
				return NULL;
			}
			memcpy(doub, ret, maxval*sizeof(struct lsys_cmd));
			free(ret);
			ret = doub;
			maxval <<= 1;
		}
		s++;
	}
	ret[n].ch = 0;
	ret[n].f = NULL;
	ret[n].n = 0;
	n++;

	doub = (struct lsys_cmd *) malloc((long) n*sizeof(struct lsys_cmd));
	if (doub == NULL)
	{
		free(ret);
		ts->stackoflow = 1;
		return NULL;
	}
	memcpy(doub, ret, n*sizeof(struct lsys_cmd));
	free(ret);
	return doub;
}

static void _fastcall lsysi_sin_cos()
{
	double locaspect = g_screen_aspect_ratio*g_x_dots/g_y_dots;
	double twopimax = 2.0*MathUtil::Pi / g_max_angle;
	for (int i = 0; i < g_max_angle; i++)
	{
		double twopimaxi = i*twopimax;
		double s;
		double c;
		FPUsincos(&twopimaxi, &s, &c);
		sins[i] = (long) (s*FIXEDLT1);
		coss[i] = (long) ((locaspect*c)*FIXEDLT1);
	}
}
