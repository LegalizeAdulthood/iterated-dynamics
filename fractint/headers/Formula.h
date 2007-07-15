#if !defined(FORMULA_H)
#define FORMULA_H

#define MAX_JUMPS 200  /* size of JUMP_CONTROL array */

struct error_data
{
	long start_pos;
	long error_pos;
	int error_number;
};

struct JUMP_PTRS
{
	int JumpOpPtr;
	int JumpLodPtr;
	int JumpStoPtr;
};

enum JumpType
{
	JUMPTYPE_NONE = 0,
	JUMPTYPE_IF = 1,
	JUMPTYPE_ELSEIF = 2,
	JUMPTYPE_ELSE = 3,
	JUMPTYPE_ENDIF = 4
};

struct JUMP_CONTROL
{
	JumpType type;
	JUMP_PTRS ptrs;
	int DestJumpIndex;
};

enum VariableNames
{
	VARIABLE_PIXEL = 0,
	VARIABLE_P1,
	VARIABLE_P2,
	VARIABLE_Z,
	VARIABLE_LAST_SQR,
	VARIABLE_PI,
	VARIABLE_E,
	VARIABLE_RAND,
	VARIABLE_P3,
	VARIABLE_WHITE_SQ,
	VARIABLE_SCRN_PIX,
	VARIABLE_SCRN_MAX,
	VARIABLE_MAX_IT,
	VARIABLE_IS_MAND,
	VARIABLE_CENTER,
	VARIABLE_MAG_X_MAG,
	VARIABLE_ROT_SKEW,
	VARIABLE_P4,
	VARIABLE_P5
};

/* function, load, store pointers */
struct function_load_store 
{ 
	void (*function)();
	union Arg *operand;
};

/* token_type definitions */
enum FormulaTokenType
{
	TOKENTYPE_ERROR = 0,
	TOKENTYPE_PARENTHESIS,
	TOKENTYPE_PARAMETER_VARIABLE,
	TOKENTYPE_USER_VARIABLE,
	TOKENTYPE_PREDEFINED_VARIABLE,
	TOKENTYPE_REAL_CONSTANT,
	TOKENTYPE_COMPLEX_CONSTANT,
	TOKENTYPE_FUNCTION,
	TOKENTYPE_PARAMETER_FUNCTION,
	TOKENTYPE_FLOW_CONTROL,
	TOKENTYPE_OPERATOR,
	TOKENTYPE_END_OF_FORMULA
};

typedef void t_function();
typedef t_function *t_function_pointer;

struct var_list_st;
struct const_list_st;

// TODO: Separate parsing and formula execution responsibilities into two classes
class Formula
{
public:
	Formula();
	~Formula();

	int orbit();
	int per_pixel();

	void end_init();
	bool RunFormula(const char *name, bool report_bad_symmetry);
	const char *PrepareFormula(FILE *file, bool report_bad_symmetry);

	bool setup_fp();
	bool setup_int();
	void init_misc();
	void free_work_area();

	void StackClear();
	void StackJump();
	void StackJumpOnFalse_d();
	void StackJumpOnFalse_l();
	void StackJumpOnTrue_d();
	void StackJumpOnTrue_l();
	void StackJumpLabel();
	void StackLoadDup_d();
	void StackLoadSqr_d();
	void StackLoadSqr2_d();
	void StackLoadDouble();
	void StackStore();
	void StackLoad();
	void Random_l();
	void Random_d();
	void StackStoreRandom_l();
	void StackStoreRandom_d();
	void StackSqr0();
	void StackSqr_d();
	void StackSqr_l();
	void get_parameter(const char *name);
	int find_item(FILE **file);
	long get_file_entry(char *wildcard);
	bool merge_formula_filename(char *new_filename, int mode);

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
	int max_fn() const					{ return m_max_function_number; }
	const char *get_filename() const	{ return m_filename; }
	const char *get_formula() const		{ return m_formula_name; }

