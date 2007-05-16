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
	MAX_WORK_LIST = 40,
	MAX_Y_BLOCK = 7,		/* MAX_X_BLOCK*MAX_Y_BLOCK*2 <= 4096, the size of "prefix" */
	MAX_X_BLOCK = 202		/* each maxnblk is oversize by 2 for a "border" */
							/* MAX_X_BLOCK defn must match fracsubr.c */
};

extern WORK_LIST g_work_list[MAX_WORK_LIST];
extern int g_xx_start;
extern int g_xx_stop;
extern int g_yy_start;
extern int g_yy_stop;
extern int g_xx_begin;
extern int g_yy_begin;
extern int g_work_pass;
extern int g_ix_start;
extern int g_iy_start;
extern int g_work_sym;

extern void sym_fill_line(int row, int left, int right, BYTE *str);

#endif
