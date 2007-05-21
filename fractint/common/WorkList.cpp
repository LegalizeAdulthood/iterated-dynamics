#include <cassert>
#include <string.h>

#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "externs.h"
#include "fractype.h"
#include "prototyp.h"

#include "big.h"
#include "calcfrac.h"
#include "diskvid.h"
#include "fracsubr.h"

#include "EscapeTime.h"
#include "MathUtil.h"
#include "WorkList.h"

WorkList g_WorkList;

WorkList::WorkList()
	:
	m_num_items(0),
	m_xx_start(0),
	m_xx_stop(0),
	m_yy_start(0),
	m_yy_stop(0),
	m_xx_begin(0),
	m_yy_begin(0),
	m_work_pass(0),
	m_ix_start(0),
	m_iy_start(0),
	m_work_sym(0)
{
	WorkListItem zero = { 0 };
	for (int i = 0; i < NUM_OF(m_items); i++)
	{
		m_items[i] = zero;
	}
}

WorkList::~WorkList()
{
}

void WorkList::perform()
{
}

void WorkList::setup_initial_work_list()
{
	/* default setup a new m_items */
	m_num_items = 1;
	m_items[0].xx_begin = 0;
	m_items[0].xx_start = 0;
	m_items[0].yy_start = 0;
	m_items[0].yy_begin = 0;
	m_items[0].xx_stop = g_x_dots - 1;
	m_items[0].yy_stop = g_y_dots - 1;
	m_items[0].pass = 0;
	m_items[0].sym = 0;
	if (g_resuming) /* restore m_items, if we can't the above will stay in place */
	{
		int vsn = start_resume();
		get_resume();
		end_resume();
		if (vsn < 2)
		{
			m_xx_begin = 0;
		}
	}
}

void WorkList::get_top_item()
{
	/* pull top entry off m_items */
	m_xx_start = m_items[0].xx_start;
	m_xx_stop = m_items[0].xx_stop;
	g_ix_start = m_xx_start;
	g_x_stop  = m_xx_stop;
	m_xx_begin  = m_items[0].xx_begin;
	m_yy_start = m_items[0].yy_start;
	m_yy_stop = m_items[0].yy_stop;
	g_iy_start = m_yy_start;
	g_y_stop  = m_yy_stop;
	m_yy_begin  = m_items[0].yy_begin;
	g_work_pass = m_items[0].pass;
	g_work_sym  = m_items[0].sym;
	--m_num_items;
	for (int i = 0; i < m_num_items; ++i)
	{
		m_items[i] = m_items[i + 1];
	}
}

void WorkList::get_resume()
{
	::get_resume(sizeof(m_num_items), &m_num_items, sizeof(m_items), m_items, 0);
}

void WorkList::put_resume()
{
	/* interrupted, resumable */
	alloc_resume(sizeof(m_items) + 20, 2);
	::put_resume(sizeof(m_num_items), &m_num_items, sizeof(m_items), m_items, 0);
}

int WorkList::get_lowest_pass() const
{
	assert(m_num_items > 0);
	int lowest = m_items[0].pass;
	for (int j = 1; j < m_num_items; ++j) /* find lowest pass in any pending window */
	{
		if (m_items[j].pass < lowest)
		{
			lowest = m_items[j].pass;
		}
	}
	return lowest;
}

void WorkList::offset_items(int row, int col)
{
	/* adjust existing m_items entries */
	for (int i = 0; i < m_num_items; ++i)
	{
		m_items[i].yy_start -= row;
		m_items[i].yy_stop  -= row;
		m_items[i].yy_begin -= row;
		m_items[i].xx_start -= col;
		m_items[i].xx_stop  -= col;
		m_items[i].xx_begin -= col;
	}
}

static int clamp_zero(int value)
{
	return MathUtil::Clamp(value, 0, value);
}
static int clamp_greater(int value, int greater)
{
	return MathUtil::Clamp(value, value, greater);
}