	void set_uses_is_mand(bool value)	{ m_uses_is_mand = value; }
	void set_uses_p1(bool value)		{ m_uses_p1 = value; }
	void set_uses_p2(bool value)		{ m_uses_p2 = value; }
	void set_uses_p3(bool value)		{ m_uses_p3 = value; }
	void set_uses_p4(bool value)		{ m_uses_p4 = value; }
	void set_uses_p5(bool value)		{ m_uses_p5 = value; }
	void set_max_fn(int value)			{ m_max_function_number = value; }
	void set_filename(const char *value);
	void set_formula(const char *value);

private:
	enum MathType m_math_type;
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
	bool m_expecting_arg;
	int m_set_random;
	var_list_st *m_variable_list;
	const_list_st *m_complex_list;
	const_list_st *m_real_list;
	int m_last_op;
	int m_parser_vsp;
	int m_formula_max_ops;
	int m_formula_max_args;
	int m_op_index;
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
	int m_max_function_number;
	function_load_store *m_function_load_store_pointers;
	ConstArg *m_variables;
	Arg **m_store;
	Arg **m_load;
	t_function_pointer *m_functions;
	JUMP_CONTROL m_jump_control[MAX_JUMPS];
	Arg m_arg1;
	Arg m_arg2;
	const char *m_formula_text;
	error_data m_errors[50];
	long m_file_pos;
	long m_statement_pos;
	int m_errors_found;
	char m_prepare_formula_text[16384];
	int m_initial_load_pointer;
	int m_initial_store_pointer;
	int m_initial_op_pointer;
	char m_filename[FILE_MAX_PATH];		/* file to find (type=)formulas in */
	char m_formula_name[ITEMNAMELEN + 1];		/* Name of the Formula (if not null) */

	bool formula_defined() const
	{
		return m_formula_name[0] != 0;
	}
	void set_function(int idx, t_function_pointer function)
	{
		m_function_load_store_pointers[idx].function = function;
	}
	void set_no_function(int idx)
	{
		set_function(idx, NULL);
	}
	bool is_function(int idx, t_function_pointer function)
	{
		return m_function_load_store_pointers[idx].function == function;
	}
	bool is_no_function(int idx)
	{
		return is_function(idx, NULL);
	}
	t_function_pointer get_function(int idx)
	{
		return m_function_load_store_pointers[idx].function;
	}
	void set_no_operand(int idx)
	{
		m_function_load_store_pointers[idx].operand = NULL;
	}
	void set_operand_next(int idx)
	{
		m_function_load_store_pointers[idx].operand = m_function_load_store_pointers[idx + 1].operand;
	}
	void set_operand_prev_next(int idx)
	{
		m_function_load_store_pointers[idx - 1].operand = m_function_load_store_pointers[idx + 1].operand;
	}
	Arg *get_operand(int idx)
	{
		return m_function_load_store_pointers[idx].operand;
	}
	void set_operand(int idx, Arg *operand)
	{
		m_function_load_store_pointers[idx].operand = operand;
	}

	ConstArg *is_constant(const char *text, int length);
	bool parse_string(const char *text, int pass);
	void parse_string_set_variables();
	void parse_string_set_parameters_int();
	void parse_string_set_parameters_float();
	void parse_string_set_center_magnification_variables();
	void parse_string_set_constants();
	void parse_string_set_math();

	void store_function(void (*function)(), int offset, int store_count);
	void store_function(void (*function)(), int p);

	int get_prec(int offset, int store_count);

	void allocate();
	void count_lists();
	bool prescan(FILE *open_file);
	void sort_prec();
	void display_var_list();
	void display_const_lists();
	void init_var_list();
	void init_const_lists();
	const char *error_messages(int which);
	bool check_name_and_symmetry(FILE *open_file, bool report_bad_symmetry);
	void frm_error(FILE *open_file, long begin_frm);
	bool fill_jump_struct();
	bool fill_jump_struct_fp();
	void CvtStk();
	bool CvtFptr(void (*function)(), int MinStk, int FreeStk, int Delta);
	t_function *is_function(const char *str, int len);
	int fill_if_group(int endif_index, JUMP_PTRS *jump_data);
	void record_error(int error_code);
	void peephole_optimizer(t_function_pointer &function);
	void peephole_optimize_load(t_function_pointer &function);
	void peephole_optimize_load_load(t_function_pointer &function);
	void peephole_optimize_load_store_same_value(t_function_pointer &function);
	void peephole_optimize_load_clear_load_same_value(t_function_pointer &function);
	void peephole_optimize_load_lastsqr_real(t_function_pointer &function);
	void peephole_optimize_load_real_constant(t_function_pointer &function);
	void peephole_optimize_add(t_function_pointer &function);
	void peephole_optimize_add_load_dup(t_function_pointer &function);
	void peephole_optimize_add_store_dup(t_function_pointer &function);
	void peephole_optimize_add_load(t_function_pointer &function);
	void peephole_optimize_add_load_real(t_function_pointer &function);
	void peephole_optimize_add_load_imaginary(t_function_pointer &function);
	void peephole_optimize_sub(t_function_pointer &function);
	void peephole_optimize_store_clear(t_function_pointer &function);
	void peephole_optimize_mul(t_function_pointer &function);
	void peephole_optimize_mul_load_dup(t_function_pointer &function);
	void peephole_optimize_mul_store_dup(t_function_pointer &function);
	void peephole_optimize_mul_load(t_function_pointer &function);
	void peephole_optimize_mul_load_push2(t_function_pointer &function);
	void peephole_optimize_mul_load_swap_loads();
	void peephole_optimize_mul_load_adjust_index();
	void peephole_optimize_mul_load_2(t_function_pointer &function);
	void peephole_optimize_mul_load_real(t_function_pointer &function);
	void peephole_optimize_mul_real(t_function_pointer &function);
	void peephole_optimize_mul_load_imaginary(t_function_pointer &function);
	void peephole_optimize_mul_load_less(t_function_pointer &function);
	void peephole_optimize_mul_load_less_equal(t_function_pointer &function);
	void peephole_optimize_divide(t_function_pointer &function);
	void peephole_optimize_real(t_function_pointer &function);
	void peephole_optimize_load_imaginary(t_function_pointer &function);
	void peephole_optimize_load_conjugate(t_function_pointer &function);
	void peephole_optimize_modulus(t_function_pointer &function);
	void peephole_optimize_flip(t_function_pointer &function);
	void peephole_optimize_abs(t_function_pointer &function);
	void peephole_optimize_sqr(t_function_pointer &function);
	void peephole_optimize_power(t_function_pointer &function);
	void peephole_optimize_power_load_real_constant(t_function_pointer &function);
	void peephole_optimize_power_load_real_constant_special(t_function_pointer &function, double constant);
	void peephole_optimize_power_load_real_constant_other(t_function_pointer &function);
	void peephole_optimize_power_load_real(t_function_pointer &function);
	void peephole_optimize_less_equal(t_function_pointer &function);
	void peephole_optimize_less(t_function_pointer &function);
	void peephole_optimize_greater(t_function_pointer &function);
	void peephole_optimize_greater_equal(t_function_pointer &function);
	void peephole_optimize_not_equal(t_function_pointer &function);
	void peephole_optimize_equal(t_function_pointer &function);
	void FinalOptimizations(t_function_pointer &out_function);
};

