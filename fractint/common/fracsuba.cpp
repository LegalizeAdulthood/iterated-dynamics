#include <string>

#include "port.h"
#include "prototyp.h"
#include "fracsuba.h"

int bail_out_mod_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l || g_magnitude_l < 0 || labs(g_new_z_l.real()) > g_rq_limit2_l
		|| labs(g_new_z_l.imag()) > g_rq_limit2_l || g_overflow)
	{
		g_overflow = false;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_real_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	if (g_temp_sqr_l.real() >= g_rq_limit_l || g_overflow)
	{
		g_overflow = false;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_imag_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	if (g_temp_sqr_l.imag() >= g_rq_limit_l || g_overflow)
	{
		g_overflow = false;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_or_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	if (g_temp_sqr_l.real() >= g_rq_limit_l || g_temp_sqr_l.imag() >= g_rq_limit_l || g_overflow)
	{
		g_overflow = false;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_and_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	if ((g_temp_sqr_l.real() >= g_rq_limit_l && g_temp_sqr_l.imag() >= g_rq_limit_l) || g_overflow)
	{
		g_overflow = false;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_manhattan_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude = std::abs(g_new_z.real()) + std::abs(g_new_z.imag());
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int bail_out_manhattan_r_l()
{
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude = std::abs(g_new_z.real() + g_new_z.imag());
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}
