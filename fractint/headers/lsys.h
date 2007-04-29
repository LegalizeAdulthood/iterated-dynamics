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

#define ANGLE2DOUBLE    (2.0*PI / 4294967296.0)

#define MAXRULES 27 /* this limits rules to 25 */
#define MAX_LSYS_LINE_LEN 255 /* this limits line length to 255 */

struct lsys_turtle_state
{
    char counter, angle, reverse, stackoflow;
    /* dmaxangle is max_angle - 1 */
    char max_angle, dmaxangle, curcolor, dummy;  /* dummy ensures longword alignment */
    long size;
    long realangle;
    long xpos, ypos; /* xpos and ypos are long, not fixed point */
    long x_min, y_min, x_max, y_max; /* as are these */
    long aspect; /* aspect ratio of each pixel, ysize/xsize */
    long num;
};

struct lsys_turtle_state_fp
{
    char counter, angle, reverse, stackoflow;
    /* dmaxangle is max_angle - 1 */
    char max_angle, dmaxangle, curcolor, dummy;  /* dummy ensures longword alignment */
    LDBL size;
    LDBL realangle;
    LDBL xpos, ypos;
    LDBL x_min, y_min, x_max, y_max;
    LDBL aspect; /* aspect ratio of each pixel, ysize/xsize */
    union
	{
        long n;
        LDBL nf;
    } parm;
};

extern int g_max_angle;

/* routines in lsysf.c */

extern struct lsys_cmd * _fastcall draw_lsysf(struct lsys_cmd *command,struct lsys_turtle_state_fp *ts, struct lsys_cmd **rules,int depth);
extern int _fastcall lsysf_find_scale(struct lsys_cmd *command, struct lsys_turtle_state_fp *ts, struct lsys_cmd **rules, int depth);
extern struct lsys_cmd *lsysf_size_transform(char *s, struct lsys_turtle_state_fp *ts);
extern struct lsys_cmd *lsysf_draw_transform(char *s, struct lsys_turtle_state_fp *ts);
extern void _fastcall lsysf_sin_cos();

#endif
