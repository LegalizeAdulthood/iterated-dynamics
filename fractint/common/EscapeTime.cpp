#include "port.h"
#include "prototyp.h"
#include "fractint.h"
#include "fractals.h"

#include "EscapeTime.h"

EscapeTimeState g_escape_time_state;

EscapeTimeState::EscapeTimeState()
	: m_use_grid(false),
	m_grid_fp(),
	m_grid_l(),
	m_grid_bf()
{
}

EscapeTimeState::~EscapeTimeState()
{
}

void EscapeTimeState::free_grids()
{
	m_grid_bf.free_grid_pointers();
	m_grid_fp.free_grid_pointers();
	m_grid_l.free_grid_pointers();
}

void EscapeTimeState::set_grids()
{
	m_grid_fp.set_grid_pointers(g_x_dots, g_y_dots);
	m_grid_l.set_grid_pointers(g_x_dots, g_y_dots);
	set_pixel_calc_functions();
}

void EscapeTimeState::fill_grid_fp()
{
	if (m_use_grid)
	{
		m_grid_fp.fill();
	}
}

void EscapeTimeState::fill_grid_l()
{
	/* note that g_x1_l & g_y1_l values can overflow into sign bit; since     */
	/* they're used only to add to g_x0_l/g_y0_l, 2s comp straightens it out  */
	if (m_use_grid)
	{
		m_grid_l.fill();
	}
}
