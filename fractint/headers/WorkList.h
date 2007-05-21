#if !defined(WORK_LIST_H)
#define WORK_LIST_H

struct WorkListItem    /* work list entry for std escape time engines */
{
	int xx_start;    /* screen window for this entry */
	int xx_stop;
	int yy_start;
	int yy_stop;
	int yy_begin;    /* start row within window, for 2pass/ssg resume */
	int sym;        /* if symmetry in window, prevents bad combines */
	int pass;       /* for 2pass and solid guessing */
	int xx_begin;    /* start col within window, =0 except on resume */
};

enum
{
	MAX_WORK_LIST = 40,
	MAX_Y_BLOCK = 7,		/* MAX_X_BLOCK*MAX_Y_BLOCK*2 <= 4096, the size of "prefix" */
	MAX_X_BLOCK = 202		/* each maxnblk is oversize by 2 for a "border" */
							/* MAX_X_BLOCK defn must match fracsubr.c */
};

class WorkList
{
public:
	WorkList();
	~WorkList();

	// accessors
	int num_items() const	{ return m_num_items; }
	const WorkListItem &item(int i) const
	{
		return m_items[i];
	}
	int xx_start() const	{ return m_xx_start; }
	int xx_stop() const		{ return m_xx_stop; }
	int yy_start() const	{ return m_yy_start; }
	int yy_stop() const		{ return m_yy_stop; }
	int xx_begin() const	{ return m_xx_begin; }
	int yy_begin() const	{ return m_yy_begin; }
	int work_pass() const	{ return m_work_pass; }
	int ix_start() const	{ return m_ix_start; }
	int iy_start() const	{ return m_iy_start; }
	int work_sym() const	{ return m_work_sym; }
	int get_lowest_pass() const;

	// mutators
	void set_xx_start(int value)	{ m_xx_start = value; }
	void set_xx_stop(int value)		{ m_xx_stop = value; }
	void set_yy_start(int value)	{ m_yy_start = value; }
	void set_yy_stop(int value)		{ m_yy_stop = value; }
	void set_xx_begin(int value)	{ m_xx_begin = value; }
	void set_yy_begin(int value)	{ m_yy_begin = value; }
	void set_work_pass(int value)	{ m_work_pass = value; }
	void set_ix_start(int value)	{ m_ix_start = value; }
	void set_iy_start(int value)	{ m_iy_start = value; }
	void set_work_sym(int value)	{ m_work_sym = value; }

	// methods
	void perform();
	void setup_initial_work_list();
	void get_top_item();
	void get_resume();
	void put_resume();
	void offset_items(int row, int col);
	void reset_items()				{ m_num_items = 0; }
	void restart_item(int i);
	void fix();
	void tidy();
	int combine();
	int add(int xfrom, int xto, int xbegin,
		int yfrom, int yto, int ybegin,
		int pass, int sym);

private:
	int m_num_items;
	WorkListItem m_items[MAX_WORK_LIST];
	int m_xx_start;
	int m_xx_stop;
	int m_yy_start;
	int m_yy_stop;
	int m_xx_begin;
	int m_yy_begin;
	int m_work_pass;
	int m_ix_start;
	int m_iy_start;
	int m_work_sym;
};

extern WorkList g_WorkList;

extern void perform_work_list();

#endif
