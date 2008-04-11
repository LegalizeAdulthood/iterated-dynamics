#if !defined(ITERATED_DYNAMICS_IMPL_H)
#define ITERATED_DYNAMICS_IMPL_H

#include "IteratedDynamics.h"

class IteratedDynamicsApp
{
public:
	virtual ~IteratedDynamicsApp() {}

	virtual void set_exe_path(const char *path) = 0;
	typedef void SignalHandler(int);
	virtual SignalHandler *signal(int number, SignalHandler *handler) = 0;
	virtual void InitMemory() = 0;
	virtual void init_failure(std::string const &message) = 0;
	virtual void init_help() = 0;
	virtual void save_parameter_history() = 0;
	virtual ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag) = 0;
	virtual void srand(unsigned int seed) = 0;
	virtual void command_files(int argc, char **argv) = 0;
	virtual void pause_error(int action) = 0;
	virtual int init_msg(const char *cmdstr, const char *bad_filename, int mode) = 0;
	virtual void history_allocate() = 0;
	virtual void check_same_name() = 0;
	virtual void intro() = 0;
	virtual void goodbye() = 0;
	virtual void set_if_old_bif() = 0;
	virtual void set_help_mode(int new_mode) = 0;
	virtual int get_commands() = 0;
	virtual int get_fractal_type() = 0;
	virtual int get_toggles() = 0;
	virtual int get_toggles2() = 0;
	virtual int get_fractal_parameters(bool type_specific) = 0;
	virtual int get_view_params() = 0;
	virtual int get_fractal_3d_parameters() = 0;
	virtual int get_command_string() = 0;
	virtual int open_drivers(int &argc, char **argv) = 0;
	virtual void exit(int code) = 0;
	virtual int main_menu(bool full_menu) = 0;
	virtual int select_video_mode(int curmode) = 0;
	virtual int check_video_mode_key(int k) = 0;
	virtual int read_overlay() = 0;
	virtual int clock_ticks() = 0;
	virtual int get_a_filename(const std::string &heading, std::string &fileTemplate, std::string &filename) = 0;
};

class Externals;

class IteratedDynamicsImpl : public IteratedDynamics
{
public:
	IteratedDynamicsImpl(IteratedDynamicsApp &app,
		AbstractDriver *driver,
		Externals &externs,
		IGlobals &globals,
		int argc, char **argv);
	virtual ~IteratedDynamicsImpl();
	virtual int Main();

private:
	void Initialize();
	void Restart();
	void RestoreStart();
	void ImageStart();

	ApplicationStateType _state;
	int _argc;
	char **_argv;
	bool _resumeFlag;
	bool _screenStacked;
	bool _keyboardMore;
	IteratedDynamicsApp &_app;
	AbstractDriver *_driver;
	Externals &_externs;
	IGlobals &_g;
};

#endif
