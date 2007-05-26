#include <cassert>
#include <string>
#include <sstream>
#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"

#include "cmdfiles.h"
#include "filesystem.h"

#include "ThreeDimensionalState.h"
#include "CommandParser.h"

ThreeDimensionalState g_3d_state;

ThreeDimensionalState::ThreeDimensionalState()
	: m_raytrace_brief(false),
	m_raytrace_output(RAYTRACE_NONE),
	m_ambient(0),
	m_haze(0),
	m_randomize_colors(false),
	m_x_trans(0),
	m_y_trans(0),
	m_sphere(false),
	m_planar(),
	m_spherical(),
	m_roughness(0),
	m_water_line(0),
	m_fill_type(FillType::Points),
	m_z_viewer(0),
	m_x_shift(0),
	m_y_shift(0),
	m_x_light(0),
	m_y_light(0),
	m_z_light(0),
	m_light_average(false),
	m_preview(false),
	m_show_box(false),
	m_x_adjust(0),
	m_y_adjust(0),
	m_eye_separation(0),
	m_glasses_type(STEREO_NONE),
	m_preview_factor(DEFAULT_PREVIEW_FACTOR),
	m_red(DEFAULT_RED_CROP_LEFT, DEFAULT_RED_CROP_RIGHT, DEFAULT_RED_BRIGHT),
	m_blue(DEFAULT_BLUE_CROP_LEFT, DEFAULT_BLUE_CROP_RIGHT, DEFAULT_BLUE_BRIGHT)
{
	m_background_color[0] = 0;
	m_background_color[1] = 0;
	m_background_color[2] = 0;
	::strcpy(m_ray_name, "fract001");
	m_transparent[0] = 0;
	m_transparent[1] = 0;
}

ThreeDimensionalState::~ThreeDimensionalState()
{
}

void ThreeDimensionalState::set_defaults()
{
	m_roughness = DEFAULT_ROUGHNESS;
	m_water_line = 0;
	m_z_viewer = 0;
	m_x_shift = 0;
	m_y_shift = 0;
	m_x_trans = 0;
	m_y_trans = 0;
	m_light_average = 0;
	m_ambient = DEFAULT_AMBIENT;
	m_randomize_colors = 0;
	m_haze = 0;
	m_background_color[0] = DEFAULT_BACKGROUND_RED;
	m_background_color[1] = DEFAULT_BACKGROUND_GREEN;
	m_background_color[2] = DEFAULT_BACKGROUND_BLUE;
	if (m_sphere)
	{
		m_spherical.m_phi1 = DEFAULT_PHI1;
		m_spherical.m_phi2 = DEFAULT_PHI2;
		m_spherical.m_theta1 = DEFAULT_THETA1;
		m_spherical.m_theta2 = DEFAULT_THETA2;
		m_spherical.m_radius = DEFAULT_RADIUS;
		m_fill_type = FillType::Gouraud;
		m_x_light = DEFAULT_SPHERICAL_X_LIGHT;
		m_y_light = DEFAULT_SPHERICAL_Y_LIGHT;
		m_z_light = DEFAULT_SPHERICAL_Z_LIGHT;
	}
	else
	{
		m_planar.m_x_rotation = DEFAULT_X_ROTATION;
		m_planar.m_y_rotation = DEFAULT_Y_ROTATION;
		m_planar.m_z_rotation = DEFAULT_Z_ROTATION;
		m_planar.m_x_scale = DEFAULT_X_SCALE;
		m_planar.m_y_scale = DEFAULT_Y_SCALE;
		m_fill_type = FillType::Points;
		m_x_light = DEFAULT_PLANAR_X_LIGHT;
		m_y_light = DEFAULT_PLANAR_Y_LIGHT;
		m_z_light = DEFAULT_PLANAR_Z_LIGHT;
	}
}

void ThreeDimensionalState::get_raytrace_parameters(short *destination) const
{
	*destination++ = m_sphere ? 1 : 0;
	if (m_sphere)
	{
		*destination++ = m_spherical.m_phi1;
		*destination++ = m_spherical.m_phi2;
		*destination++ = m_spherical.m_theta1;
		*destination++ = m_spherical.m_theta2;
		*destination++ = m_spherical.m_radius;
	}
	else
	{
		*destination++ = m_planar.m_x_rotation;
		*destination++ = m_planar.m_y_rotation;
		*destination++ = m_planar.m_z_rotation;
		*destination++ = m_planar.m_x_scale;
		*destination++ = m_planar.m_y_scale;
	}
	*destination++ = m_roughness;
	*destination++ = m_water_line;
	*destination++ = m_fill_type;
	*destination++ = m_z_viewer;
	*destination++ = m_x_shift;
	*destination++ = m_y_shift;
	*destination++ = m_x_light;
	*destination++ = m_y_light;
	*destination++ = m_z_light;
	*destination++ = m_light_average;
}

