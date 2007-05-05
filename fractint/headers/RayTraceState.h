#if !defined(RAYTRACE_STATE_H)
#define RAYTRACE_STATE_H

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

class RayTraceState
{
public:
	RayTraceState();
	~RayTraceState();

	bool sphere() const			{ return (m_init_3d[SPHERE] != 0); }
	int x_rot() const			{ return m_init_3d[XROT]; }
	int y_rot() const			{ return m_init_3d[YROT]; }
	int z_rot() const			{ return m_init_3d[ZROT]; }
	int x_scale() const			{ return m_init_3d[XSCALE]; }
	int y_scale() const			{ return m_init_3d[YSCALE]; }
	int phi1() const			{ return m_init_3d[PHI1]; }
	int phi2() const			{ return m_init_3d[PHI2]; }
	int theta1() const			{ return m_init_3d[THETA1]; }
	int theta2() const			{ return m_init_3d[THETA2]; }
	int radius() const			{ return m_init_3d[RADIUS]; }
	int rough() const			{ return m_init_3d[ROUGH]; }
	int water_line() const		{ return m_init_3d[WATERLINE]; }
	int fill_type() const		{ return m_init_3d[FILLTYPE]; }
	int z_viewer() const		{ return m_init_3d[ZVIEWER]; }
	int x_shift() const			{ return m_init_3d[XSHIFT]; }
	int y_shift() const			{ return m_init_3d[YSHIFT]; }
	int x_light() const			{ return m_init_3d[XLIGHT]; }
	int y_light() const			{ return m_init_3d[YLIGHT]; }
	int z_light() const			{ return m_init_3d[ZLIGHT]; }
	int light_avg() const		{ return m_init_3d[LIGHTAVG]; }
	void get_init_3d(short *data, int count) const;
	void history_save(short *destination) const;
	void history_restore(const short *data);
	int parse_sphere(const cmd_context &context);

	void set_sphere(bool value)		{ m_init_3d[SPHERE] = value ? 1 : 0; }
	void set_x_rot(int value)		{ m_init_3d[XROT] = value; }
	void set_y_rot(int value)		{ m_init_3d[YROT] = value; }
	void set_z_rot(int value)		{ m_init_3d[ZROT] = value; }
	void set_x_scale(int value)		{ m_init_3d[XSCALE] = value; }
	void set_y_scale(int value)		{ m_init_3d[YSCALE] = value; }
	void set_phi1(int value)		{ m_init_3d[PHI1] = value; }
	void set_phi2(int value)		{ m_init_3d[PHI2] = value; }
	void set_theta1(int value)		{ m_init_3d[THETA1] = value; }
	void set_theta2(int value)		{ m_init_3d[THETA2] = value; }
	void set_radius(int value)		{ m_init_3d[RADIUS] = value; }
	void set_rough(int value)		{ m_init_3d[ROUGH] = value; }
	void set_water_line(int value)	{ m_init_3d[WATERLINE] = value; }
	void set_fill_type(int value)	{ m_init_3d[FILLTYPE] = value; }
	void set_z_viewer(int value)	{ m_init_3d[ZVIEWER] = value; }
	void set_x_shift(int value)		{ m_init_3d[XSHIFT] = value; }
	void set_y_shift(int value)		{ m_init_3d[YSHIFT] = value; }
	void set_x_light(int value)		{ m_init_3d[XLIGHT] = value; }
	void set_y_light(int value)		{ m_init_3d[YLIGHT] = value; }
	void set_z_light(int value)		{ m_init_3d[ZLIGHT] = value; }
	void set_light_avg(int value)	{ m_init_3d[LIGHTAVG] = value; }
	void set_defaults();
	void set_init_3d(const short *data, int count);

	int m_raytrace_brief;
	int	m_raytrace_output;
	int m_ambient;
	BYTE m_background_color[3];
	int m_haze;
	int m_randomize_colors;
	int m_x_trans;
	int m_y_trans;
	char m_ray_name[FILE_MAX_PATH];

private:
	enum
	{
		SPHERE = 0,

		XROT = 1,
		YROT,
		ZROT,
		XSCALE,
		YSCALE,

		PHI1 = 1,
		PHI2,
		THETA1,
		THETA2,
		RADIUS,

		ROUGH = 6,
		WATERLINE,
		FILLTYPE,
		ZVIEWER,
		XSHIFT,
		YSHIFT,
		XLIGHT,
		YLIGHT,
		ZLIGHT,
		LIGHTAVG
	};

	int m_init_3d[20];
};

extern RayTraceState g_raytrace_state;

#endif
