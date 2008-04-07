#include <csignal>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "Browse.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "decoder.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
#include "idhelp.h"
#include "filesystem.h"
#include "idmain.h"
#include "fracsubr.h"
#include "framain2.h"
#include "history.h"
#include "intro.h"
#include "loadfile.h"
#include "miscovl.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"

#include "CommandParser.h"
#include "Formula.h"
#include "SoundState.h"
#include "ViewWindow.h"
#include "IteratedDynamics.h"
#include "Externals.h"
#include "IteratedDynamicsImpl.h"

static std::string null_string(char const *ptr)
{
	return !ptr ? "" : ptr;
}

// Do nothing if math error
static Externals *s_fpu_error_externs = 0;

static void my_floating_point_err(int sig)
{
	if (sig != 0)
	{
		s_fpu_error_externs->SetOverflow(true);
	}
}

IteratedDynamicsImpl::IteratedDynamicsImpl(IteratedDynamicsApp &app,
										   AbstractDriver *driver,
										   Externals &externs,
										   IGlobals &globals,
										   int argc, char **argv)
	: _state(APPSTATE_RESTART),
	_argc(argc),
	_argv(argv),
	_resumeFlag(false),
	_screenStacked(false),
	_keyboardMore(false),
	_app(app),
	_driver(driver),
	_externs(externs),
	_g(globals)
{
}

IteratedDynamicsImpl::~IteratedDynamicsImpl()
{
}

int IteratedDynamicsImpl::Main()
{
	Initialize();

	while (_state != APPSTATE_NO_CHANGE)
	{
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		switch (_state)
		{
		case APPSTATE_RESTART:
			Restart();
			break;

		case APPSTATE_RESTORE_START:
			RestoreStart();
			break;

		case APPSTATE_IMAGE_START:
			ImageStart();
			break;

		case APPSTATE_RESUME_LOOP:
			_app.save_parameter_history();
			_state = _app.big_while_loop(_keyboardMore, _screenStacked, _resumeFlag);
			break;
		}
	}

	return 0;
}

#if !defined(SOURCE_DIR)
#define SOURCE_DIR "."
#endif

void IteratedDynamicsImpl::Initialize()
{
	_app.set_exe_path(_argv[0]);

	_externs.SetFractDir1(null_string(getenv("FRACTDIR")));
	if (_externs.FractDir1().length() == 0)
	{
		_externs.SetFractDir1(".");
	}
	_externs.SetFractDir2(SOURCE_DIR);

	// this traps non-math library floating point errors
	s_fpu_error_externs = &_externs;
	_app.signal(SIGFPE, my_floating_point_err);

	_externs.SetOverflow(false);
	_app.InitMemory();

	// let drivers add their video modes
	if (!_app.open_drivers(_argc, _argv))
	{
		_app.init_failure("Sorry, I couldn't find any working video drivers for your system\n");
		_app.exit(-1);
	}
	_app.init_help();
}