void ThreeDimensionalState::set_raytrace_parameters(const short *values)
{
	m_sphere = (*values++ != 0);
	if (m_sphere)
	{
		m_spherical.m_phi1 = *values++;
		m_spherical.m_phi2 = *values++;
		m_spherical.m_theta1 = *values++;
		m_spherical.m_theta2 = *values++;
		m_spherical.m_radius = *values++;
	}
	else
	{
		m_planar.m_x_rotation = *values++;
		m_planar.m_y_rotation = *values++;
		m_planar.m_z_rotation = *values++;
		m_planar.m_x_scale = *values++;
		m_planar.m_y_scale = *values++;
	}
	m_roughness = *values++;
	m_water_line = *values++;
	m_fill_type = *values++;
	m_z_viewer = *values++;
	m_x_shift = *values++;
	m_y_shift = *values++;
	m_x_light = *values++;
	m_y_light = *values++;
	m_z_light = *values++;
	m_light_average = *values++;
}
int ThreeDimensionalState::parse_sphere(const cmd_context &context)
{
	return FlagParser<bool>(m_sphere, Command::ThreeDParameter).parse(context);
}

int ThreeDimensionalState::parse_rotation(const cmd_context &context)
{
	if (context.totparms != 3 || context.intparms != 3)
	{
		return bad_arg(context.curarg);
	}
	m_planar.m_x_rotation = context.intval[0];
	m_planar.m_y_rotation = context.intval[1];
	m_planar.m_z_rotation = context.intval[2];
	return Command::FractalParameter | Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_perspective(const cmd_context &context)
{
	if (context.numval == NON_NUMERIC)
	{
		return bad_arg(context.curarg);
	}
	m_z_viewer = context.numval;
	return Command::FractalParameter | Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_xy_shift(const cmd_context &context)
{
	if (context.totparms != 2 || context.intparms != 2)
	{
		return bad_arg(context.curarg);
	}
	m_x_shift = context.intval[0];
	m_y_shift = context.intval[1];
	return Command::FractalParameter | Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_xy_translate(const cmd_context &context)
{
	if (context.totparms != 2 || context.intparms != 2)
	{
		return bad_arg(context.curarg);
	}
	m_x_trans = context.intval[0];
	m_y_trans = context.intval[1];
	return Command::FractalParameter | Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_xyz_scale(const cmd_context &context)
{
	if (context.totparms < 2 || context.intparms != context.totparms)
	{
		return bad_arg(context.curarg);
	}
	m_planar.m_x_scale = context.intval[0];
	m_planar.m_y_scale = context.intval[1];
	if (context.totparms > 2)
	{
		m_roughness = context.intval[2];
	}
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_roughness(const cmd_context &context)
{
	/* "rough" is really scale z, but we add it here for convenience */
	m_roughness = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_water_line(const cmd_context &context)
{
	if (context.numval < 0)
	{
		return bad_arg(context.curarg);
	}
	m_water_line = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_fill_type(const cmd_context &context)
{
	if (context.numval < FillType::SurfaceGrid || context.numval > FillType::LightAfter)
	{
		return bad_arg(context.curarg);
	}
	m_fill_type = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_light_source(const cmd_context &context)
{
	if (context.totparms != 3 || context.intparms != 3)
	{
		return bad_arg(context.curarg);
	}
	m_x_light = context.intval[0];
	m_y_light = context.intval[1];
	m_z_light = context.intval[2];
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_smoothing(const cmd_context &context)
{
	if (context.numval < 0)
	{
		return bad_arg(context.curarg);
	}
	m_light_average = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_lattitude(const cmd_context &context)
{
	if (context.totparms != 2 || context.intparms != 2)
	{
		return bad_arg(context.curarg);
	}
	m_spherical.m_theta1 = context.intval[0];
	m_spherical.m_theta2 = context.intval[1];
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_longitude(const cmd_context &context)
{
	if (context.totparms != 2 || context.intparms != 2)
	{
		return bad_arg(context.curarg);
	}
	m_spherical.m_phi1 = context.intval[0];
	m_spherical.m_phi2 = context.intval[1];
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_radius(const cmd_context &context)
{
	if (context.numval < 0)
	{
		return bad_arg(context.curarg);
	}
	m_spherical.m_radius = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_randomize_colors(const cmd_context &context)
{
	if (context.numval < 0 || context.numval > 7)
	{
		return bad_arg(context.curarg);
	}
	m_randomize_colors = (context.numval != 0);
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_ambient(const cmd_context &context)
{
	if (context.numval < 0 || context.numval > 100)
	{
		return bad_arg(context.curarg);
	}
	m_ambient = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_haze(const cmd_context &context)
{
	if (context.numval < 0 || context.numval > 100)
	{
		return bad_arg(context.curarg);
	}
	m_haze = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_background_color(const cmd_context &context)
{
	int i;

	if (context.totparms != 3 || context.intparms != 3)
	{
		return bad_arg(context.curarg);
	}
	for (i = 0; i < 3; i++)
	{
		if (context.intval[i] & ~0xff)
		{
			return bad_arg(context.curarg);
		}
	}
	m_background_color[0] = (BYTE)context.intval[0];
	m_background_color[1] = (BYTE)context.intval[1];
	m_background_color[2] = (BYTE)context.intval[2];
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_raytrace_output(const cmd_context &context)
{
	if (context.numval < 0 || context.numval > 6)
	{
		return bad_arg(context.curarg);
	}
	m_raytrace_output = context.numval;
	return Command::ThreeDParameter;
}

int ThreeDimensionalState::parse_raytrace_brief(const cmd_context &context)
{
	return FlagParser<bool>(m_raytrace_brief, Command::ThreeDParameter).parse(context);
}

int ThreeDimensionalState::parse_preview(const cmd_context &context)
{
	return FlagParser<bool>(m_preview, Command::ThreeDParameter).parse(context);
}

int ThreeDimensionalState::parse_show_box(const cmd_context &context)
{
	return FlagParser<bool>(m_show_box, Command::ThreeDParameter).parse(context);
}

void ThreeDimensionalState::set_ray_name(const char *value)
{
	::strcpy(m_ray_name, value);
}

void ThreeDimensionalState::next_ray_name()
{
	check_write_file(m_ray_name, ".ray");
}

const char *ThreeDimensionalState::parameter_text() const
{
	std::ostringstream text;

	if (sphere())
	{
		text << " sphere=y";
		text << " latitude=" << theta1() << "/" << theta2();
		text << " longitude=" << phi1() << "/" << phi2();
		text << " radius=" << radius();
	}
	text << " scalexyz=" << x_scale() << "/" << y_scale();
	text << " roughness=" << roughness();
	text << " waterline=" << water_line();
	if (fill_type())
	{
		text << " filltype=" << fill_type();
	}

	if (m_raytrace_output)
	{
		text << " ray=" << m_raytrace_output;
		if (m_raytrace_brief)
		{
			text << " brief=y";
		}
	}
	if (fill_type() > FillType::Bars)
	{
		text << " lightsource=" << x_light() << "/" << y_light() << "/" << z_light();
		if (light_avg())
		{
			text << " smoothing=" << light_avg();
		}
	}
	if (m_randomize_colors)
	{
		text << " randomize=" << m_randomize_colors;
	}
	if (ambient())
	{
		text << " ambient=" << ambient();
	}
	if (haze())
	{
		text << " haze=" << haze();
	}
	if (m_background_color[0] != DEFAULT_BACKGROUND_RED
		|| m_background_color[1] != DEFAULT_BACKGROUND_GREEN
		|| m_background_color[2] != DEFAULT_BACKGROUND_BLUE)
	{
		text << " background=" << m_background_color[0]
			<< "/" << m_background_color[1]
			<< "/" << m_background_color[2];
	}

	text << std::ends;
	return text.str().c_str();
}

int ThreeDimensionalState::x_rotation() const
{
	assert(!m_sphere);
	return m_planar.m_x_rotation;
}

int ThreeDimensionalState::y_rotation() const
{
	assert(!m_sphere);
	return m_planar.m_y_rotation;
}

int ThreeDimensionalState::z_rotation() const
{
	assert(!m_sphere);
	return m_planar.m_z_rotation;
}

int ThreeDimensionalState::x_scale() const
{
	assert(!m_sphere);
	return m_planar.m_x_scale;
}

int ThreeDimensionalState::y_scale() const
{
	assert(!m_sphere);
	return m_planar.m_y_scale;
}

int ThreeDimensionalState::phi1() const
{
	assert(m_sphere);
	return m_spherical.m_phi1;
}

int ThreeDimensionalState::phi2() const
{
	assert(m_sphere);
	return m_spherical.m_phi2;
}

int ThreeDimensionalState::theta1() const
{
	assert(m_sphere);
	return m_spherical.m_theta1;
}

int ThreeDimensionalState::theta2() const
{
	assert(m_sphere);
	return m_spherical.m_theta2;
}

int ThreeDimensionalState::radius() const
{
	assert(m_sphere);
	return m_spherical.m_radius;
}

void ThreeDimensionalState::set_x_rotation(int value)
{
	assert(!m_sphere);
	m_planar.m_x_rotation = value;
}

void ThreeDimensionalState::set_y_rotation(int value)
{
	assert(!m_sphere);
	m_planar.m_y_rotation = value;
}

void ThreeDimensionalState::set_z_rotation(int value)
{
	assert(!m_sphere);
	m_planar.m_z_rotation = value;
}

void ThreeDimensionalState::set_x_scale(int value)
{
	assert(!m_sphere);
	m_planar.m_x_scale = value;
}

void ThreeDimensionalState::set_y_scale(int value)
{
	assert(!m_sphere);
	m_planar.m_y_scale = value;
}

void ThreeDimensionalState::set_phi1(int value)
{
	assert(m_sphere);
	m_spherical.m_phi1 = value;
}

void ThreeDimensionalState::set_phi2(int value)
{
	assert(m_sphere);
	m_spherical.m_phi2 = value;
}

void ThreeDimensionalState::set_theta1(int value)
{
	assert(m_sphere);
	m_spherical.m_theta1 = value;
}

void ThreeDimensionalState::set_theta2(int value)
{
	assert(m_sphere);
	m_spherical.m_theta2 = value;
}

void ThreeDimensionalState::set_radius(int value)
{
	assert(m_sphere);
	m_spherical.m_radius = value;
}
