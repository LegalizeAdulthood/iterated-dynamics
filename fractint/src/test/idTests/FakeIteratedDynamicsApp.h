#if !defined(FAKE_ITERATED_DYNAMICS_APP_H)
#define FAKE_ITERATED_DYNAMICS_APP_H

#include "IteratedDynamics.h"
#include "NotImplementedException.h"

class FakeIteratedDynamicsApp : public IteratedDynamicsApp
{
public:
	virtual ~FakeIteratedDynamicsApp() {}

	virtual void set_exe_path(const char *path)
	{ throw not_implemented("set_exe_path"); }
	virtual IteratedDynamicsApp::SignalHandler *signal(int number, IteratedDynamicsApp::SignalHandler *handler)
	{ throw not_implemented("signal"); }
	virtual void InitMemory()
	{ throw not_implemented("InitMemory"); }
	virtual void init_failure(std::string const &message)
	{ throw not_implemented("init_failure"); }
	virtual void init_help()
	{ throw not_implemented("init_help"); }
	virtual void save_parameter_history()
	{ throw not_implemented("save_parameter_history"); }
	virtual ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag)
	{ throw not_implemented("big_while_loop"); }
	virtual void srand(unsigned int seed)
	{ throw not_implemented("srand"); }
	virtual void command_files(int argc, char **argv)
	{ throw not_implemented("command_files"); }
	virtual void pause_error(int action)
	{ throw not_implemented("pause_error"); }
	virtual int init_msg(const char *cmdstr, const char *bad_filename, int mode)
	{ throw not_implemented("init_msg"); }
	virtual void history_allocate()
	{ throw not_implemented("history_allocate"); }
	virtual void check_same_name()
	{ throw not_implemented("check_same_name"); }
	virtual void intro()
	{ throw not_implemented("intro"); }
	virtual void goodbye()
	{ throw not_implemented("goodbye"); }
	virtual void set_if_old_bif()
	{ throw not_implemented("set_if_old_bif"); }
	virtual void set_help_mode(int new_mode)
	{ throw not_implemented("set_help_mode"); }
	virtual int get_commands()
	{ throw not_implemented("get_commands"); }
	virtual int get_fractal_type()
	{ throw not_implemented("get_fractal_type"); }
	virtual int get_toggles()
	{ throw not_implemented("get_toggles"); }
	virtual int get_toggles2()
	{ throw not_implemented("get_toggles2"); }
	virtual int get_fractal_parameters(bool type_specific)
	{ throw not_implemented("get_fractal_parameters"); }
	virtual int get_view_params()
	{ throw not_implemented("get_view_params"); }
	virtual int get_fractal_3d_parameters()
	{ throw not_implemented("get_fractal_3d_parameters"); }
	virtual int get_command_string()
	{ throw not_implemented("get_command_string"); }
	virtual int open_drivers(int &argc, char **argv)
	{ throw not_implemented("open_drivers"); }
	virtual void exit(int code)
	{ throw not_implemented("exit"); }
	virtual int main_menu(bool full_menu)
	{ throw not_implemented("main_menu"); }
	virtual int select_video_mode(int curmode)
	{ throw not_implemented("select_video_mode"); }
	virtual int check_video_mode_key(int k)
	{ throw not_implemented("check_video_mode_key"); }
	virtual int read_overlay()
	{ throw not_implemented("read_overlay"); }
	virtual int clock_ticks()
	{ throw not_implemented("clock_ticks"); }
	virtual int get_a_filename(const std::string &heading, std::string &fileTemplate, std::string &filename)
	{ throw not_implemented("get_a_filename"); }
};

#endif
