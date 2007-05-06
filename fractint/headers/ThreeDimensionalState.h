#if !defined(RAYTRACE_STATE_H)
#define RAYTRACE_STATE_H

#include "CommandParser.h"

/* ThreeDimensionalState::raytrace_output() values */
#define RAYTRACE_NONE		0
#define RAYTRACE_POVRAY		1
#define RAYTRACE_VIVID		2
#define RAYTRACE_RAW		3
#define RAYTRACE_MTV		4
#define RAYTRACE_RAYSHADE	5
#define RAYTRACE_ACROSPIN	6
#define RAYTRACE_DXF		7

/* g_glasses_type values */
#define STEREO_NONE			0
#define STEREO_ALTERNATE	1
#define STEREO_SUPERIMPOSE	2
#define STEREO_PHOTO		3
#define STEREO_PAIR			4

/* FILLTYPE values */
class FillType
{
public:
	enum
	{
		SurfaceGrid = -1,
		Points = 0,
		WireFrame = 1,
		Gouraud = 2,
		Flat = 3,
		Bars = 4,
		LightBefore = 5,
		LightAfter = 6
	};
};

class RedBlueState
{
public:
	RedBlueState(int left, int right, int bright)
		: m_crop_left(left),
		m_crop_right(right),
		m_bright(bright)
	{
	}
	int crop_left() const	{ return m_crop_left; }
	int crop_right() const	{ return m_crop_right; }
	int bright() const		{ return m_bright; }

	void set_crop_left(int value)	{ m_crop_left = value; }
	void set_crop_right(int value)	{ m_crop_right = value; }
	void set_bright(int value)		{ m_bright = value; }

private:
	int m_crop_left;
	int m_crop_right;
	int m_bright;
};

class ThreeDimensionalState
{
public:
	ThreeDimensionalState();
	~ThreeDimensionalState();

	int raytrace_output() const		{ return m_raytrace_output; }
	int raytrace_brief() const		{ return m_raytrace_brief; }
	int ambient() const				{ return m_ambient; }
	BYTE background_red() const		{ return m_background_color[0]; }
	BYTE background_green() const	{ return m_background_color[1]; }
	BYTE background_blue() const	{ return m_background_color[2]; }
	int haze() const				{ return m_haze; }
	int randomize_colors() const	{ return m_randomize_colors; }
	int x_trans() const				{ return m_x_trans; }
	int y_trans() const				{ return m_y_trans; }
	const char *ray_name() const	{ return m_ray_name; }

	bool sphere() const				{ return m_sphere; }
	int x_rotation() const;
	int y_rotation() const;
	int z_rotation() const;
	int x_scale() const;
	int y_scale() const;
	int phi1() const;
	int phi2() const;
	int theta1() const;
	int theta2() const;
	int radius() const;
	int roughness() const			{ return m_roughness; }
	int water_line() const			{ return m_water_line; }
	int fill_type() const			{ return m_fill_type; }
	int z_viewer() const			{ return m_z_viewer; }
	int x_shift() const				{ return m_x_shift; }
	int y_shift() const				{ return m_y_shift; }
	int x_light() const				{ return m_x_light; }
	int y_light() const				{ return m_y_light; }
	int z_light() const				{ return m_z_light; }
	int light_avg() const			{ return m_light_average; }

	bool preview() const			{ return m_preview; }
	bool show_box() const			{ return m_show_box; }
	int x_adjust() const			{ return m_x_adjust; }
	int y_adjust() const			{ return m_y_adjust; }
	int eye_separation() const		{ return m_eye_separation; }
	int glasses_type() const		{ return m_glasses_type; }
	int preview_factor() const		{ return m_preview_factor; }
	const RedBlueState &red() const	{ return m_red; }
	const RedBlueState &blue() const { return m_blue; }
	int transparent0() const		{ return m_transparent[0]; }
	int transparent1() const		{ return m_transparent[1]; }

	void get_raytrace_parameters(short *destination) const;
	void set_raytrace_parameters(const short *values);

	void set_raytrace_output(int value) { m_raytrace_output = value; }
	void set_raytrace_brief(bool value)	{ m_raytrace_brief = value; }
	void set_ambient(int value)			{ m_ambient = value; }
	void set_haze(int value)			{ m_haze = value; }
	void set_background_color(BYTE red, BYTE green, BYTE blue)
	{
		m_background_color[0] = red;
		m_background_color[1] = green;
		m_background_color[2] = blue;
	}
	void set_x_trans(int value)			{ m_x_trans = value; }
	void set_y_trans(int value)			{ m_y_trans = value; }
	void set_randomize_colors(int value) { m_randomize_colors = value; }
	void set_ray_name(const char *value);
	void set_sphere(bool value)			{ m_sphere = value; }
	void set_x_rotation(int value);
	void set_y_rotation(int value);
	void set_z_rotation(int value);
	void set_x_scale(int value);
	void set_y_scale(int value);
	void set_phi1(int value);
	void set_phi2(int value);
	void set_theta1(int value);
	void set_theta2(int value);
	void set_radius(int value);
	void set_roughness(int value)		{ m_roughness = value; }
	void set_water_line(int value)		{ m_water_line = value; }
	void set_fill_type(int value)		{ m_fill_type = value; }
	void set_z_viewer(int value)		{ m_z_viewer = value; }
	void set_x_shift(int value)			{ m_x_shift = value; }
	void set_y_shift(int value)			{ m_y_shift = value; }
	void set_x_light(int value)			{ m_x_light = value; }
	void set_y_light(int value)			{ m_y_light = value; }
	void set_z_light(int value)			{ m_z_light = value; }
	void set_light_average(int value)	{ m_light_average = value; }

