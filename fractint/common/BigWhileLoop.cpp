#include <fstream>
#include <string>

#include <string.h>
#include <time.h>
#include <ctype.h>

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
#include "fihelp.h"
#include "filesystem.h"
#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "gifview.h"
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
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"
#include "zoom.h"
#include "ZoomBox.h"

#include "BigWhileLoop.h"
#include "EscapeTime.h"
#include "CommandParser.h"
#include "FrothyBasin.h"
#include "SoundState.h"
#include "ViewWindow.h"

extern int out_line_compare(BYTE *pixels, int line_length);

ApplicationStateType BigWhileLoop::GetMainMenuState(bool julia_entered_from_mandelbrot, int kbdchar)
{
	ApplicationStateType mainMenuState = _data.EvolvingFlags() ?
		_app->evolver_menu_switch(kbdchar, julia_entered_from_mandelbrot, _keyboardMore, _screenStacked)
		: _app->main_menu_switch(kbdchar, julia_entered_from_mandelbrot, _keyboardMore, _screenStacked);
	if (_data.QuickCalculate()
		&& (mainMenuState == APPSTATE_IMAGE_START ||
		mainMenuState == APPSTATE_RESTORE_START ||
		mainMenuState == APPSTATE_RESTART))
	{
		_data.SetQuickCalculate(false);
		_data.SetUserStandardCalculationMode(_data.StandardCalculationModeOld());
	}
	if (_data.QuickCalculate() && _data.CalculationStatus() != CALCSTAT_COMPLETED)
	{
		_data.SetUserStandardCalculationMode(CALCMODE_SINGLE_PASS);
	}
	return mainMenuState;
}

bool BigWhileLoop::StatusNotResumableOrShowFilePending()
{
	return _data.CalculationStatus() != CALCSTAT_RESUMABLE || _data.ShowFile() == SHOWFILE_PENDING;
}

