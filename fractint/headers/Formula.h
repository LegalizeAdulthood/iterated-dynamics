#if !defined(FORMULA_H)
#define FORMULA_H

struct var_list_st
{
	char name[34];
	var_list_st *next_item;
};

struct const_list_st
{
	_CMPLX complex_const;
	const_list_st *next_item;
};

typedef void t_function();

class Formula
{
public:
	Formula();
	~Formula();

	int orbit();
	int per_pixel();

	void end_init();
	int RunForm(char *Name, int from_prompts1c);
	char *PrepareFormula(FILE *file, int from_prompts1c);

	int setup_fp();
	int setup_int();
	void init_misc();
	void free_work_area();

	void StackClear();
	void StackJump();
	void StackJumpOnFalse_d();
	void StackJumpOnFalse_m();
	void StackJumpOnFalse_l();
	void StackJumpOnTrue_d();
	void StackJumpOnTrue_m();
	void StackJumpOnTrue_l();
	void StackJumpLabel();
	void StackLoadDup_d();
	void StackLoadSqr_d();
	void StackLoadSqr2_d();
	void StackLoadDouble();
	void StackStore();
	void StackLoad();

	int get_parameter(const char *name);

	int total_formula_memory() const	{ return m_total_formula_mem; }
	int parser_vsp() const				{ return m_parser_vsp; }
	int max_ops() const					{ return m_formula_max_ops; }
	int max_args() const				{ return m_formula_max_args; }
	const char *info_line1() const;
	const char *info_line2() const;
	bool uses_is_mand() const			{ return m_uses_is_mand; }
	bool uses_p1() const				{ return m_uses_p1; }
	bool uses_p2() const				{ return m_uses_p2; }
	bool uses_p3() const				{ return m_uses_p3; }
	bool uses_p4() const				{ return m_uses_p4; }
	bool uses_p5() const				{ return m_uses_p5; }
	int max_fn() const					{ return m_max_fn; }

	void set_uses_is_mand(bool value)	{ m_uses_is_mand = value; }
	void set_uses_p1(bool value)		{ m_uses_p1 = value; }
	void set_uses_p2(bool value)		{ m_uses_p2 = value; }
	void set_uses_p3(bool value)		{ m_uses_p3 = value; }
	void set_uses_p4(bool value)		{ m_uses_p4 = value; }
	void set_uses_p5(bool value)		{ m_uses_p5 = value; }
	void set_max_fn(int value)			{ m_max_fn = value; }

private:
	enum MATH_TYPE m_math_type;
	int m_number_of_ops;
	int m_number_of_loads;
	int m_number_of_stores;
	int m_number_of_jumps;
	int m_initial_jump_index;
	int m_variable_count;
	int m_complex_count;
	int m_real_count;
	int m_chars_in_formula;
	Arg m_argument_stack[20];
	int m_next_operation;
	int m_initial_n;
	int m_parenthesis_count;
	int m_expecting_arg;
	int m_set_random;
	var_list_st *m_variable_list;
	const_list_st *m_complex_list;
	const_list_st *m_real_list;
	int m_last_op;
	int m_parser_vsp;
	int m_formula_max_ops;
	int m_formula_max_args;
	int m_total_formula_mem;
	int m_op_ptr;
	bool m_uses_jump;
	int m_jump_index;
	int m_store_ptr;
	int m_load_ptr;
	bool m_is_mand;
	int m_posp;
	int m_last_init_op;
	bool m_uses_is_mand;
	double m_fudge_limit;
	bool m_uses_p1;
	bool m_uses_p2;
	bool m_uses_p3;
	bool m_uses_p4;
	bool m_uses_p5;
	int m_max_fn;

	ConstArg *is_constant(char *text, int length);
	int ParseStr(char *text, int pass);
	void allocate();
	void count_lists();
	int prescan(FILE *open_file);
	void RecSortPrec();
	void display_var_list();
	void display_const_lists();
	void init_var_list();
	void init_const_lists();
	const char *error_messages(int which);
	int frm_check_name_and_sym(FILE *open_file, int report_bad_sym);
	void frm_error(FILE *open_file, long begin_frm);
	int fill_jump_struct();
	int fill_jump_struct_fp();
	int CvtStk();
	int CvtFptr(void (* ffptr)(), int MinStk, int FreeStk, int Delta);
	t_function *is_function(char *str, int len);
};

extern Arg *Arg1;
extern Arg *Arg2;
extern double _1_;
extern double _2_;
extern Arg s[20];
extern Arg **Store;
extern Arg **Load;
extern ConstArg *v;
extern int InitLodPtr;
extern int InitStoPtr;
extern int InitOpPtr;
extern void (**f)();
extern JUMP_CONTROL_ST jump_control[];
extern Formula g_formula_state;

#endif
