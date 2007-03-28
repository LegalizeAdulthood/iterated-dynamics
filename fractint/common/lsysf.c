#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "lsys.h"
#include "drivers.h"

#ifdef max
#undef max
#endif

struct lsys_cmd
{
	void (*f)(struct lsys_turtlestatef *);
	int ptype;
	union
	{
		long n;
		LDBL nf;
	} parm;
	char ch;
};

#define sins_f ((LDBL *)(boxy))
#define coss_f (((LDBL *)(boxy) + 50))

static struct lsys_cmd *_fastcall findsize(struct lsys_cmd *, struct lsys_turtlestatef *, struct lsys_cmd **, int);

/* Define blanks for portability */
#if defined(XFRACT) || defined(_WIN32)
void lsysf_prepfpu(struct lsys_turtlestatef *x)
{ }
void lsysf_donefpu(struct lsys_turtlestatef *x)
{ }
#endif


#if defined(XFRACT) || defined(_WIN32)
static void lsysf_doplus(struct lsys_turtlestatef *cmd)
{
	if (cmd->reverse)
	{
		if (++cmd->angle == cmd->maxangle)
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
#else
extern void lsysf_doplus(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
/* This is the same as lsys_doplus, except maxangle is a power of 2. */
static void lsysf_doplus_pow2(struct lsys_turtlestatef *cmd)
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
#else
extern void lsysf_doplus_pow2(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dominus(struct lsys_turtlestatef *cmd)
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
		if (++cmd->angle == cmd->maxangle)
		{
			cmd->angle = 0;
		}
	}
}
#else
extern void lsysf_dominus(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dominus_pow2(struct lsys_turtlestatef *cmd)
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
#else
extern void lsysf_dominus_pow2(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_doslash(struct lsys_turtlestatef *cmd)
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
#else
extern void lsysf_doslash(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dobslash(struct lsys_turtlestatef *cmd)
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
#else
extern void lsysf_dobslash(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_doat(struct lsys_turtlestatef *cmd)
{
	cmd->size *= cmd->parm.nf;
}
#else
extern void lsysf_doat(struct lsys_turtlestatef *cmd, long n);
#endif

static void
lsysf_dopipe(struct lsys_turtlestatef *cmd)
{
	cmd->angle = (char)(cmd->angle + cmd->maxangle / 2);
	cmd->angle %= cmd->maxangle;
}

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dopipe_pow2(struct lsys_turtlestatef *cmd)
{
	cmd->angle += cmd->maxangle >> 1;
	cmd->angle &= cmd->dmaxangle;
}
#else
extern void lsysf_dopipe_pow2(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dobang(struct lsys_turtlestatef *cmd)
{
	cmd->reverse = ! cmd->reverse;
}
#else
extern void lsysf_dobang(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dosizedm(struct lsys_turtlestatef *cmd)
{
	double angle = (double) cmd->realangle;
	double s, c;

	s = sin(angle);
	c = cos(angle);

	cmd->xpos += cmd->size*cmd->aspect*c;
	cmd->ypos += cmd->size*s;

	if (cmd->xpos > cmd->xmax)
	{
		cmd->xmax = cmd->xpos;
	}
	if (cmd->ypos > cmd->ymax)
	{
		cmd->ymax = cmd->ypos;
	}
	if (cmd->xpos < cmd->xmin)
	{
		cmd->xmin = cmd->xpos;
	}
	if (cmd->ypos < cmd->ymin)
	{
		cmd->ymin = cmd->ypos;
	}
}
#else
extern void lsysf_dosizedm(struct lsys_turtlestatef *cmd, long n);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dosizegf(struct lsys_turtlestatef *cmd)
{
	cmd->xpos += cmd->size*coss_f[(int)cmd->angle];
	cmd->ypos += cmd->size*sins_f[(int)cmd->angle];

	if (cmd->xpos > cmd->xmax)
	{
		cmd->xmax = cmd->xpos;
	}
	if (cmd->ypos > cmd->ymax)
	{
		cmd->ymax = cmd->ypos;
	}
	if (cmd->xpos < cmd->xmin)
	{
		cmd->xmin = cmd->xpos;
	}
	if (cmd->ypos < cmd->ymin)
	{
		cmd->ymin = cmd->ypos;
	}
}
#else
extern void lsysf_dosizegf(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawd(struct lsys_turtlestatef *cmd)
{
	double angle = (double) cmd->realangle;
	double s, c;
	int lastx, lasty;
	s = sin(angle);
	c = cos(angle);

	lastx = (int) cmd->xpos;
	lasty = (int) cmd->ypos;

	cmd->xpos += cmd->size*cmd->aspect*c;
	cmd->ypos += cmd->size*s;

	driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}
#else
extern void lsysf_dodrawd(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawm(struct lsys_turtlestatef *cmd)
{
	double angle = (double) cmd->realangle;
	double s, c;

	s = sin(angle);
	c = cos(angle);

	cmd->xpos += cmd->size*cmd->aspect*c;
	cmd->ypos += cmd->size*s;
}
#else
extern void lsysf_dodrawm(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawg(struct lsys_turtlestatef *cmd)
{
	cmd->xpos += cmd->size*coss_f[(int)cmd->angle];
	cmd->ypos += cmd->size*sins_f[(int)cmd->angle];
}
#else
extern void lsysf_dodrawg(struct lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawf(struct lsys_turtlestatef *cmd)
{
	int lastx = (int) cmd->xpos;
	int lasty = (int) cmd->ypos;
	cmd->xpos += cmd->size*coss_f[(int)cmd->angle];
	cmd->ypos += cmd->size*sins_f[(int)cmd->angle];
	driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}
#else
extern void lsysf_dodrawf(struct lsys_turtlestatef *cmd);
#endif

static void lsysf_dodrawc(struct lsys_turtlestatef *cmd)
{
	cmd->curcolor = (char)(((int) cmd->parm.n) % colors);
}

static void lsysf_dodrawgt(struct lsys_turtlestatef *cmd)
{
	cmd->curcolor = (char)(cmd->curcolor - cmd->parm.n);
	cmd->curcolor %= colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = (char)(colors-1);
	}
}

static void lsysf_dodrawlt(struct lsys_turtlestatef *cmd)
{
	cmd->curcolor = (char)(cmd->curcolor + cmd->parm.n);
	cmd->curcolor %= colors;
	if (cmd->curcolor == 0)
	{
		cmd->curcolor = 1;
	}
}

static struct lsys_cmd *_fastcall
findsize(struct lsys_cmd *command, struct lsys_turtlestatef *ts, struct lsys_cmd **rules, int depth)
{
	struct lsys_cmd **rulind;
	int tran;

	if (overflow)     /* integer math routines overflowed */
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
					if (findsize((*rulind) + 1, ts, rules, depth-1) == NULL)
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
				char saveang, saverev;
				LDBL savesize, savex, savey, saverang;

				lsys_donefpu(ts);
				saveang = ts->angle;
				saverev = ts->reverse;
				savesize = ts->size;
				saverang = ts->realangle;
				savex = ts->xpos;
				savey = ts->ypos;
				lsys_prepfpu(ts);
				command = findsize(command + 1, ts, rules, depth);
				if (command == NULL)
				{
					return NULL;
				}
				lsys_donefpu(ts);
				ts->angle = saveang;
				ts->reverse = saverev;
				ts->size = savesize;
				ts->realangle = saverang;
				ts->xpos = savex;
				ts->ypos = savey;
				lsys_prepfpu(ts);
			}
		}
		command++;
	}
	return command;
}

int _fastcall
lsysf_findscale(struct lsys_cmd *command, struct lsys_turtlestatef *ts, struct lsys_cmd **rules, int depth)
{
	float horiz, vert;
	LDBL xmin, xmax, ymin, ymax;
	LDBL locsize;
	LDBL locaspect;
	struct lsys_cmd *fsret;

	locaspect = screenaspect*xdots/ydots;
	ts->aspect = locaspect;
	ts->xpos =
	ts->ypos =
	ts->xmin =
	ts->xmax =
	ts->ymax =
	ts->ymin = 0;
	ts->angle =
	ts->reverse =
	ts->counter = 0;
	ts->realangle = 0;
	ts->size = 1;
	lsys_prepfpu(ts);
	fsret = findsize(command, ts, rules, depth);
	lsys_donefpu(ts);
	thinking(0, NULL); /* erase thinking message if any */
	xmin = ts->xmin;
	xmax = ts->xmax;
	ymin = ts->ymin;
	ymax = ts->ymax;
/*   locsize = ts->size; */
	if (fsret == NULL)
	{
		return 0;
	}
	horiz = (xmax == xmin) ? 1.0e37f : (float)((xdots-10)/(xmax-xmin));
	vert  = (ymax == ymin) ? 1.0e37f : (float)((ydots-6) /(ymax-ymin));
	locsize = (vert < horiz) ? vert : horiz;

	ts->xpos = (horiz == 1.0e37f) ? xdots/2 : (xdots-locsize*(xmax + xmin))/2;
	ts->ypos = (vert  == 1.0e37f) ? ydots/2 : (ydots-locsize*(ymax + ymin))/2;
	ts->size = locsize;

	return 1;
}

struct lsys_cmd *_fastcall
drawLSysF(struct lsys_cmd *command, struct lsys_turtlestatef *ts, struct lsys_cmd **rules, int depth)
{
	struct lsys_cmd **rulind;
	int tran;

	if (overflow)     /* integer math routines overflowed */
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
					if (drawLSysF((*rulind) + 1, ts, rules, depth-1) == NULL)
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
				char saveang, saverev, savecolor;
				LDBL savesize, savex, savey, saverang;

				lsys_donefpu(ts);
				saveang = ts->angle;
				saverev = ts->reverse;
				savesize = ts->size;
				saverang = ts->realangle;
				savex = ts->xpos;
				savey = ts->ypos;
				savecolor = ts->curcolor;
				lsys_prepfpu(ts);
				command = drawLSysF(command + 1, ts, rules, depth);
				if (command == NULL)
					return NULL;
				lsys_donefpu(ts);
				ts->angle = saveang;
				ts->reverse = saverev;
				ts->size = savesize;
				ts->realangle = saverang;
				ts->xpos = savex;
				ts->ypos = savey;
				ts->curcolor = savecolor;
				lsys_prepfpu(ts);
			}
		}
		command++;
	}
	return command;
}

struct lsys_cmd *
LSysFSizeTransform(char *s, struct lsys_turtlestatef *ts)
{
	struct lsys_cmd *ret;
	struct lsys_cmd *doub;
	int max = 10;
	int n = 0;
	void (*f)();
	long num;
	int ptype;
	double PI180 = PI / 180.0;

	void (*plus)() = (ispow2(ts->maxangle)) ? lsysf_doplus_pow2 : lsysf_doplus;
	void (*minus)() = (ispow2(ts->maxangle)) ? lsysf_dominus_pow2 : lsysf_dominus;
	void (*pipe)() = (ispow2(ts->maxangle)) ? lsysf_dopipe_pow2 : lsysf_dopipe;

	void (*slash)() =  lsysf_doslash;
	void (*bslash)() = lsysf_dobslash;
	void (*at)() =     lsysf_doat;
	void (*dogf)() =   lsysf_dosizegf;

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
		case '/': f = slash;           ptype = 10;  ret[n].parm.nf = getnumber(&s)*PI180;  break;
		case '\\': f = bslash;         ptype = 10;  ret[n].parm.nf = getnumber(&s)*PI180;  break;
		case '@': f = at;              ptype = 10;  ret[n].parm.nf = getnumber(&s);  break;
		case '|': f = pipe;            break;
		case '!': f = lsysf_dobang;     break;
		case 'd':
		case 'm': f = lsysf_dosizedm;   break;
		case 'g':
		case 'f': f = dogf;       break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
			break;
		}
#if defined(XFRACT)
		ret[n].f = (void (*)())f;
#else
		ret[n].f = (void (*)(struct lsys_turtlestatef *))f;
#endif
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
LSysFDrawTransform(char *s, struct lsys_turtlestatef *ts)
{
	struct lsys_cmd *ret;
	struct lsys_cmd *doub;
	int max = 10;
	int n = 0;
	void (*f)();
	LDBL num;
	int ptype;
	LDBL PI180 = PI / 180.0;

	void (*plus)() = (ispow2(ts->maxangle)) ? lsysf_doplus_pow2 : lsysf_doplus;
	void (*minus)() = (ispow2(ts->maxangle)) ? lsysf_dominus_pow2 : lsysf_dominus;
	void (*pipe)() = (ispow2(ts->maxangle)) ? lsysf_dopipe_pow2 : lsysf_dopipe;

	void (*slash)() =  lsysf_doslash;
	void (*bslash)() = lsysf_dobslash;
	void (*at)() =     lsysf_doat;
	void (*drawg)() =  lsysf_dodrawg;

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
		case '/': f = slash;           ptype = 10;  ret[n].parm.nf = getnumber(&s)*PI180;  break;
		case '\\': f = bslash;         ptype = 10;  ret[n].parm.nf = getnumber(&s)*PI180;  break;
		case '@': f = at;              ptype = 10;  ret[n].parm.nf = getnumber(&s);  break;
		case '|': f = pipe;            break;
		case '!': f = lsysf_dobang;    break;
		case 'd': f = lsysf_dodrawd;   break;
		case 'm': f = lsysf_dodrawm;   break;
		case 'g': f = drawg;           break;
		case 'f': f = lsysf_dodrawf;   break;
		case 'c': f = lsysf_dodrawc;   num = getnumber(&s);    break;
		case '<': f = lsysf_dodrawlt;  num = getnumber(&s);    break;
		case '>': f = lsysf_dodrawgt;  num = getnumber(&s);    break;
		case '[': num = 1;        break;
		case ']': num = 2;        break;
		default:
			num = 3;
			break;
		}
#if defined(XFRACT)
		ret[n].f = (void (*)())f;
#else
		ret[n].f = (void (*)(struct lsys_turtlestatef *))f;
#endif
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

void _fastcall lsysf_dosincos(void)
{
	LDBL locaspect;
	LDBL TWOPI = 2.0*PI;
	LDBL twopimax;
	LDBL twopimaxi;
	int i;

	locaspect = screenaspect*xdots/ydots;
	twopimax = TWOPI / maxangle;
	for (i = 0; i < maxangle; i++)
	{
		twopimaxi = i*twopimax;
		sins_f[i] = sinl(twopimaxi);
		coss_f[i] = locaspect*cosl(twopimaxi);
	}
}