	void set_preview(bool value)		{ m_preview = value; }
	void set_show_box(bool value)		{ m_show_box = value; }
	void set_x_adjust(int value)		{ m_x_adjust = value; }
	void set_y_adjust(int value)		{ m_y_adjust = value; }
	void set_eye_separation(int value)	{ m_eye_separation = value; }
	void set_glasses_type(int value)	{ m_glasses_type = value; }
	void set_preview_factor(int value)	{ m_preview_factor = value; }
	void set_transparent0(int value)	{ m_transparent[0] = value; }
	void set_transparent1(int value)	{ m_transparent[1] = value; }
	RedBlueState &set_red()				{ return m_red; }
	RedBlueState &set_blue()			{ return m_blue; }

	void set_defaults();

	int parse_sphere(const cmd_context &context);
	int parse_rotation(const cmd_context &context);
	int parse_perspective(const cmd_context &context);
	int parse_xy_shift(const cmd_context &context);
	int parse_xy_translate(const cmd_context &context);
	int parse_xyz_scale(const cmd_context &context);
	int parse_roughness(const cmd_context &context);
	int parse_water_line(const cmd_context &context);
	int parse_fill_type(const cmd_context &context);
	int parse_light_source(const cmd_context &context);
	int parse_smoothing(const cmd_context &context);
	int parse_lattitude(const cmd_context &context);
	int parse_longitude(const cmd_context &context);
	int parse_radius(const cmd_context &context);
	int parse_randomize_colors(const cmd_context &context);
	int parse_ambient(const cmd_context &context);
	int parse_haze(const cmd_context &context);
	int parse_background_color(const cmd_context &context);
	int parse_raytrace_output(const cmd_context &context);
	int parse_raytrace_brief(const cmd_context &context);
	int parse_preview(const cmd_context &context);
	int parse_show_box(const cmd_context &context);

	void next_ray_name();

	const char *parameter_text() const;

private:
	enum
	{
		DEFAULT_BACKGROUND_RED = 51,
		DEFAULT_BACKGROUND_GREEN = 153,
		DEFAULT_BACKGROUND_BLUE = 200,
		DEFAULT_AMBIENT = 20,
		DEFAULT_ROUGHNESS = 30,
		DEFAULT_X_ROTATION = 60,
		DEFAULT_Y_ROTATION = 30,
		DEFAULT_Z_ROTATION = 0,
		DEFAULT_X_SCALE = 90,
		DEFAULT_Y_SCALE = 90,
		DEFAULT_PLANAR_X_LIGHT = 1,
		DEFAULT_PLANAR_Y_LIGHT = -1,
		DEFAULT_PLANAR_Z_LIGHT = 1,
		DEFAULT_PHI1 = 180,
		DEFAULT_PHI2 = 0,
		DEFAULT_THETA1 = -90,
		DEFAULT_THETA2 = 90,
		DEFAULT_RADIUS = 100,
		DEFAULT_SPHERICAL_X_LIGHT = 1,
		DEFAULT_SPHERICAL_Y_LIGHT = 1,
		DEFAULT_SPHERICAL_Z_LIGHT = 1,
		DEFAULT_PREVIEW_FACTOR = 20,
		DEFAULT_RED_CROP_LEFT = 4,
		DEFAULT_RED_CROP_RIGHT = 0,
		DEFAULT_RED_BRIGHT = 80,
		DEFAULT_BLUE_CROP_LEFT = 0,
		DEFAULT_BLUE_CROP_RIGHT = 4,
		DEFAULT_BLUE_BRIGHT = 100
	};
	bool m_raytrace_brief;
	int	m_raytrace_output;
	int m_ambient;
	BYTE m_background_color[3];
	int m_haze;
	int m_randomize_colors;
	int m_x_trans;
	int m_y_trans;
	char m_ray_name[FILE_MAX_PATH];

	bool m_sphere;
	struct PlanarState
	{
		int m_x_rotation;
		int m_y_rotation;
		int m_z_rotation;
		int m_x_scale;
		int m_y_scale;
	};
	struct SphericalState
	{
		int m_phi1;
		int m_phi2;
		int m_theta1;
		int m_theta2;
		int m_radius;
	};
	PlanarState m_planar;
	SphericalState m_spherical;
	int m_roughness;
	int m_water_line;
	int m_fill_type;
	int m_z_viewer;
	int m_x_shift;
	int m_y_shift;
	int m_x_light;
	int m_y_light;
	int m_z_light;
	int m_light_average;

	bool m_preview;
	bool m_show_box;
	int m_x_adjust;
	int m_y_adjust;
	int m_eye_separation;
	int m_glasses_type;
	int m_preview_factor;
	RedBlueState m_red;
	RedBlueState m_blue;
	int	m_transparent[2];
};

extern ThreeDimensionalState g_3d_state;

#endif