extern Arg *Arg1;
extern Arg *Arg2;
extern Arg s[20];
extern Formula g_formula_state;

extern void dStkAbs();
extern void lStkAbs();
extern void dStkSqr();
extern void lStkSqr();
extern void dStkAdd();
extern void lStkAdd();
extern void dStkSub();
extern void lStkSub();
extern void dStkConj();
extern void lStkConj();
extern void dStkZero();
extern void lStkZero();
extern void dStkOne();
extern void lStkOne();
extern void dStkReal();
extern void lStkReal();
extern void dStkImag();
extern void lStkImag();
extern void dStkNeg();
extern void lStkNeg();
extern void dStkMul();
extern void lStkMul();
extern void dStkDiv();
extern void lStkDiv();
extern void StkSto();
extern void StkLod();
extern void dStkMod();
extern void lStkMod();
extern void StkClr();
extern void dStkFlip();
extern void lStkFlip();
extern void dStkSin();
extern void lStkSin();
extern void dStkTan();
extern void lStkTan();
extern void dStkTanh();
extern void lStkTanh();
extern void dStkCoTan();
extern void lStkCoTan();
extern void dStkCoTanh();
extern void lStkCoTanh();
extern void dStkRecip();
extern void lStkRecip();
extern void StkIdent();
extern void dStkSinh();
extern void lStkSinh();
extern void dStkCos();
extern void lStkCos();
extern void dStkCosXX();
extern void lStkCosXX();
extern void dStkCosh();
extern void lStkCosh();
extern void dStkLT();
extern void lStkLT();
extern void dStkGT();
extern void lStkGT();
extern void dStkLTE();
extern void lStkLTE();
extern void dStkGTE();
extern void lStkGTE();
extern void dStkEQ();
extern void lStkEQ();
extern void dStkNE();
extern void lStkNE();
extern void dStkOR();
extern void lStkOR();
extern void dStkAND();
extern void lStkAND();
extern void dStkLog();
extern void lStkLog();
extern void FPUcplxexp(ComplexD *, ComplexD *);
extern void dStkExp();
extern void lStkExp();
extern void dStkPwr();
extern void lStkPwr();
extern void dStkASin();
extern void lStkASin();
extern void dStkASinh();
extern void lStkASinh();
extern void dStkACos();
extern void lStkACos();
extern void dStkACosh();
extern void lStkACosh();
extern void dStkATan();
extern void lStkATan();
extern void dStkATanh();
extern void lStkATanh();
extern void dStkCAbs();
extern void lStkCAbs();
extern void dStkSqrt();
extern void lStkSqrt();
extern void dStkFloor();
extern void lStkFloor();
extern void dStkCeil();
extern void lStkCeil();
extern void dStkTrunc();
extern void lStkTrunc();
extern void dStkRound();
extern void lStkRound();
extern int fFormula();
extern int formula_orbit();
extern int bad_formula();
extern int form_per_pixel();
extern int formula_setup_fp();
extern int formula_setup_int();
extern void EndInit();

#endif
