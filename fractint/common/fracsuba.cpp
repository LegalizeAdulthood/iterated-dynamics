#include "port.h"
#include "prototyp.h"
#include "fracsuba.h"

int bail_out_mod_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0 || labs(g_new_z_l.x) > g_limit2_l
		|| labs(g_new_z_l.y) > g_limit2_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_real_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_x_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_imag_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_y_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_or_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_x_l >= g_limit_l || g_temp_sqr_y_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_and_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if ((g_temp_sqr_x_l >= g_limit_l && g_temp_sqr_y_l >= g_limit_l) || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_manhattan_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_manhattan_r_l()
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude = fabs(g_new_z.x + g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}
