#if !defined(ESCAPE_TIME_H)
#define ESCAPE_TIME_H

template <typename T>
class EscapeTimeState
{
public:
	EscapeTimeState<T>() :
		m_width(0),
		m_height(0),
		m_x0(NULL),
		m_y0(NULL),
		m_x1(NULL),
		m_y1(NULL),
		m_x_min(0),
		m_x_max(0),
		m_x_3rd(0),
		m_y_min(0),
		m_y_max(0),
		m_y_3rd(0),
		m_delta_x(0),
		m_delta_y(0),
		m_delta_x2(0),
		m_delta_y2(0)
	{
	}
	~EscapeTimeState<T>()
	{
		free_grid_pointers();
	}

	void free_grid_pointers()
	{
		m_width = 0;
		m_height = 0;
		if (m_x0)
		{
			delete[] m_x0;
			m_x0 = NULL;
		}
		m_x1 = NULL;
		m_y0 = NULL;
		m_y1 = NULL;
	}

	void set_grid_pointers(int width, int height)
	{
		free_grid_pointers();
		m_width = width;
		m_height = height;
		m_x0 = new T[2*width + 2*height];
		m_y0 = &m_x0[width];
		m_x1 = &m_y0[height];
		m_y1 = &m_x1[width];
	}

	T x_pixel_grid(int col, int row) const { return m_x0[col] + m_x1[row]; }
	T y_pixel_grid(int col, int row) const { return m_y0[row] + m_y1[col]; }

	void fill()
	{
		m_x0[0] = m_x_min;              /* fill up the x, y grids */
		m_y0[0] = m_y_max;
		m_x1[0] = 0;
		m_y1[0] = 0;
		for (int i = 1; i < m_width; i++)
		{
			m_x0[i] = m_x0[0] + i*m_delta_x;
			m_y1[i] = m_y1[0] - i*m_delta_y2;
		}
		for (int i = 1; i < m_height; i++)
		{
			m_y0[i] = m_y0[0] - i*m_delta_y;
			m_x1[i] = m_x1[0] + i*m_delta_x2;
		}
	}

	T x0(int idx) const { return m_x0[idx]; }
	T y0(int idx) const { return m_y0[idx]; }
	T x1(int idx) const { return m_x1[idx]; }
	T y1(int idx) const { return m_y1[idx]; }

	T x_min() const		{ return m_x_min; }
	T x_max() const		{ return m_x_max; }
	T x_3rd() const		{ return m_x_3rd; }
	T y_min() const		{ return m_y_min; }
	T y_max() const		{ return m_y_max; }
	T y_3rd() const		{ return m_y_3rd; }
	T delta_x() const	{ return m_delta_x; }
	T delta_y() const	{ return m_delta_y; }
	T delta_x2() const	{ return m_delta_x2; }
	T delta_y2() const	{ return m_delta_y2; }

	T &x_min()			{ return m_x_min; }
	T &x_max()			{ return m_x_max; }
	T &x_3rd()			{ return m_x_3rd; }
	T &y_min()			{ return m_y_min; }
	T &y_max()			{ return m_y_max; }
	T &y_3rd()			{ return m_y_3rd; }
	T &delta_x()		{ return m_delta_x; }
	T &delta_y()		{ return m_delta_y; }
	T &delta_x2()		{ return m_delta_x2; }
	T &delta_y2()		{ return m_delta_y2; }

private:
	int m_width;
	int m_height;
	T *m_x0;
	T *m_y0;
	T *m_x1;
	T *m_y1;
	T m_x_min;
	T m_x_max;
	T m_x_3rd;
	T m_y_min;
	T m_y_max;
	T m_y_3rd;
	T m_delta_x;
	T m_delta_y;
	T m_delta_x2;
	T m_delta_y2;
};

extern EscapeTimeState<bf_t>	g_escape_time_state_bf;
extern EscapeTimeState<double>	g_escape_time_state_fp;
extern EscapeTimeState<long>	g_escape_time_state_l;

#endif
