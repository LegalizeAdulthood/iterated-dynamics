#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "ant.h"
#include "Browse.h"
#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "encoder.h"
#include "evolve.h"
#include "idhelp.h"
#include "filesystem.h"
#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "history.h"
#include "jiim.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "miscovl.h"
#include "miscfrac.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "rotate.h"
#include "stereo.h"
#include "StopMessage.h"
#include "zoom.h"
#include "ZoomBox.h"

#include "BigWhileLoop.h"
#include "EscapeTime.h"
#include "CommandParser.h"
#include "FrothyBasin.h"
#include "ViewWindow.h"
#include "Externals.h"
#include "BigWhileLoopImpl.h"

ApplicationStateType BigWhileLoopImpl::GetMainMenuState(bool julia_entered_from_mandelbrot, int kbdchar)
{
	ApplicationStateType mainMenuState = _externs.EvolvingFlags() ?
		_app->evolver_menu_switch(kbdchar, julia_entered_from_mandelbrot, _keyboardMore, _screenStacked)
		: _app->main_menu_switch(kbdchar, julia_entered_from_mandelbrot, _keyboardMore, _screenStacked);
	if (_externs.QuickCalculate()
		&& (mainMenuState == APPSTATE_IMAGE_START ||
		mainMenuState == APPSTATE_RESTORE_START ||
		mainMenuState == APPSTATE_RESTART))
	{
		_externs.SetQuickCalculate(false);
		_externs.SetUserStandardCalculationMode(_data.StandardCalculationModeOld());
	}
	if (_externs.QuickCalculate() && _externs.CalculationStatus() != CALCSTAT_COMPLETED)
	{
		_externs.SetUserStandardCalculationMode(CALCMODE_SINGLE_PASS);
	}
	return mainMenuState;
}

bool BigWhileLoopImpl::StatusNotResumableOrShowFilePending()
{
	return _externs.CalculationStatus() != CALCSTAT_RESUMABLE || _externs.ShowFile() == SHOWFILE_PENDING;
}

