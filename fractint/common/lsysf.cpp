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

#ifdef max
#undef max
#endif

struct lsys_cmd
{
	void (*f)(struct lsys_turtle_state_fp *);
	int ptype;
	union
	{
		long n;
		LDBL nf;
	} parm;
	char ch;
};

#define sins_f ((LDBL *) g_box_y)
#define coss_f ((LDBL *) g_box_y + 50)

static struct lsys_cmd *_fastcall find_size(struct lsys_cmd *, struct lsys_turtle_state_fp *, struct lsys_cmd **, int);

static void lsysf_plus(struct lsys_turtle_state_fp *cmd)
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
static void lsysf_plus_pow2(struct lsys_turtle_state_fp *cmd)
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

static void lsysf_minus(struct lsys_turtle_state_fp *cmd)
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

static void lsysf_minus_pow2(struct lsys_turtle_state_fp *cmd)
{
	if (cmd->reverse)
	{
		cmd->angle--;
		cmd->angle &= cmd->dmaxangle;
	}
	else
	{
		cmd->angle++;
		cmd->angle &= cmd->dmaxangle;
	}
}

static void lsysf_slash(struct lsys_turtle_state_fp *cmd)
{
	if (cmd->reverse)
	{
		cmd->realangle -= cmd->parm.nf;
	}
	else
	{
		cmd->realangle += cmd->parm.nf;
	}
}

static void lsysf_backslash(struct lsys_turtle_state_fp *cmd)
{
	if (cmd->reverse)
	{
		cmd->realangle += cmd->parm.nf;
	}
	else
	{
		cmd->realangle -= cmd->parm.nf;
	}
}

static void lsysf_at(struct lsys_turtle_state_fp *cmd)
{
	cmd->size *= cmd->parm.nf;
}

static void lsysf_pipe(struct lsys_turtle_state_fp *cmd)
{
	cmd->angle = (char)(cmd->angle + cmd->max_angle / 2);
	cmd->angle %= cmd->max_angle;
}

static void lsysf_pipe_pow2(struct lsys_turtle_state_fp *cmd)
{
	cmd->angle += cmd->max_angle >> 1;
	cmd->angle &= cmd->dmaxangle;
}

static void lsysf_exclamation(struct lsys_turtle_state_fp *cmd)
{
	cmd->reverse = ! cmd->reverse;
}

static void lsysf_size_dm(struct lsys_turtle_state_fp *cmd)
{
	double angle = (double) cmd->realangle;
	double s;
	double c;

	s = sin(angle);
	c = cos(angle);

	cmd->xpos += cmd->size*cmd->aspect*c;
	cmd->ypos += cmd->size*s;

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

static void lsysf_size_gf(struct lsys_turtle_state_fp *cmd)
{
	cmd->xpos += cmd->size*coss_f[(int)cmd->angle];
	cmd->ypos += cmd->size*sins_f[(int)cmd->angle];

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

static void lsysf_draw_d(struct lsys_turtle_state_fp *cmd)
{
	double angle = (double) cmd->realangle;
	double s;
	double c;
	int lastx;
	int lasty;
	s = sin(angle);
	c = cos(angle);

	lastx = (int) cmd->xpos;
	lasty = (int) cmd->ypos;

	cmd->xpos += cmd->size*cmd->aspect*c;
	cmd->ypos += cmd->size*s;

	driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}

static void lsysf_draw_m(struct lsys_turtle_state_fp *cmd)
{
	double angle = (double) cmd->realangle;
	double s;
	double c;

	s = sin(angle);
	c = cos(angle);

	cmd->xpos += cmd->size*cmd->aspect*c;
	cmd->ypos += cmd->size*s;
}

static void lsysf_draw_g(struct lsys_turtle_state_fp *cmd)
{
	cmd->xpos += cmd->size*coss_f[(int)cmd->angle];
	cmd->ypos += cmd->size*sins_f[(int)cmd->angle];
}

static void lsysf_draw_f(struct lsys_turtle_state_fp *cmd)
{
	int lastx = (int) cmd->xpos;
	int lasty = (int) cmd->ypos;
	cmd->xpos += cmd->size*coss_f[(int)cmd->angle];
	cmd->ypos += cmd->size*sins_f[(int)cmd->angle];
	driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}

static void lsysf_draw_c(struct lsys_turtle_state_fp *cmd)
{
	cmd->curcolor = (char)(((int) cmd->parm.n) % g_colors);
}

static void lsysf_draw_gt(struct lsys_turtle_state_fp *cmd)
{
	cmd->curcolor = (char)(cmd->curcolor - cmd->parm.n);
	cmd->curcolor %= g_colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = (char)(g_colors-1);
	}
}

static void lsysf_draw_lt(struct lsys_turtle_state_fp *cmd)
{
	cmd->curcolor = (char)(cmd->curcolor + cmd->parm.n);
	cmd->curcolor %= g_colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = 1;
	}
}

