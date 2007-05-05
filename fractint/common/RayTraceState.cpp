#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"
#include "RayTraceState.h"
#include "CommandParser.h"

RayTraceState g_raytrace_state;

RayTraceState::RayTraceState()
	: m_ambient(0),
	m_haze(0),
	m_randomize_colors(0),
	m_x_trans(0),
	m_y_trans(0)
{
	m_background_color[0] = 0;
	m_background_color[1] = 0;
	m_background_color[2] = 0;
	for (int i = 0; i < NUM_OF(m_init_3d); i++)
	{
		m_init_3d[i] = 0;
	}
	::strcpy(m_ray_name, "fract001");
}

RayTraceState::~RayTraceState()
{
}

void RayTraceState::set_defaults()
{
	m_init_3d[ROUGH] = 30;
	m_init_3d[WATERLINE] = 0;
	m_init_3d[ZVIEWER] = 0;
	m_init_3d[XSHIFT] = 0;
	m_init_3d[YSHIFT] = 0;
	m_x_trans = 0;
	m_y_trans = 0;
	m_init_3d[LIGHTAVG] = 0;
	m_ambient = 20;
	m_randomize_colors = 0;
	m_haze = 0;
	m_background_color[0] = 51;
	m_background_color[1] = 153;
	m_background_color[2] = 200;
	if (m_init_3d[SPHERE])
	{
		m_init_3d[PHI1] =  180;
		m_init_3d[PHI2] =  0;
		m_init_3d[THETA1] =  -90;
		m_init_3d[THETA2] =  90;
		m_init_3d[RADIUS] =  100;
		m_init_3d[FILLTYPE] = FillType::Gouraud;
		m_init_3d[XLIGHT] = 1;
		m_init_3d[YLIGHT] = 1;
		m_init_3d[ZLIGHT] = 1;
	}
	else
	{
		m_init_3d[XROT] = 60;
		m_init_3d[YROT] = 30;
		m_init_3d[ZROT] = 0;
		m_init_3d[XSCALE] = 90;
		m_init_3d[YSCALE] = 90;
		m_init_3d[FILLTYPE] = FillType::Points;
		m_init_3d[XLIGHT] = 1;
		m_init_3d[YLIGHT] = -1;
		m_init_3d[ZLIGHT] = 1;
	}
}

void RayTraceState::set_init_3d(const short *data, int count)
{
	for (int i = 0; i < count; i++)
	{
		m_init_3d[i] = data[i];
	}
}

void RayTraceState::get_init_3d(short *data, int count) const
{
	for (int i = 0; i < count; i++)
	{
		data[i] = m_init_3d[i];
	}
}

void RayTraceState::history_save(short *data) const
{
	data[0]		= (short) m_init_3d[0];
	data[1]		= (short) m_init_3d[1];
	data[2]		= (short) m_init_3d[2];
	data[3]		= (short) m_init_3d[3];
	data[4]		= (short) m_init_3d[4];
	data[5]		= (short) m_init_3d[5];
	data[6]		= (short) m_init_3d[6];
	data[7]		= (short) m_init_3d[7];
	data[8]		= (short) m_init_3d[8];
	data[9]		= (short) m_init_3d[9];
	data[10]	= (short) m_init_3d[10];
	data[11]	= (short) m_init_3d[12];
	data[12]	= (short) m_init_3d[13];
	data[13]	= (short) m_init_3d[14];
	data[14]	= (short) m_init_3d[15];
	data[15]	= (short) m_init_3d[16];
}

void RayTraceState::history_restore(const short *data)
{
	m_init_3d[0]           	= data[0];
	m_init_3d[1]           	= data[1];
	m_init_3d[2]           	= data[2];
	m_init_3d[3]           	= data[3];
	m_init_3d[4]           	= data[4];
	m_init_3d[5]           	= data[5];
	m_init_3d[6]           	= data[6];
	m_init_3d[7]           	= data[7];
	m_init_3d[8]           	= data[8];
	m_init_3d[9]           	= data[9];
	m_init_3d[10]          	= data[10];
	m_init_3d[12]          	= data[11];
	m_init_3d[13]          	= data[12];
	m_init_3d[14]          	= data[13];
	m_init_3d[15]          	= data[14];
	m_init_3d[16]          	= data[15];
}

int RayTraceState::parse_sphere(const cmd_context &context)
{
	return FlagParser<int>(m_init_3d[0], Command::ThreeDParameter).parse(context);
}

