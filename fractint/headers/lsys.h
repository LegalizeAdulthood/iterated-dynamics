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

struct lsys_turtlestatei
{
    char counter, angle, reverse, stackoflow;
    /* dmaxangle is maxangle - 1 */
    char maxangle, dmaxangle, curcolor, dummy;  /* dummy ensures longword alignment */
    long size;
    long realangle;
    long xpos, ypos; /* xpos and ypos are long, not fixed point */
    long xmin, ymin, xmax, ymax; /* as are these */
    long aspect; /* aspect ratio of each pixel, ysize/xsize */
    long num;
};

struct lsys_turtlestatef
{
    char counter, angle, reverse, stackoflow;
    /* dmaxangle is maxangle - 1 */
    char maxangle, dmaxangle, curcolor, dummy;  /* dummy ensures longword alignment */
    LDBL size;
    LDBL realangle;
    LDBL xpos, ypos;
    LDBL xmin, ymin, xmax, ymax;
    LDBL aspect; /* aspect ratio of each pixel, ysize/xsize */
    union
	{
        long n;
        LDBL nf;
    } parm;
};

extern char maxangle;

/* routines in lsysf.c */

extern struct lsys_cmd * _fastcall drawLSysF(struct lsys_cmd *command,struct lsys_turtlestatef *ts, struct lsys_cmd **rules,int depth);
extern int _fastcall lsysf_findscale(struct lsys_cmd *command, struct lsys_turtlestatef *ts, struct lsys_cmd **rules, int depth);
extern struct lsys_cmd *LSysFSizeTransform(char *s, struct lsys_turtlestatef *ts);
extern struct lsys_cmd *LSysFDrawTransform(char *s, struct lsys_turtlestatef *ts);
extern void _fastcall lsysf_dosincos(void);

#endif
