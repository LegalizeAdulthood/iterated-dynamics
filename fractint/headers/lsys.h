/* lsys.h
 *      Header file for L-system code.
 *      Nicholas Wilt, 6/26/93.
 */

#ifndef LSYS_H
#define LSYS_H


#define size    ssize
/* Needed for use of asm -- helps decide which pointer to function
 * to put into the struct lsys_cmds.
 */

/* Macro to take an FP number and turn it into a
 * 16/16-bit fixed-point number.
 */
#define FIXEDMUL        524288L
#define FIXEDPT(x)      ((long) (FIXEDMUL * (x)))

/* The number by which to multiply sines, cosines and other
 * values with magnitudes less than or equal to 1.
 * sins and coss are a 3/29 bit fixed-point scheme (so the
 * range is +/- 2, with good accuracy.  The range is to
 * avoid overflowing when the aspect ratio is taken into
 * account.
 */
#define FIXEDLT1        536870912.0

#define ANGLE2DOUBLE    (2.0*MathUtil::Pi/4294967296.0)

#define MAXRULES 27 /* this limits rules to 25 */
#define MAX_LSYS_LINE_LEN 255 /* this limits line length to 255 */

template <typename T>
struct lsys_turtle_state_base
{
	char counter;
	char angle;
	char reverse;
	bool stackoflow;
    /* dmaxangle is max_angle - 1 */
	char max_angle;
	char dmaxangle;
	char curcolor;
	char dummy;  /* dummy ensures longword alignment */
    T size;
    T realangle;
	T xpos;
	T ypos; /* xpos and ypos are T, not fixed point */
	T x_min;
	T y_min;
	T x_max;
	T y_max; /* as are these */
    T aspect; /* aspect ratio of each pixel, ysize/xsize */
};

struct lsys_turtle_state_l : lsys_turtle_state_base<long>
{
    long num;
};

struct lsys_turtle_state_fp : lsys_turtle_state_base<LDBL>
{
    union
	{
        long n;
        LDBL nf;
    } parm;
};

extern int g_max_angle;

/* routines in lsysf.c */

struct lsys_cmd;
extern lsys_cmd * draw_lsysf(lsys_cmd *command, lsys_turtle_state_fp *ts, lsys_cmd **rules, int depth);
extern int lsysf_find_scale(lsys_cmd *command, lsys_turtle_state_fp *ts, lsys_cmd **rules, int depth);
extern lsys_cmd *lsysf_size_transform(char *s, lsys_turtle_state_fp *ts);
extern lsys_cmd *lsysf_draw_transform(char *s, lsys_turtle_state_fp *ts);
extern void lsysf_sin_cos();

extern LDBL  get_number(char **);
extern bool is_pow2(int);
extern int l_system();
extern int l_load();

#endif