void IteratedDynamicsImpl::ImageStart()
{
	if (_screenStacked)
	{
		_driver->discard_screen();
		_screenStacked = false;
	}
	_externs.SetTabStatus(TAB_STATUS_NONE);

	if (_externs.ShowFile() != SHOWFILE_PENDING)
	{
		if (_externs.CalculationStatus() > CALCSTAT_PARAMS_CHANGED)              // goto imagestart implies re-calc
		{
			_externs.SetCalculationStatus(CALCSTAT_PARAMS_CHANGED);
		}
	}

	if (!_externs.InitializeBatch())
	{
		_driver->set_mouse_mode(-IDK_PAGE_UP);           // just mouse left button, == pgup
	}

	_externs.SetCycleLimit(_externs.InitialCycleLimit()); // default cycle limit
	_g.SetAdapter(_g.InitialVideoMode());                  // set the video adapter up
	_g.SetInitialVideoModeNone();                       // (once)

	while (_g.Adapter() < 0)                // cycle through instructions
	{
		if (_externs.InitializeBatch())                          // batch, nothing to do
		{
			_externs.SetInitializeBatch(INITBATCH_BAILOUT_INTERRUPTED); // exit with error condition set
			_app.goodbye();
		}
		int kbdchar = _app.main_menu(false);
		if (kbdchar == IDK_INSERT) // restart pgm on Insert Key
		{
			_state = APPSTATE_RESTART;
			return;
		}
		if (kbdchar == IDK_DELETE)                    // select video mode list
		{
			kbdchar = _app.select_video_mode(-1);
		}
		_g.SetAdapter(_app.check_video_mode_key(kbdchar));
		if (_g.Adapter() >= 0)
		{
			break;                                 // got a video mode now
		}
		if ('A' <= kbdchar && kbdchar <= 'Z')
		{
			kbdchar = tolower(kbdchar);
		}
		if (kbdchar == 'd')  // shell to DOS
		{
			_driver->set_clear();
			_driver->shell();
			_state = APPSTATE_IMAGE_START;
			return;
		}

		if (kbdchar == '@' || kbdchar == '2')  // execute commands
		{
			if ((_app.get_commands() & COMMANDRESULT_3D_YES) == 0)
			{
				_state = APPSTATE_IMAGE_START;
				return;
			}
			kbdchar = '3';                         // 3d=y so fall thru '3' code
		}
		if (kbdchar == 'r' || kbdchar == '3' || kbdchar == '#')
		{
			_externs.SetDisplay3D(DISPLAY3D_NONE);
			if (kbdchar == '3' || kbdchar == '#' || kbdchar == IDK_F3)
			{
				_externs.SetDisplay3D(DISPLAY3D_YES);
			}
			if (_externs.ColorPreloaded())
			{
				_g.PushDAC();     // save in case colors= present
			}
			_driver->set_for_text(); // switch to text mode
			_externs.SetShowFile(SHOWFILE_CANCELLED);
			_state = APPSTATE_RESTORE_START;
			return;
		}
		if (kbdchar == 't')  // set fractal type
		{
			_externs.SetJulibrot(false);
			_app.get_fractal_type();
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'x')  // generic toggle switch
		{
			_app.get_toggles();
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'y')  // generic toggle switch
		{
			_app.get_toggles2();
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'z')  // type specific parms
		{
			_app.get_fractal_parameters(true);
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'v')  // view parameters
		{
			_app.get_view_params();
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == IDK_CTL_B)  // ctrl B = browse parms
		{
			_externs.Browse().GetParameters();
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == IDK_CTL_F)  // ctrl f = sound parms
		{
			_externs.Sound().get_parameters();
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'f')  // floating pt toggle
		{
			_externs.SetUserFloatFlag(_externs.UserFloatFlag());
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'i')  // set 3d fractal parms
		{
			_app.get_fractal_3d_parameters(); // get the parameters
			_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'g')
		{
			_app.get_command_string(); // get command string
			_state = APPSTATE_IMAGE_START;
			return;
		}
		// buzzer(2); */                          /* unrecognized key
	}

	_externs.SetZoomOff(true);			// zooming is enabled
	_app.set_help_mode(FIHELP_MAIN);         // now use this help mode
	_resumeFlag = false;  // allows taking goto inside big_while_loop()

	_state = APPSTATE_RESUME_LOOP;
}

void IteratedDynamicsImpl::Restart()
{
	_externs.Browse().Restart();
	_externs.Browse().SetSubImages(true);
	_externs.Browse().SetName("");
	_externs.SetNameStackPtr(-1); // init loaded files stack

	_externs.SetEvolvingFlags(EVOLVE_NONE);
	_externs.SetParameterRangeX(4);
	_externs.SetParameterOffsetX(-2.0);
	_externs.SetNewParameterOffsetX(-2.0);
	_externs.SetParameterRangeY(3);
	_externs.SetParameterOffsetY(-1.5);
	_externs.SetNewParameterOffsetY(-1.5);
	_externs.SetDiscreteParameterOffsetX(0);
	_externs.SetDiscreteParameterOffsetY(0);
	_externs.SetGridSize(9);
	_externs.SetFiddleFactor(1);
	_externs.SetFiddleReduction(1.0);
	_externs.SetThisGenerationRandomSeed(unsigned int(_app.clock_ticks()));
	_app.srand(_externs.ThisGenerationRandomSeed());
	_externs.SetStartShowOrbit(false);
	_externs.SetShowDot(-1); // turn off g_show_dot if entered with <g> command
	_externs.SetCalculationStatus(CALCSTAT_NO_FRACTAL);                    // no active fractal image

	_app.command_files(_argc, _argv);         // process the command-line
	_app.pause_error(PAUSE_ERROR_NO_BATCH); // pause for error msg if not batch
	_app.init_msg("", 0, 0);  // this causes _driver->get_key if init_msg called on runup

	_app.history_allocate();

	if (DEBUGMODE_ABORT_SAVENAME == _externs.DebugMode() && _externs.InitializeBatch() == INITBATCH_NORMAL)   // abort if savename already exists
	{
		_app.check_same_name();
	}
	_driver->window();
	_g.PushDAC();      // save in case colors= present

	_driver->set_for_text();					// switch to text mode
	_g.SetSaveDAC(SAVEDAC_NO);					// don't save the VGA DAC

	_externs.SetColors(256);
	_externs.SetMaxInputCounter(80);			// check the keyboard this often

	if ((_externs.ShowFile() != SHOWFILE_PENDING) && _g.InitialVideoMode() < 0)
	{
		// TODO: refactor to IInputContext
		_app.intro();                          // display the credits screen
		if (_driver->key_pressed() == IDK_ESC)
		{
			_driver->get_key();
			_app.goodbye();
		}
	}

	_externs.Browse().SetBrowsing(false);

	if (!_externs.FunctionPreloaded())
	{
		_app.set_if_old_bif();
	}
	_screenStacked = false;

	_state = APPSTATE_RESTORE_START;
}