void BigWhileLoopImpl::HandleVisibleViewWindow()
{
	double ftemp;
	if (_data.GetViewWindow().Visible())
	{
		// bypass for VESA virtual screen
		ftemp = _data.GetViewWindow().AspectRatio()*((double(_externs.ScreenHeight()))/(double(_externs.ScreenWidth()))/_externs.ScreenAspectRatio());
		_externs.SetXDots(_data.GetViewWindow().Width());
		if (_externs.XDots() != 0)
		{	// _externs.XDots specified
			_externs.SetYDots(_data.GetViewWindow().Height());
			if (_externs.YDots() == 0) // calc _externs.YDots?
			{
				_externs.SetYDots(int(double(_externs.XDots())*ftemp + 0.5));
			}
		}
		else if (_data.GetViewWindow().AspectRatio() <= _externs.ScreenAspectRatio())
		{
			_externs.SetXDots(int(double(_externs.ScreenWidth())/_data.GetViewWindow().Reduction() + 0.5));
			_externs.SetYDots(int(double(_externs.XDots())*ftemp + 0.5));
		}
		else
		{
			_externs.SetYDots(int(double(_externs.ScreenHeight())/_data.GetViewWindow().Reduction() + 0.5));
			_externs.SetXDots(int(double(_externs.YDots())/ftemp + 0.5));
		}
		if (_externs.XDots() > _externs.ScreenWidth() || _externs.YDots() > _externs.ScreenHeight())
		{
			_app->stop_message(STOPMSG_NORMAL, "View window too large; using full screen.");
			_data.GetViewWindow().FullScreen(_externs.ScreenWidth(), _externs.ScreenHeight());
			_externs.SetXDots(_externs.ScreenWidth());
			_externs.SetYDots(_externs.ScreenHeight());
		}
		else if (((_externs.XDots() <= 1) // changed test to 1, so a 2x2 window will
			|| (_externs.YDots() <= 1)) // work with the sound feature
			&& !(_externs.EvolvingFlags() & EVOLVE_FIELD_MAP))
		{	// so ssg works
			// but no check if in evolve mode to allow lots of small views
			_app->stop_message(STOPMSG_NORMAL, "View window too small; using full screen.");
			_data.GetViewWindow().Hide();
			_externs.SetXDots(_externs.ScreenWidth());
			_externs.SetYDots(_externs.ScreenHeight());
		}
		if ((_externs.EvolvingFlags() & EVOLVE_FIELD_MAP) && (_data.CurrentFractalSpecificFlags() & FRACTALFLAG_INFINITE_CALCULATION))
		{
			_app->stop_message(STOPMSG_NORMAL, "Fractal doesn't terminate! switching off evolution.");
			_externs.SetEvolvingFlags(_externs.EvolvingFlags() & ~EVOLVE_FIELD_MAP);
			_data.GetViewWindow().Hide();
			_externs.SetXDots(_externs.ScreenWidth());
			_externs.SetYDots(_externs.ScreenHeight());
		}
		if (_externs.EvolvingFlags() & EVOLVE_FIELD_MAP)
		{
			_externs.SetXDots((_externs.ScreenWidth()/_externs.GridSize()) - !((_externs.EvolvingFlags() & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT));
			_externs.SetXDots(_externs.XDots() - _externs.XDots() % 4); // trim to multiple of 4 for SSG
			_externs.SetYDots((_externs.ScreenHeight()/_externs.GridSize()) - !((_externs.EvolvingFlags() & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT));
			_externs.SetYDots(_externs.YDots() - _externs.YDots() % 4);
		}
		else
		{
			_data.SetScreenOffset((_externs.ScreenWidth() - _externs.XDots())/2,
				(_externs.ScreenHeight() - _externs.YDots())/3);
		}
	}
}

ApplicationStateType BigWhileLoopImpl::Execute()
{
#if defined(_WIN32)
	_ASSERTE(_CrtCheckMemory());
#endif
	bool julia_entered_from_mandelbrot = false;
	if (_resumeFlag)
	{
		goto resumeloop;
	}

	int i = 0;
	while (true)
	{
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		if (StatusNotResumableOrShowFilePending())
		{
			_g.SetVideoEntry(_g.Adapter());
			_externs.SetXDots(_g.VideoEntry().x_dots);       // # dots across the screen
			_externs.SetYDots(_g.VideoEntry().y_dots);       // # dots down the screen
			_externs.SetColors(_g.VideoEntry().colors);      // # colors available
			_externs.SetScreenWidth(_externs.XDots());
			_externs.SetScreenHeight(_externs.YDots());
			_data.SetScreenOffset(0, 0);
			_data.SetRotateHigh((_data.RotateHigh() < _externs.Colors())
				? _data.RotateHigh() : _externs.Colors() - 1);

			_g.PushDAC();

			if (_externs.Overlay3D() && !_externs.InitializeBatch())
			{
				_driver->unstack_screen();            // restore old graphics image
				_externs.SetOverlay3D(false);
			}
			else
			{
				_app->change_video_mode(_g.VideoEntry()); // switch video modes
				// switching video modes may have changed drivers or disk flag...
				if (!_g.GoodMode())
				{
					if (!_driver->diskp())
					{
						_app->stop_message(STOPMSG_NORMAL, "That video mode is not available with your adapter.");
					}
					_externs.UiState().ask_video = true;
					_g.SetInitialVideoModeNone();
					_driver->set_for_text(); // switch to text mode
					// goto restorestart;
					return APPSTATE_RESTORE_START;
				}

				_externs.SetXDots(_externs.ScreenWidth());
				_externs.SetYDots(_externs.ScreenHeight());
				_g.SetVideoEntrySize(_externs.XDots(), _externs.YDots());
			}

			if (_g.SaveDAC() || _externs.ColorPreloaded())
			{
				_g.PopDAC(); // restore the DAC
				_app->load_dac();
				_externs.SetColorPreloaded(false);
			}
			else
			{	// reset DAC to defaults, which setvideomode has done for us
				if (_g.MapDAC())
				{	// but there's a map=, so load that
					_g.DAC() = *_g.MapDAC();
					_app->load_dac();
				}
				_g.SetColorState(COLORSTATE_DEFAULT);
			}
			HandleVisibleViewWindow();
			_externs.SetDxSize(_externs.XDots() - 1);				// convert just once now
			_externs.SetDySize(_externs.YDots() - 1);
		}
		// assume we save next time (except jb)
		_g.SetSaveDAC((_g.SaveDAC() == SAVEDAC_NO) ? SAVEDAC_NEXT : SAVEDAC_YES);
		if (_externs.InitializeBatch() == INITBATCH_NONE)
		{
			_driver->set_mouse_mode(-IDK_PAGE_UP);			// mouse left button == pgup
		}

		if (_externs.ShowFile() == SHOWFILE_PENDING)
		{               // loading an image
			_externs.SetOutLineCleanup(_externs.OutLineCleanupNull());	// g_out_line routine can set this
			if (_externs.Display3D())							// set up 3D decoding
			{
				_externs.SetOutLine(_externs.OutLine3D());
			}
			else if (_data.CompareGIF())
			{
				_externs.SetOutLine(_externs.OutLineCompare());
			}
			else if (_externs.Potential16Bit())
			{            // .pot format input file
				if (_app->disk_start_potential() < 0)
				{                           // pot file failed?
					_externs.SetShowFile(SHOWFILE_DONE);
					_externs.SetPotentialFlag(false);
					_externs.SetPotential16Bit(false);
					_g.SetInitialVideoModeNone();
					_externs.SetCalculationStatus(CALCSTAT_RESUMABLE);         // "resume" without 16-bit
					_driver->set_for_text();
					_app->get_fractal_type();
					// goto imagestart;
					return APPSTATE_IMAGE_START;
				}
				_externs.SetOutLine(_externs.OutLinePotential());
			}
			else if ((_externs.Sound().flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP && !_externs.EvolvingFlags()) // regular gif/fra input file
			{
				_externs.SetOutLine(_externs.OutLineSound());
			}
			else
			{
				_externs.SetOutLine(_externs.OutLineRegular());
			}
			if (DEBUGMODE_2224 == _externs.DebugMode())
			{
				_app->stop_message(STOPMSG_NO_BUZZER,
					str(boost::format("floatflag=%d") % (_externs.UserFloatFlag() ? 1 : 0)));
			}
			i = _app->funny_glasses_call(_externs.GIFView());
			_externs.OutLineCleanup();
			if (i == 0)
			{
				_driver->buzzer(BUZZER_COMPLETE);
			}
			else
			{
				_externs.SetCalculationStatus(CALCSTAT_NO_FRACTAL);
				// TODO: don't support aborting of load
				if (_driver->key_pressed())
				{
					_driver->buzzer(BUZZER_INTERRUPT);
					while (_driver->key_pressed())
					{
						_driver->get_key();
					}
					_app->text_temp_message("*** load incomplete ***");
				}
			}
		}

		_externs.SetZoomOff(true);                      // zooming is enabled
		if (_driver->diskp() || (_data.CurrentFractalSpecificFlags() & FRACTALFLAG_NO_ZOOM) != 0)
		{
			_externs.SetZoomOff(false);;                   // for these cases disable zooming
		}
		if (!_externs.EvolvingFlags())
		{
			_app->calculate_fractal_initialize();
		}
		_driver->schedule_alarm(1);

		_externs.SetSxMin(_externs.EscapeTime().m_grid_fp.x_min()); // save 3 corners for zoom.c ref points
		_externs.SetSxMax(_externs.EscapeTime().m_grid_fp.x_max());
		_externs.SetSx3rd(_externs.EscapeTime().m_grid_fp.x_3rd());
		_externs.SetSyMin(_externs.EscapeTime().m_grid_fp.y_min());
		_externs.SetSyMax(_externs.EscapeTime().m_grid_fp.y_max());
		_externs.SetSy3rd(_externs.EscapeTime().m_grid_fp.y_3rd());

		if (_externs.BfMath())
		{
			_app->copy_bf(_externs.SxMinBf(), _externs.EscapeTime().m_grid_bf.x_min());
			_app->copy_bf(_externs.SxMaxBf(), _externs.EscapeTime().m_grid_bf.x_max());
			_app->copy_bf(_externs.SyMinBf(), _externs.EscapeTime().m_grid_bf.y_min());
			_app->copy_bf(_externs.SyMaxBf(), _externs.EscapeTime().m_grid_bf.y_max());
			_app->copy_bf(_externs.Sx3rdBf(), _externs.EscapeTime().m_grid_bf.x_3rd());
			_app->copy_bf(_externs.Sy3rdBf(), _externs.EscapeTime().m_grid_bf.y_3rd());
		}
		_app->history_save_info();

		if (_externs.ShowFile() == SHOWFILE_PENDING)
		{               // image has been loaded
			_externs.SetShowFile(SHOWFILE_DONE);
			if (_externs.InitializeBatch() == INITBATCH_NORMAL && _externs.CalculationStatus() == CALCSTAT_RESUMABLE)
			{
				_externs.SetInitializeBatch(INITBATCH_FINISH_CALC); // flag to finish calc before save
			}
			if (_externs.Loaded3D())      // 'r' of image created with '3'
			{
				_externs.SetDisplay3D(DISPLAY3D_YES);  // so set flag for 'b' command
			}
		}
		else
		{                            // draw an image
			if (_externs.SaveTime() != 0          // autosave and resumable?
				&& (_data.CurrentFractalSpecificFlags() & FRACTALFLAG_NOT_RESUMABLE) == 0)
			{
				_externs.SetSaveBase(_app->read_ticker()); // calc's start time
				_externs.SetSaveTicks(_externs.SaveTime()*60*1000); // in milliseconds
				_externs.SetFinishRow(-1);
			}
			_externs.Browse().SetBrowsing(false);      // regenerate image, turn off browsing
			_data.SetNameStackPointer(-1);
			_externs.Browse().SetName("");
			if (_data.GetViewWindow().Visible() && (_externs.EvolvingFlags() & EVOLVE_FIELD_MAP) && (_externs.CalculationStatus() != CALCSTAT_COMPLETED))
			{
				// generate a set of images with varied parameters on each one
				int grout;
				int ecount;
				int tmpxdots;
				int tmpydots;
				int gridsqr;
				evolution_info resume_e_info;

				if ((_externs.Evolve() != 0) && (_externs.CalculationStatus() == CALCSTAT_RESUMABLE))
				{
					resume_e_info = *_externs.Evolve();
					_externs.SetParameterRangeX(resume_e_info.parameter_range_x);
					_externs.SetParameterRangeY(resume_e_info.parameter_range_y);
					_externs.SetParameterOffsetX(resume_e_info.opx);
					_externs.SetParameterOffsetY(resume_e_info.opy);
					_externs.SetNewParameterOffsetX(resume_e_info.opx);
					_externs.SetNewParameterOffsetY(resume_e_info.opy);
					_externs.SetNewDiscreteParameterOffsetX(resume_e_info.odpx);
					_externs.SetNewDiscreteParameterOffsetY(resume_e_info.odpy);
					_externs.SetDiscreteParameterOffsetX(_externs.NewDiscreteParameterOffsetX());
					_externs.SetDiscreteParameterOffsetY(_externs.NewDiscreteParameterOffsetY());
					_externs.SetPx(resume_e_info.px);
					_externs.SetPy(resume_e_info.py);
					_data.SetScreenOffset(resume_e_info.screen_x_offset, resume_e_info.screen_y_offset);
					_externs.SetXDots(resume_e_info.x_dots);
					_externs.SetYDots(resume_e_info.y_dots);
					_externs.SetGridSize(resume_e_info.grid_size);
					_externs.SetThisGenerationRandomSeed(resume_e_info.this_generation_random_seed);
					_externs.SetFiddleFactor(resume_e_info.fiddle_factor);
					_externs.SetEvolvingFlags(resume_e_info.evolving);
					if (_externs.EvolvingFlags())
					{
						_data.GetViewWindow().Show();
					}
					ecount = resume_e_info.ecount;
					delete _externs.Evolve();
					_externs.SetEvolve(0);
				}
				else
				{ // not resuming, start from the beginning
					int mid = _externs.GridSize()/2;
					if ((_externs.Px() != mid) || (_externs.Py() != mid))
					{
						_externs.SetThisGenerationRandomSeed((unsigned int) _app->clock_ticks()); // time for new set
					}
					_app->save_parameter_history();
					ecount = 0;
					_externs.SetFiddleFactor(_externs.FiddleFactor()*_externs.FiddleReduction());
					_externs.SetParameterOffsetX(_externs.NewParameterOffsetX());
					_externs.SetParameterOffsetY(_externs.NewParameterOffsetY());
					// odpx used for discrete parms like inside, outside, trigfn etc
					_externs.SetDiscreteParameterOffsetX(_externs.NewDiscreteParameterOffsetX());
					_externs.SetDiscreteParameterOffsetY(_externs.NewDiscreteParameterOffsetY());
				}
				_externs.SetParameterBoxCount(0);
				_externs.SetDeltaParameterImageX(_externs.ParameterRangeX()/(_externs.GridSize() - 1));
				_externs.SetDeltaParameterImageY(_externs.ParameterRangeY()/(_externs.GridSize() - 1));
				grout = !((_externs.EvolvingFlags() & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
				tmpxdots = _externs.XDots() + grout;
				tmpydots = _externs.YDots() + grout;
				gridsqr = _externs.GridSize()*_externs.GridSize();
				while (ecount < gridsqr)
				{
					_app->spiral_map(ecount); // sets px & py
					_data.SetScreenOffset(tmpxdots*_externs.Px(), tmpydots*_externs.Py());
					_app->restore_parameter_history();
					_app->fiddle_parameters(_externs.Genes(), ecount);
					_app->calculate_fractal_initialize();
					if (_app->calculate_fractal() == -1)
					{
						goto done;
					}
					ecount ++;
				}
done:
#if defined(_WIN32)
				_ASSERTE(_CrtCheckMemory());
#endif

				if (ecount == gridsqr)
				{
					i = 0;
					_driver->buzzer(BUZZER_COMPLETE); // finished!!
				}
				else
				{	// interrupted screen generation, save info
					if (_externs.Evolve() == 0)
					{
						_externs.SetEvolve(new evolution_info);
					}
					resume_e_info.parameter_range_x = _externs.ParameterRangeX();
					resume_e_info.parameter_range_y = _externs.ParameterRangeY();
					resume_e_info.opx = _externs.ParameterOffsetX();
					resume_e_info.opy = _externs.ParameterOffsetY();
					resume_e_info.odpx = short(_externs.DiscreteParameterOffsetX());
					resume_e_info.odpy = short(_externs.DiscreteParameterOffsetY());
					resume_e_info.px = short(_externs.Px());
					resume_e_info.py = short(_externs.Py());
					resume_e_info.screen_x_offset = short(_externs.ScreenXOffset());
					resume_e_info.screen_y_offset = short(_externs.ScreenYOffset());
					resume_e_info.x_dots = short(_externs.XDots());
					resume_e_info.y_dots = short(_externs.YDots());
					resume_e_info.grid_size = short(_externs.GridSize());
					resume_e_info.this_generation_random_seed = short(_externs.ThisGenerationRandomSeed());
					resume_e_info.fiddle_factor = _externs.FiddleFactor();
					resume_e_info.evolving = short(_externs.EvolvingFlags());
					resume_e_info.ecount = short(ecount);
					*_externs.Evolve() = resume_e_info;
				}
				_data.SetScreenOffset(0, 0);
				_externs.SetXDots(_externs.ScreenWidth());
				_externs.SetYDots(_externs.ScreenHeight()); // otherwise save only saves a sub image and boxes get clipped

				// set up for 1st selected image, this reuses px and py
				_externs.SetPx(_externs.GridSize()/2);
				_externs.SetPy(_externs.GridSize()/2);
				_app->unspiral_map(); // first time called, w/above line sets up array
				_app->restore_parameter_history();
				_app->fiddle_parameters(_externs.Genes(), 0);
			}
			// end of evolution loop
			else
			{
				i = _app->calculate_fractal();       // draw the fractal
				if (i == 0)
				{
					_driver->buzzer(BUZZER_COMPLETE); // finished!!
				}
			}

			_externs.SetSaveTicks(0);                 // turn off autosave timer
			if (_driver->diskp() && i == 0) // disk-video
			{
				_app->disk_video_status(0, "Image has been completed");
			}
		}
		_externs.Zoom().set_count(0);                     // no zoom box yet
		_externs.SetZWidth(0);

		if (_externs.FractalType() == FRACTYPE_PLASMA)
		{
			_externs.SetCycleLimit(256);              // plasma clouds need quick spins
			_g.SetDACSleepCount(256);
		}

resumeloop:
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		_keyboardMore = true;
		int kbdchar;
		while (_keyboardMore)
		{           // loop through command keys
			if (_externs.TimedSave() != TIMEDSAVE_DONE)
			{
				if (_externs.TimedSave() == TIMEDSAVE_START)
				{       // woke up for timed save
					_driver->get_key();     // eat the dummy char
					kbdchar = 's'; // do the save
					_externs.SetResaveMode(RESAVE_YES);
					_externs.SetTimedSave(TIMEDSAVE_PENDING);
				}
				else
				{                      // save done, resume
					_externs.SetTimedSave(TIMEDSAVE_DONE);
					_externs.SetResaveMode(RESAVE_DONE);
					kbdchar = IDK_ENTER;
				}
			}
			else if (_externs.InitializeBatch() == INITBATCH_NONE)      // not batch mode
			{
				_driver->set_mouse_mode((_externs.ZWidth() == 0) ? -IDK_PAGE_UP : LOOK_MOUSE_ZOOM_BOX);
				if (_externs.CalculationStatus() == CALCSTAT_RESUMABLE && _externs.ZWidth() == 0 && !_driver->key_pressed())
				{
					kbdchar = IDK_ENTER;  // no visible reason to stop, continue
				}
				else      // wait for a real keystroke
				{
					if (_externs.Browse().AutoBrowse() && _externs.Browse().SubImages())
					{
						kbdchar = 'l';
					}
					else
					{
						_driver->wait_key_pressed(0);
						kbdchar = _driver->get_key();
					}
					if (kbdchar == IDK_ESC || kbdchar == 'm' || kbdchar == 'M')
					{
						if (kbdchar == IDK_ESC && _externs.EscapeExitFlag())
						{
							// don't ask, just get out
							_app->goodbye();
						}
						_driver->stack_screen();
						kbdchar = _app->main_menu(true);
						if (kbdchar == '\\' || kbdchar == IDK_CTL_BACKSLASH ||
							kbdchar == 'h' || kbdchar == IDK_BACKSPACE ||
							_app->check_video_mode_key(kbdchar) >= 0)
						{
							_driver->discard_screen();
						}
						else if (kbdchar == 'x' || kbdchar == 'y' ||
								kbdchar == 'z' || kbdchar == 'g' ||
								kbdchar == 'v' || kbdchar == IDK_CTL_B ||
								kbdchar == IDK_CTL_E || kbdchar == IDK_CTL_F)
						{
							_externs.SetFromTextFlag(true);
						}
						else
						{
							_driver->unstack_screen();
						}
					}
				}
			}
			else          // batch mode, fake next keystroke
			{
				// _externs.InitializeBatch == -1  flag to finish calc before save
				// _externs.InitializeBatch == 0   not in batch mode
				// _externs.InitializeBatch == 1   normal batch mode
				// _externs.InitializeBatch == 2   was 1, now do a save
				// _externs.InitializeBatch == 3   bailout with errorlevel == 2, error occurred, no save
				// _externs.InitializeBatch == 4   bailout with errorlevel == 1, interrupted, try to save
				// _externs.InitializeBatch == 5   was 4, now do a save

				if (_externs.InitializeBatch() == INITBATCH_FINISH_CALC)       // finish calc
				{
					kbdchar = IDK_ENTER;
					_externs.SetInitializeBatch(INITBATCH_NORMAL);
				}
				else if (_externs.InitializeBatch() == INITBATCH_NORMAL || _externs.InitializeBatch() == INITBATCH_BAILOUT_INTERRUPTED) // save-to-disk
				{
					kbdchar = (DEBUGMODE_COMPARE_RESTORED == _externs.DebugMode()) ? 'r' : 's';
					if (_externs.InitializeBatch() == INITBATCH_NORMAL)
					{
						_externs.SetInitializeBatch(INITBATCH_SAVE);
					}
					if (_externs.InitializeBatch() == INITBATCH_BAILOUT_INTERRUPTED)
					{
						_externs.SetInitializeBatch(INITBATCH_BAILOUT_SAVE);
					}
				}
				else
				{
					if (_externs.CalculationStatus() != CALCSTAT_COMPLETED)
					{
						_externs.SetInitializeBatch(INITBATCH_BAILOUT_ERROR); // bailout with error
					}
					_app->goodbye();               // done, exit
				}
			}

			if ('A' <= kbdchar && kbdchar <= 'Z')
			{
				kbdchar = tolower(kbdchar);
			}

			ApplicationStateType mainMenuState = GetMainMenuState(julia_entered_from_mandelbrot, kbdchar);
			switch (mainMenuState)
			{
			case APPSTATE_IMAGE_START:		return APPSTATE_IMAGE_START;
			case APPSTATE_RESTORE_START:	return APPSTATE_RESTORE_START;
			case APPSTATE_RESTART:			return APPSTATE_RESTART;
			case APPSTATE_CONTINUE:			continue;
			default:						break;
			}
			if (_externs.ZoomOff() && _keyboardMore) // draw/clear a zoom box?
			{
				_app->zoom_box_draw(true);
			}
			if (_driver->resize())
			{
				_externs.SetCalculationStatus(CALCSTAT_NO_FRACTAL);
			}
		}
	}
}
