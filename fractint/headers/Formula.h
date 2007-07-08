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

struct JUMP_CONTROL
{
	int type;
	JUMP_PTRS ptrs;
	int DestJumpIndex;
};

/* function, load, store pointers */
struct function_load_store 
{ 
	void (*function)();
	union Arg *operand;
};

/* token_type definitions */
enum TokenType
{
	TOKENTYPE_NONE = 0,
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

struct token_st
{
	char token_str[80];
	TokenType token_type;
	int token_id;
	ComplexD token_const;
};

struct var_list_st
{
	char name[34];
	var_list_st *next_item;

	void display() const;

	static var_list_st *add(var_list_st *list, token_st tok);
};

struct const_list_st
{
	ComplexD complex_const;
	const_list_st *next_item;

	void display(const char *title) const;

	static const_list_st *add(const_list_st *p, token_st tok);
};

typedef void t_function();
typedef t_function *t_function_pointer;

// TODO: Separate parsing and formula execution responsibilities into two classes
class Formula
{
public:
	Formula();
	~Formula();

	int orbit();
	int per_pixel();

	void end_init();
	int RunFormula(const char *name, bool report_bad_symmetry);
	const char *PrepareFormula(FILE *file, bool report_bad_symmetry);

	int setup_fp();
	int setup_int();
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

	int get_parameter(const char *name);

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

	void set_uses_is_mand(bool value)	{ m_uses_is_mand = value; }
	void set_uses_p1(bool value)		{ m_uses_p1 = value; }
	void set_uses_p2(bool value)		{ m_uses_p2 = value; }
	void set_uses_p3(bool value)		{ m_uses_p3 = value; }
	void set_uses_p4(bool value)		{ m_uses_p4 = value; }
	void set_uses_p5(bool value)		{ m_uses_p5 = value; }
	void set_max_fn(int value)			{ m_max_function_number = value; }

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
	bool m_expecting_arg;
	int m_set_random;
	var_list_st *m_variable_list;
	const_list_st *m_complex_list;
	const_list_st *m_real_list;
	int m_last_op;
	int m_parser_vsp;
	int m_formula_max_ops;
	int m_formula_max_args;
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

	ConstArg *is_constant(const char *text, int length);
	bool ParseStr(const char *text, int pass);
	void StoreFunction(void (*function)(), int offset, int store_count);
	void StoreFunction(void (*function)(), int p);

	int GetP(int offset, int store_count);

	void allocate();
	void count_lists();
	int prescan(FILE *open_file);
	void RecSortPrec();
	void display_var_list();
	void display_const_lists();
	void init_var_list();
	void init_const_lists();
	const char *error_messages(int which);
	int check_name_and_symmetry(FILE *open_file, bool report_bad_symmetry);
	void frm_error(FILE *open_file, long begin_frm);
	int fill_jump_struct();
	int fill_jump_struct_fp();
	int CvtStk();
	int CvtFptr(void (* ffptr)(), int MinStk, int FreeStk, int Delta);
	t_function *is_function(const char *str, int len);
	int fill_if_group(int endif_index, JUMP_PTRS *jump_data);
	void record_error(int error_code);
};

extern Arg *Arg1;
extern Arg *Arg2;
extern Arg s[20];
extern int InitLodPtr;
extern int InitStoPtr;
extern int InitOpPtr;
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
extern int BadFormula();
extern int form_per_pixel();
extern int formula_setup_fp();
extern int formula_setup_int();
extern void EndInit();

#endif