/* force an item to restart */
void WorkList::restart_item(int wknum)
{
	int yfrom = clamp_zero(m_items[wknum].yy_start);
	int xfrom = clamp_zero(m_items[wknum].xx_start);
	int yto = clamp_greater(m_items[wknum].yy_stop, g_y_dots - 1);
	int xto = clamp_greater(m_items[wknum].xx_stop, g_x_dots - 1);
	memset(g_stack, 0, g_x_dots); /* use g_stack as a temp for the row; clear it */
	while (yfrom <= yto)
	{
		put_line(yfrom++, xfrom, xto, (BYTE *) g_stack);
	}

	m_items[wknum].sym = 0;
	m_items[wknum].pass = 0;
	m_items[wknum].yy_begin = m_items[wknum].yy_start;
	m_items[wknum].xx_begin = m_items[wknum].xx_start;
}

void WorkList::fix()
{
	int i, j, k;
	WorkListItem *wk;
	for (i = 0; i < m_num_items; ++i)
	{
		wk = &m_items[i];
		if (wk->yy_start >= g_y_dots || wk->yy_stop < 0
			|| wk->xx_start >= g_x_dots || wk->xx_stop < 0)  /* offscreen, delete */
		{
			for (j = i + 1; j < m_num_items; ++j)
			{
				m_items[j-1] = m_items[j];
			}
			--m_num_items;
			--i;
			continue;
		}
		if (wk->yy_start < 0)  /* partly off top edge */
		{
			if ((wk->sym&1) == 0)  /* no sym, easy */
			{
				wk->yy_start = 0;
				wk->xx_begin = 0;
			}
			else  /* xaxis symmetry */
			{
				j = wk->yy_stop + wk->yy_start;
				if (j > 0 && m_num_items < MAX_WORK_LIST)  /* split the sym part */
				{
					m_items[m_num_items] = m_items[i];
					m_items[m_num_items].yy_start = 0;
					m_items[m_num_items++].yy_stop = j;
					wk->yy_start = j + 1;
				}
				else
				{
					wk->yy_start = 0;
				}
				restart_item(i); /* restart the no-longer sym part */
			}
		}
		if (wk->yy_stop >= g_y_dots)  /* partly off bottom edge */
		{
			j = g_y_dots-1;
			if ((wk->sym&1) != 0)  /* uses xaxis symmetry */
			{
				k = wk->yy_start + (wk->yy_stop - j);
				if (k < j)
				{
					if (m_num_items >= MAX_WORK_LIST) /* no room to split */
					{
						restart_item(i);
					}
					else  /* split it */
					{
						m_items[m_num_items] = m_items[i];
						m_items[m_num_items].yy_start = k;
						m_items[m_num_items++].yy_stop = j;
						j = k-1;
					}
				}
				wk->sym &= -1 - 1;
			}
			wk->yy_stop = j;
		}
		if (wk->xx_start < 0)  /* partly off left edge */
		{
			if ((wk->sym&2) == 0) /* no sym, easy */
				wk->xx_start = 0;
			else  /* yaxis symmetry */
			{
				j = wk->xx_stop + wk->xx_start;
				if (j > 0 && m_num_items < MAX_WORK_LIST)  /* split the sym part */
				{
					m_items[m_num_items] = m_items[i];
					m_items[m_num_items].xx_start = 0;
					m_items[m_num_items++].xx_stop = j;
					wk->xx_start = j + 1;
				}
				else
				{
					wk->xx_start = 0;
				}
				restart_item(i); /* restart the no-longer sym part */
			}
		}
		if (wk->xx_stop >= g_x_dots)  /* partly off right edge */
		{
			j = g_x_dots-1;
			if ((wk->sym&2) != 0)  /* uses xaxis symmetry */
			{
				k = wk->xx_start + (wk->xx_stop - j);
				if (k < j)
				{
					if (m_num_items >= MAX_WORK_LIST) /* no room to split */
					{
						restart_item(i);
					}
					else  /* split it */
					{
						m_items[m_num_items] = m_items[i];
						m_items[m_num_items].xx_start = k;
						m_items[m_num_items++].xx_stop = j;
						j = k-1;
					}
				}
				wk->sym &= -1 - 2;
			}
			wk->xx_stop = j;
		}
		if (wk->yy_begin < wk->yy_start)
		{
			wk->yy_begin = wk->yy_start;
		}
		if (wk->yy_begin > wk->yy_stop)
		{
			wk->yy_begin = wk->yy_stop;
		}
		if (wk->xx_begin < wk->xx_start)
		{
			wk->xx_begin = wk->xx_start;
		}
		if (wk->xx_begin > wk->xx_stop)
		{
			wk->xx_begin = wk->xx_stop;
		}
	}
	tidy(); /* combine where possible, re-sort */
}