void BigWhileLoop::HandleVisibleViewWindow()
{
	double ftemp;
	if (_data.GetViewWindow().Visible())
	{
		// bypass for VESA virtual screen
		ftemp = _data.GetViewWindow().AspectRatio()*((double(_data.ScreenHeight()))/(double(_data.ScreenWidth()))/_data.ScreenAspectRatio());
		_data.SetXDots(_data.GetViewWindow().Width());
		if (_data.XDots() != 0)
		{	// g_x_dots specified
			_data.SetYDots(_data.GetViewWindow().Height());
			if (_data.YDots() == 0) // calc g_y_dots?
			{
				_data.SetYDots(int(double(_data.XDots())*ftemp + 0.5));
			}
		}
		else if (_data.GetViewWindow().AspectRatio() <= _data.ScreenAspectRatio())
		{
			_data.SetXDots(int(double(_data.ScreenWidth())/_data.GetViewWindow().Reduction() + 0.5));
			_data.SetYDots(int(double(_data.XDots())*ftemp + 0.5));
		}
		else
		{
			_data.SetYDots(int(double(_data.ScreenHeight())/_data.GetViewWindow().Reduction() + 0.5));
			_data.SetXDots(int(double(_data.YDots())/ftemp + 0.5));
		}
		if (_data.XDots() > _data.ScreenWidth() || _data.YDots() > _data.ScreenHeight())
		{
			_app->stop_message(STOPMSG_NORMAL, "View window too large; using full screen.");
			_data.GetViewWindow().FullScreen(_data.ScreenWidth(), _data.ScreenHeight());
			_data.SetXDots(_data.ScreenWidth());
			_data.SetYDots(_data.ScreenHeight());
		}
		else if (((_data.XDots() <= 1) // changed test to 1, so a 2x2 window will
			|| (_data.YDots() <= 1)) // work with the sound feature
			&& !(_data.EvolvingFlags() & EVOLVE_FIELD_MAP))
		{	// so ssg works
			// but no check if in evolve mode to allow lots of small views
			_app->stop_message(STOPMSG_NORMAL, "View window too small; using full screen.");
			_data.GetViewWindow().Hide();
			_data.SetXDots(_data.ScreenWidth());
			_data.SetYDots(_data.ScreenHeight());
		}
		if ((_data.EvolvingFlags() & EVOLVE_FIELD_MAP) && (_data.CurrentFractalSpecificFlags() & FRACTALFLAG_INFINITE_CALCULATION))
		{
			_app->stop_message(STOPMSG_NORMAL, "Fractal doesn't terminate! switching off evolution.");
			_data.SetEvolvingFlags(_data.EvolvingFlags() & ~EVOLVE_FIELD_MAP);
			_data.GetViewWindow().Hide();
			_data.SetXDots(_data.ScreenWidth());
			_data.SetYDots(_data.ScreenHeight());
		}
		if (_data.EvolvingFlags() & EVOLVE_FIELD_MAP)
		{
			_data.SetXDots((_data.ScreenWidth()/g_grid_size) - !((_data.EvolvingFlags() & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT));
			_data.SetXDots(_data.XDots() - _data.XDots() % 4); // trim to multiple of 4 for SSG
			_data.SetYDots((_data.ScreenHeight()/g_grid_size) - !((_data.EvolvingFlags() & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT));
			_data.SetYDots(_data.YDots() - _data.YDots() % 4);
		}
		else
		{
			_data.SetScreenOffset((_data.ScreenWidth() - _data.XDots())/2,
				(_data.ScreenHeight() - _data.YDots())/3);
		}
	}
}

ApplicationStateType BigWhileLoop::Execute()
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
			_data.SetXDots(_g.VideoEntry().x_dots);       // # dots across the screen 
			_data.SetYDots(_g.VideoEntry().y_dots);       // # dots down the screen   
			_data.SetColors(_g.VideoEntry().colors);      // # colors available 
			_data.SetScreenWidth(_data.XDots());
			_data.SetScreenHeight(_data.YDots());
			_data.SetScreenOffset(0, 0);
			_data.SetRotateHigh((_data.RotateHigh() < _data.Colors())
				? _data.RotateHigh() : _data.Colors() - 1);

			_g.PushDAC();

			if (_data.Overlay3D() && !_data.InitializeBatch())
			{
				_driver->unstack_screen();            // restore old graphics image 
				_data.SetOverlay3D(false);
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
					g_ui_state.ask_video = true;
					_g.SetInitialVideoModeNone();
					_driver->set_for_text(); // switch to text mode 
					// goto restorestart; 
					return APPSTATE_RESTORE_START;
				}

				_data.SetXDots(_data.ScreenWidth());
				_data.SetYDots(_data.ScreenHeight());
				_g.SetVideoEntrySize(_data.XDots(), _data.YDots());
			}

			if (_g.SaveDAC() || _data.ColorPreloaded())
			{
				_g.PopDAC(); // restore the DAC 
				_app->load_dac();
				_data.SetColorPreloaded(false);
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
			_data.SetDXSize(_data.XDots() - 1);				// convert just once now 
			_data.SetDYSize(_data.YDots() - 1);
		}
		// assume we save next time (except jb) 
		_g.SetSaveDAC((_g.SaveDAC() == SAVEDAC_NO) ? SAVEDAC_NEXT : SAVEDAC_YES);
		if (_data.InitializeBatch() == INITBATCH_NONE)
		{
			_driver->set_mouse_mode(-IDK_PAGE_UP);			// mouse left button == pgup 
		}

		if (_data.ShowFile() == SHOWFILE_PENDING)
		{               // loading an image 
			_data.SetOutLineCleanup(out_line_cleanup_null);	// g_out_line routine can set this 
			if (_data.Display3D())							// set up 3D decoding 
			{
				_data.SetOutLine(out_line_3d);
			}
			else if (_data.CompareGIF())
			{
				_data.SetOutLine(out_line_compare);
			}
			else if (_data.Potential16Bit())
			{            // .pot format input file 
				if (_app->disk_start_potential() < 0)
				{                           // pot file failed?  
					_data.SetShowFile(SHOWFILE_DONE);
					_data.SetPotentialFlag(false);
					_data.SetPotential16Bit(false);
					_g.SetInitialVideoModeNone();
					_data.SetCalculationStatus(CALCSTAT_RESUMABLE);         // "resume" without 16-bit 
					_driver->set_for_text();
					_app->get_fractal_type();
					// goto imagestart; 
					return APPSTATE_IMAGE_START;
				}
				_data.SetOutLine(out_line_potential);
			}
			else if ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP && !_data.EvolvingFlags()) // regular gif/fra input file 
			{
				_data.SetOutLine(out_line_sound);      // sound decoding 
			}
			else
			{
				_data.SetOutLine(out_line);        // regular decoding 
			}
			if (DEBUGMODE_2224 == _data.DebugMode())
			{
				_app->stop_message(STOPMSG_NO_BUZZER,
					str(boost::format("floatflag=%d") % (g_user_float_flag ? 1 : 0)));
			}
			i = _app->funny_glasses_call(gifview);
			_data.OutLineCleanup();
			if (i == 0)
			{
				_driver->buzzer(BUZZER_COMPLETE);
			}
			else
			{
				_data.SetCalculationStatus(CALCSTAT_NO_FRACTAL);
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

		_data.SetZoomOff(true);                      // zooming is enabled 
		if (_driver->diskp() || (_data.CurrentFractalSpecificFlags() & FRACTALFLAG_NO_ZOOM) != 0)
		{
			_data.SetZoomOff(false);;                   // for these cases disable zooming 
		}
		if (!_data.EvolvingFlags())
		{
			_app->calculate_fractal_initialize();
		}
		_driver->schedule_alarm(1);

		g_sx_min = g_escape_time_state.m_grid_fp.x_min(); // save 3 corners for zoom.c ref points 
		g_sx_max = g_escape_time_state.m_grid_fp.x_max();
		g_sx_3rd = g_escape_time_state.m_grid_fp.x_3rd();
		g_sy_min = g_escape_time_state.m_grid_fp.y_min();
		g_sy_max = g_escape_time_state.m_grid_fp.y_max();
		g_sy_3rd = g_escape_time_state.m_grid_fp.y_3rd();

		if (g_bf_math)
		{
			copy_bf(g_sx_min_bf, g_escape_time_state.m_grid_bf.x_min());
			copy_bf(g_sx_max_bf, g_escape_time_state.m_grid_bf.x_max());
			copy_bf(g_sy_min_bf, g_escape_time_state.m_grid_bf.y_min());
			copy_bf(g_sy_max_bf, g_escape_time_state.m_grid_bf.y_max());
			copy_bf(g_sx_3rd_bf, g_escape_time_state.m_grid_bf.x_3rd());
			copy_bf(g_sy_3rd_bf, g_escape_time_state.m_grid_bf.y_3rd());
		}
		_app->history_save_info();

		if (_data.ShowFile() == SHOWFILE_PENDING)
		{               // image has been loaded 
			_data.SetShowFile(SHOWFILE_DONE);
			if (_data.InitializeBatch() == INITBATCH_NORMAL && _data.CalculationStatus() == CALCSTAT_RESUMABLE)
			{
				_data.SetInitializeBatch(INITBATCH_FINISH_CALC); // flag to finish calc before save 
			}
			if (_data.Loaded3D())      // 'r' of image created with '3' 
			{
				_data.SetDisplay3D(DISPLAY3D_YES);  // so set flag for 'b' command 
			}
		}
		else
		{                            // draw an image 
			if (_data.SaveTime() != 0          // autosave and resumable? 
				&& (_data.CurrentFractalSpecificFlags() & FRACTALFLAG_NOT_RESUMABLE) == 0)
			{
				_data.SetSaveBase(_app->read_ticker()); // calc's start time 
				_data.SetSaveTicks(_data.SaveTime()*60*1000); // in milliseconds 
				_data.SetFinishRow(-1);
			}
			g_browse_state.SetBrowsing(false);      // regenerate image, turn off browsing 
			_data.SetNameStackPointer(-1);
			g_browse_state.SetName("");
			if (_data.GetViewWindow().Visible() && (_data.EvolvingFlags() & EVOLVE_FIELD_MAP) && (_data.CalculationStatus() != CALCSTAT_COMPLETED))
			{
				// generate a set of images with varied parameters on each one 
				int grout;
				int ecount;
				int tmpxdots;
				int tmpydots;
				int gridsqr;
				evolution_info resume_e_info;

				if ((g_evolve_info != 0) && (_data.CalculationStatus() == CALCSTAT_RESUMABLE))
				{
					resume_e_info = *g_evolve_info;
					g_parameter_range_x = resume_e_info.parameter_range_x;
					g_parameter_range_y = resume_e_info.parameter_range_y;
					g_parameter_offset_x = resume_e_info.opx;
					g_parameter_offset_y = resume_e_info.opy;
					g_new_parameter_offset_x = resume_e_info.opx;
					g_new_parameter_offset_y = resume_e_info.opy;
					g_new_discrete_parameter_offset_x = resume_e_info.odpx;
					g_new_discrete_parameter_offset_y = resume_e_info.odpy;
					g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x;
					g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y;
					g_px = resume_e_info.px;
					g_py = resume_e_info.py;
					_data.SetScreenOffset(resume_e_info.screen_x_offset, resume_e_info.screen_y_offset);
					_data.SetXDots(resume_e_info.x_dots);
					_data.SetYDots(resume_e_info.y_dots);
					g_grid_size = resume_e_info.grid_size;
					g_this_generation_random_seed = resume_e_info.this_generation_random_seed;
					g_fiddle_factor = resume_e_info.fiddle_factor;
					_data.SetEvolvingFlags(resume_e_info.evolving);
					if (_data.EvolvingFlags())
					{
						_data.GetViewWindow().Show();
					}
					ecount = resume_e_info.ecount;
					delete g_evolve_info;
					g_evolve_info = 0;
				}
				else
				{ // not resuming, start from the beginning 
					int mid = g_grid_size/2;
					if ((g_px != mid) || (g_py != mid))
					{
						g_this_generation_random_seed = (unsigned int)clock_ticks(); // time for new set 
					}
					_app->save_parameter_history();
					ecount = 0;
					g_fiddle_factor *= g_fiddle_reduction;
					g_parameter_offset_x = g_new_parameter_offset_x;
					g_parameter_offset_y = g_new_parameter_offset_y;
					// odpx used for discrete parms like inside, outside, trigfn etc 
					g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x;
					g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y; 
				}
				g_parameter_box_count = 0;
				g_delta_parameter_image_x = g_parameter_range_x/(g_grid_size-1);
				g_delta_parameter_image_y = g_parameter_range_y/(g_grid_size-1);
				grout = !((_data.EvolvingFlags() & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
				tmpxdots = _data.XDots() + grout;
				tmpydots = _data.YDots() + grout;
				gridsqr = g_grid_size*g_grid_size;
				while (ecount < gridsqr)
				{
					_app->spiral_map(ecount); // sets px & py 
					_data.SetScreenOffset(tmpxdots*g_px, tmpydots*g_py);
					_app->restore_parameter_history();
					_app->fiddle_parameters(g_genes, ecount);
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
					if (g_evolve_info == 0)
					{
						g_evolve_info = new evolution_info;
					}
					resume_e_info.parameter_range_x = g_parameter_range_x;
					resume_e_info.parameter_range_y = g_parameter_range_y;
					resume_e_info.opx = g_parameter_offset_x;
					resume_e_info.opy = g_parameter_offset_y;
					resume_e_info.odpx = short(g_discrete_parameter_offset_x);
					resume_e_info.odpy = short(g_discrete_parameter_offset_y);
					resume_e_info.px = short(g_px);
					resume_e_info.py = short(g_py);
					resume_e_info.screen_x_offset = short(_data.ScreenXOffset());
					resume_e_info.screen_y_offset = short(_data.ScreenYOffset());
					resume_e_info.x_dots = short(_data.XDots());
					resume_e_info.y_dots = short(_data.YDots());
					resume_e_info.grid_size = short(g_grid_size);
					resume_e_info.this_generation_random_seed = short(g_this_generation_random_seed);
					resume_e_info.fiddle_factor = g_fiddle_factor;
					resume_e_info.evolving = short(_data.EvolvingFlags());
					resume_e_info.ecount = short(ecount);
					*g_evolve_info = resume_e_info;
				}
				_data.SetScreenOffset(0, 0);
				_data.SetXDots(_data.ScreenWidth());
				_data.SetYDots(_data.ScreenHeight()); // otherwise save only saves a sub image and boxes get clipped 

				// set up for 1st selected image, this reuses px and py 
				g_px = g_grid_size/2;
				g_py = g_grid_size/2;
				_app->unspiral_map(); // first time called, w/above line sets up array 
				_app->restore_parameter_history();
				_app->fiddle_parameters(g_genes, 0);
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

			_data.SetSaveTicks(0);                 // turn off autosave timer 
			if (_driver->diskp() && i == 0) // disk-video 
			{
				_app->disk_video_status(0, "Image has been completed");
			}
		}
#ifndef XFRACT
		g_zoomBox.set_count(0);                     // no zoom box yet  
		g_z_width = 0;
#else
		if (!XZoomWaiting)
		{
			g_zoomBox.set_count(0);                 // no zoom box yet  
			g_z_width = 0;
		}
#endif

		if (g_fractal_type == FRACTYPE_PLASMA)
		{
			g_cycle_limit = 256;              // plasma clouds need quick spins 
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
			if (g_timed_save != TIMEDSAVE_DONE)
			{
				if (g_timed_save == TIMEDSAVE_START)
				{       // woke up for timed save 
					_driver->get_key();     // eat the dummy char 
					kbdchar = 's'; // do the save 
					g_resave_mode = RESAVE_YES;
					g_timed_save = TIMEDSAVE_PENDING;
				}
				else
				{                      // save done, resume 
					g_timed_save = TIMEDSAVE_DONE;
					g_resave_mode = RESAVE_DONE;
					kbdchar = IDK_ENTER;
				}
			}
			else if (_data.InitializeBatch() == INITBATCH_NONE)      // not batch mode 
			{
				_driver->set_mouse_mode((g_z_width == 0) ? -IDK_PAGE_UP : LOOK_MOUSE_ZOOM_BOX);
				if (_data.CalculationStatus() == CALCSTAT_RESUMABLE && g_z_width == 0 && !_driver->key_pressed())
				{
					kbdchar = IDK_ENTER;  // no visible reason to stop, continue 
				}
				else      // wait for a real keystroke 
				{
					if (g_browse_state.AutoBrowse() && g_browse_state.SubImages())
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
						if (kbdchar == IDK_ESC && g_escape_exit_flag)
						{
							// don't ask, just get out 
							_app->goodbye();
						}
						_driver->stack_screen();
#ifndef XFRACT
						kbdchar = _app->main_menu(true);
#else
						if (XZoomWaiting)
						{
							kbdchar = IDK_ENTER;
						}
						else
						{
							kbdchar = main_menu(true);
							if (XZoomWaiting)
							{
								kbdchar = IDK_ENTER;
							}
						}
#endif
						if (kbdchar == '\\' || kbdchar == IDK_CTL_BACKSLASH ||
							kbdchar == 'h' || kbdchar == IDK_BACKSPACE ||
							check_video_mode_key(kbdchar) >= 0)
						{
							_driver->discard_screen();
						}
						else if (kbdchar == 'x' || kbdchar == 'y' ||
								kbdchar == 'z' || kbdchar == 'g' ||
								kbdchar == 'v' || kbdchar == IDK_CTL_B ||
								kbdchar == IDK_CTL_E || kbdchar == IDK_CTL_F)
						{
							g_from_text_flag = true;
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
				// g_initialize_batch == -1  flag to finish calc before save 
				// g_initialize_batch == 0   not in batch mode 
				// g_initialize_batch == 1   normal batch mode 
				// g_initialize_batch == 2   was 1, now do a save 
				// g_initialize_batch == 3   bailout with errorlevel == 2, error occurred, no save 
				// g_initialize_batch == 4   bailout with errorlevel == 1, interrupted, try to save 
				// g_initialize_batch == 5   was 4, now do a save 

				if (_data.InitializeBatch() == INITBATCH_FINISH_CALC)       // finish calc 
				{
					kbdchar = IDK_ENTER;
					_data.SetInitializeBatch(INITBATCH_NORMAL);
				}
				else if (_data.InitializeBatch() == INITBATCH_NORMAL || _data.InitializeBatch() == INITBATCH_BAILOUT_INTERRUPTED) // save-to-disk 
				{
					kbdchar = (DEBUGMODE_COMPARE_RESTORED == _data.DebugMode()) ? 'r' : 's';
					if (_data.InitializeBatch() == INITBATCH_NORMAL)
					{
						_data.SetInitializeBatch(INITBATCH_SAVE);
					}
					if (_data.InitializeBatch() == INITBATCH_BAILOUT_INTERRUPTED)
					{
						_data.SetInitializeBatch(INITBATCH_BAILOUT_SAVE);
					}
				}
				else
				{
					if (_data.CalculationStatus() != CALCSTAT_COMPLETED)
					{
						_data.SetInitializeBatch(INITBATCH_BAILOUT_ERROR); // bailout with error 
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
			if (_data.ZoomOff() && _keyboardMore) // draw/clear a zoom box? 
			{
				_app->zoom_box_draw(true);
			}
			if (_driver->resize())
			{
				_data.SetCalculationStatus(CALCSTAT_NO_FRACTAL);
			}
		}
	}
}