static struct lsys_cmd *_fastcall
find_size(struct lsys_cmd *command, struct lsys_turtle_state_fp *ts, struct lsys_cmd **rules, int depth)
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
				switch (command->ptype)
				{
				case 4:
					ts->parm.n = command->parm.n;
					break;
				case 10:
					ts->parm.nf = command->parm.nf;
					break;
				default:
					break;
				}
				(*command->f)(ts);
			}
			else if (command->ch == '[')
			{
				char saveang;
				char saverev;
				LDBL savesize;
				LDBL savex;
				LDBL savey;
				LDBL saverang;

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

int _fastcall
lsysf_find_scale(struct lsys_cmd *command, struct lsys_turtle_state_fp *ts, struct lsys_cmd **rules, int depth)
{
	float horiz;
	float vert;
	LDBL x_min;
	LDBL x_max;
	LDBL y_min;
	LDBL y_max;
	LDBL locsize;
	LDBL locaspect;
	struct lsys_cmd *fsret;

	locaspect = g_screen_aspect_ratio*g_x_dots/g_y_dots;
	ts->aspect = locaspect;
	ts->xpos =
	ts->ypos =
	ts->x_min =
	ts->x_max =
	ts->y_max =
	ts->y_min = 0;
	ts->angle =
	ts->reverse =
	ts->counter = 0;
	ts->realangle = 0;
	ts->size = 1;
	fsret = find_size(command, ts, rules, depth);
	thinking(0, NULL); /* erase thinking message if any */
	x_min = ts->x_min;
	x_max = ts->x_max;
	y_min = ts->y_min;
	y_max = ts->y_max;
	if (fsret == NULL)
	{
		return 0;
	}
	horiz = (x_max == x_min) ? 1.0e37f : (float)((g_x_dots-10)/(x_max-x_min));
	vert  = (y_max == y_min) ? 1.0e37f : (float)((g_y_dots-6) /(y_max-y_min));
	locsize = (vert < horiz) ? vert : horiz;

	ts->xpos = (horiz == 1.0e37f) ? g_x_dots/2 : (g_x_dots-locsize*(x_max + x_min))/2;
	ts->ypos = (vert  == 1.0e37f) ? g_y_dots/2 : (g_y_dots-locsize*(y_max + y_min))/2;
	ts->size = locsize;

	return 1;
}

struct lsys_cmd *_fastcall
draw_lsysf(struct lsys_cmd *command, struct lsys_turtle_state_fp *ts, struct lsys_cmd **rules, int depth)
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
					if (draw_lsysf((*rulind) + 1, ts, rules, depth-1) == NULL)
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
				switch (command->ptype)
				{
				case 4:
					ts->parm.n = command->parm.n;
					break;
				case 10:
					ts->parm.nf = command->parm.nf;
					break;
				default:
					break;
				}
				(*command->f)(ts);
			}
			else if (command->ch == '[')
			{
				char saveang;
				char saverev;
				char savecolor;
				LDBL savesize;
				LDBL savex;
				LDBL savey;
				LDBL saverang;

				saveang = ts->angle;
				saverev = ts->reverse;
				savesize = ts->size;
				saverang = ts->realangle;
				savex = ts->xpos;
				savey = ts->ypos;
				savecolor = ts->curcolor;
				command = draw_lsysf(command + 1, ts, rules, depth);
				if (command == NULL)
					return NULL;
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

struct lsys_cmd *
lsysf_size_transform(char *s, struct lsys_turtle_state_fp *ts)
{
	struct lsys_cmd *ret;
	struct lsys_cmd *doub;
	int max = 10;
	int n = 0;
	void (*f)(lsys_turtle_state_fp *);
	long num;
	int ptype;

	void (*plus)(lsys_turtle_state_fp *) = (is_pow2(ts->max_angle)) ? lsysf_plus_pow2 : lsysf_plus;
	void (*minus)(lsys_turtle_state_fp *) = (is_pow2(ts->max_angle)) ? lsysf_minus_pow2 : lsysf_minus;
	void (*pipe)(lsys_turtle_state_fp *) = (is_pow2(ts->max_angle)) ? lsysf_pipe_pow2 : lsysf_pipe;

	void (*slash)(lsys_turtle_state_fp *) =  lsysf_slash;
	void (*bslash)(lsys_turtle_state_fp *) = lsysf_backslash;
	void (*at)(lsys_turtle_state_fp *) =     lsysf_at;
	void (*dogf)(lsys_turtle_state_fp *) =   lsysf_size_gf;

	ret = (struct lsys_cmd *) malloc((long) max*sizeof(struct lsys_cmd));
	if (ret == NULL)
	{
		ts->stackoflow = 1;
		return NULL;
	}
	while (*s)
	{
		f = NULL;
		num = 0;
		ptype = 4;
		ret[n].ch = *s;
		switch (*s)
		{
		case '+':
			f = plus;
			break;
		case '-':
			f = minus;
			break;
		case '/':
			f = slash;
			ptype = 10;
			ret[n].parm.nf = MathUtil::DegreesToRadians(get_number(&s));
			break;
		case '\\':
			f = bslash;
			ptype = 10;
			ret[n].parm.nf = MathUtil::DegreesToRadians(get_number(&s));
			break;
		case '@':
			f = at;
			ptype = 10;
			ret[n].parm.nf = get_number(&s);
			break;
		case '|':
			f = pipe;
			break;
		case '!':
			f = lsysf_exclamation;
			break;
		case 'd':
		case 'm':
			f = lsysf_size_dm;
			break;
		case 'g':
		case 'f':
			f = dogf;
			break;
		case '[':
			num = 1;
			break;
		case ']':
			num = 2;
			break;
		default:
			num = 3;
			break;
		}
		ret[n].f = f;
		if (ptype == 4)
		{
			ret[n].parm.n = num;
		}
		ret[n].ptype = ptype;
		if (++n == max)
		{
			doub = (struct lsys_cmd *) malloc((long) max*2*sizeof(struct lsys_cmd));
			if (doub == NULL)
			{
				free(ret);
				ts->stackoflow = 1;
				return NULL;
			}
			memcpy(doub, ret, max*sizeof(struct lsys_cmd));
			free(ret);
			ret = doub;
			max <<= 1;
		}
		s++;
	}
	ret[n].ch = 0;
	ret[n].f = NULL;
	ret[n].parm.n = 0;
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

struct lsys_cmd *
lsysf_draw_transform(char *s, struct lsys_turtle_state_fp *ts)
{
	struct lsys_cmd *ret;
	struct lsys_cmd *doub;
	int max = 10;
	int n = 0;
	void (*f)(lsys_turtle_state_fp *);
	LDBL num;
	int ptype;

	void (*plus)(lsys_turtle_state_fp *) = (is_pow2(ts->max_angle)) ? lsysf_plus_pow2 : lsysf_plus;
	void (*minus)(lsys_turtle_state_fp *) = (is_pow2(ts->max_angle)) ? lsysf_minus_pow2 : lsysf_minus;
	void (*pipe)(lsys_turtle_state_fp *) = (is_pow2(ts->max_angle)) ? lsysf_pipe_pow2 : lsysf_pipe;

	void (*slash)(lsys_turtle_state_fp *) =  lsysf_slash;
	void (*bslash)(lsys_turtle_state_fp *) = lsysf_backslash;
	void (*at)(lsys_turtle_state_fp *) =     lsysf_at;
	void (*drawg)(lsys_turtle_state_fp *) =  lsysf_draw_g;

	ret = (struct lsys_cmd *) malloc((long) max*sizeof(struct lsys_cmd));
	if (ret == NULL)
	{
		ts->stackoflow = 1;
		return NULL;
	}
	while (*s)
	{
		f = NULL;
		num = 0;
		ptype = 4;
		ret[n].ch = *s;
		switch (*s)
		{
		case '+': f = plus;            break;
		case '-': f = minus;           break;
		case '/': f = slash;           ptype = 10;  ret[n].parm.nf = MathUtil::DegreesToRadians(get_number(&s));  break;
		case '\\': f = bslash;         ptype = 10;  ret[n].parm.nf = MathUtil::DegreesToRadians(get_number(&s));  break;
		case '@': f = at;              ptype = 10;  ret[n].parm.nf = get_number(&s);  break;
		case '|': f = pipe;            break;
		case '!': f = lsysf_exclamation;    break;
		case 'd': f = lsysf_draw_d;   break;
		case 'm': f = lsysf_draw_m;   break;
		case 'g': f = drawg;           break;
		case 'f': f = lsysf_draw_f;   break;
		case 'c': f = lsysf_draw_c;   num = get_number(&s);    break;
		case '<': f = lsysf_draw_lt;  num = get_number(&s);    break;
		case '>': f = lsysf_draw_gt;  num = get_number(&s);    break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
			break;
		}
		ret[n].f = (void (*)(struct lsys_turtle_state_fp *))f;
		if (ptype == 4)
		{
			ret[n].parm.n = (long)num;
		}
		ret[n].ptype = ptype;
		if (++n == max)
		{
			doub = (struct lsys_cmd *) malloc((long) max*2*sizeof(struct lsys_cmd));
			if (doub == NULL)
			{
				free(ret);
				ts->stackoflow = 1;
				return NULL;
			}
			memcpy(doub, ret, max*sizeof(struct lsys_cmd));
			free(ret);
			ret = doub;
			max <<= 1;
		}
		s++;
	}
	ret[n].ch = 0;
	ret[n].f = NULL;
	ret[n].parm.n = 0;
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

void _fastcall lsysf_sin_cos()
{
	LDBL locaspect;
	LDBL TWOPI = 2.0*MathUtil::Pi;
	LDBL twopimax;
	LDBL twopimaxi;
	int i;

	locaspect = g_screen_aspect_ratio*g_x_dots/g_y_dots;
	twopimax = TWOPI / g_max_angle;
	for (i = 0; i < g_max_angle; i++)
	{
		twopimaxi = i*twopimax;
		sins_f[i] = sinl(twopimaxi);
		coss_f[i] = locaspect*cosl(twopimaxi);
	}
}
