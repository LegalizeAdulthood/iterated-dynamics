#pragma once

#define MAX_CALC_WORK 12

struct WORKLIST     // work list entry for std escape time engines
{
    int xxstart;    // screen window for this entry
    int xxstop;
    int yystart;
    int yystop;
    int yybegin;    // start row within window, for 2pass/ssg resume
    int sym;        // if symmetry in window, prevents bad combines
    int pass;       // for 2pass and solid guessing
    int xxbegin;    // start col within window, =0 except on resume
};

extern int                   g_num_work_list;
extern WORKLIST              g_work_list[MAX_CALC_WORK];

int add_worklist(int xfrom, int xto, int xbegin,
                 int yfrom, int yto, int ybegin,
                 int pass, int sym);
void tidy_worklist();