void WorkList::tidy()
{
	int i;
	int j;
	WorkListItem tempwork;
	while ((i = combine()) != 0)
	{
		/* merged two, delete the gone one */
		while (++i < m_num_items)
		{
			m_items[i-1] = m_items[i];
		}
		--m_num_items;
	}
	for (i = 0; i < m_num_items; ++i)
	{
		for (j = i + 1; j < m_num_items; ++j)
		{
			if (m_items[j].pass < m_items[i].pass
				|| (m_items[j].pass == m_items[i].pass
				&& (m_items[j].yy_start < m_items[i].yy_start
				|| (m_items[j].yy_start == m_items[i].yy_start
				&& m_items[j].xx_start <  m_items[i].xx_start))))
			{ /* dumb sort, swap 2 entries to correct order */
				tempwork = m_items[i];
				m_items[i] = m_items[j];
				m_items[j] = tempwork;
			}
		}
	}
}

int WorkList::combine()
{
	int i;
	int j;
	for (i = 0; i < m_num_items; ++i)
	{
		if (m_items[i].yy_start == m_items[i].yy_begin)
		{
			for (j = i + 1; j < m_num_items; ++j)
			{
				if (m_items[j].sym == m_items[i].sym
					&& m_items[j].yy_start == m_items[j].yy_begin
					&& m_items[j].xx_start == m_items[j].xx_begin
					&& m_items[i].pass == m_items[j].pass)
				{
					if (m_items[i].xx_start == m_items[j].xx_start
						&& m_items[i].xx_begin == m_items[j].xx_begin
						&& m_items[i].xx_stop  == m_items[j].xx_stop)
					{
						if (m_items[i].yy_stop + 1 == m_items[j].yy_start)
						{
							m_items[i].yy_stop = m_items[j].yy_stop;
							return j;
						}
						if (m_items[j].yy_stop + 1 == m_items[i].yy_start)
						{
							m_items[i].yy_start = m_items[j].yy_start;
							m_items[i].yy_begin = m_items[j].yy_begin;
							return j;
						}
					}
					if (m_items[i].yy_start == m_items[j].yy_start
						&& m_items[i].yy_begin == m_items[j].yy_begin
						&& m_items[i].yy_stop  == m_items[j].yy_stop)
					{
						if (m_items[i].xx_stop + 1 == m_items[j].xx_start)
						{
							m_items[i].xx_stop = m_items[j].xx_stop;
							return j;
						}
						if (m_items[j].xx_stop + 1 == m_items[i].xx_start)
						{
							m_items[i].xx_start = m_items[j].xx_start;
							m_items[i].xx_begin = m_items[j].xx_begin;
							return j;
						}
					}
				}
			}
		}
	}
	return 0; /* nothing combined */
}

int WorkList::add(int xfrom, int xto, int xbegin,
	int yfrom, int yto, int ybegin,
	int pass, int sym)
{
	if (m_num_items >= MAX_WORK_LIST)
	{
		return -1;
	}
	m_items[m_num_items].xx_start = xfrom;
	m_items[m_num_items].xx_stop  = xto;
	m_items[m_num_items].xx_begin = xbegin;
	m_items[m_num_items].yy_start = yfrom;
	m_items[m_num_items].yy_stop  = yto;
	m_items[m_num_items].yy_begin = ybegin;
	m_items[m_num_items].pass    = pass;
	m_items[m_num_items].sym     = sym;
	++m_num_items;
	tidy();
	return 0;
}