void IteratedDynamicsImpl::RestoreStart()
{
	if (_externs.ColorPreloaded())
	{
		_g.PopDAC();   // restore in case colors= present
	}

	_driver->set_mouse_mode(LOOK_MOUSE_NONE);			// ignore mouse

	// image is to be loaded
	while (_externs.ShowFile() == SHOWFILE_PENDING || _externs.ShowFile() == SHOWFILE_CANCELLED)
	{
		char *hdg;
		_externs.SetTabDisplayEnabled(false);
		if (!_externs.Browse().Browsing())     /*RB*/
		{
			if (_externs.Overlay3D())
			{
				hdg = "Select File for 3D Overlay";
				_app.set_help_mode(FIHELP_3D_OVERLAY);
			}
			else if (_externs.Display3D())
			{
				hdg = "Select File for 3D Transform";
				_app.set_help_mode(FIHELP_3D_IMAGES);
			}
			else
			{
				hdg = "Select File to Restore";
				_app.set_help_mode(FIHELP_SAVE_RESTORE);
			}
			if (_externs.ShowFile() == SHOWFILE_CANCELLED
				&& _app.get_a_filename(hdg, _externs.GIFMask(), _externs.ReadName()) < 0)
			{
				_externs.SetShowFile(SHOWFILE_DONE);               // cancelled
				_g.SetInitialVideoModeNone();
				break;
			}

			_externs.SetNameStackPtr(0); // 'r' reads first filename for browsing
			_externs.SetFileNameStackTop(_externs.Browse().Name());
		}

		_externs.SetEvolvingFlags(EVOLVE_NONE);
		_externs.View().Hide();
		_externs.SetShowFile(SHOWFILE_DONE);
		_app.set_help_mode(-1);
		_externs.SetTabDisplayEnabled(true);
		if (_screenStacked)
		{
			_driver->discard_screen();
			_driver->set_for_text();
			_screenStacked = false;
		}
		if (_app.read_overlay() == 0)       // read hdr, get video mode
		{
			break;                      // got it, exit
		}
		_externs.SetShowFile(_externs.Browse().Browsing() ? SHOWFILE_DONE : SHOWFILE_CANCELLED);
	}

	_app.set_help_mode(FIHELP_MENU);                 // now use this help mode
	_externs.SetTabDisplayEnabled(true);
	_driver->set_mouse_mode(LOOK_MOUSE_NONE);                     // ignore mouse

	if (((_externs.Overlay3D() && !_externs.InitializeBatch()) || _screenStacked) && _g.InitialVideoMode() < 0)        // overlay command failed
	{
		_driver->unstack_screen();                  // restore the graphics screen
		_screenStacked = false;
		_externs.SetOverlay3D(0);					// forget overlays
		_externs.SetDisplay3D(DISPLAY3D_NONE);
		if (_externs.CalculationStatus() == CALCSTAT_NON_RESUMABLE)
		{
			_externs.SetCalculationStatus(CALCSTAT_PARAMS_CHANGED);
		}
		_resumeFlag = true;
		_state = APPSTATE_RESUME_LOOP;
	}

	_g.SetSaveDAC(SAVEDAC_NO);                         // don't save the VGA DAC
	_state = APPSTATE_IMAGE_START;
}

