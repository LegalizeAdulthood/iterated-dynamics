#if !defined(WORK_LIST_H)
#define WORK_LIST_H

struct WorkListItem    /* work list entry for std escape time engines */
{
	int xx_start;    /* screen window for this entry */
	int xx_stop;
	int yy_start;
	int yy_stop;
	int yy_begin;    /* start row within window, for 2pass/ssg resume */
	int sym;        /* if symmetry in window, prevents bad combines */
	int pass;       /* for 2pass and solid guessing */
	int xx_begin;    /* start col within window, =0 except on resume */
};

typedef struct WorkListItem WORK_LIST;

enum
{
	MAX_WORK_LIST = 40
};

extern WORK_LIST g_work_list[MAX_WORK_LIST];
extern int g_xx_start;
extern int g_xx_stop;
extern int g_yy_start;
extern int g_yy_stop;

#endif
