#include <cmath>
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"

#include "helpdefs.h"
#include "loadfdos.h"
#include "UIChoices.h"
#include "ViewWindow.h"
#include "zoom.h"

using namespace boost;

float const ViewWindow::DEFAULT_REDUCTION = 4.2f;
float const ViewWindow::ASPECTRATIO_3x4 = 3.0f/4.0f;

int ViewWindow::CommandArgument(cmd_context const &context)
{
	if (context.totparms > 5
		|| (context.floatparms - context.intparms) > 2
		|| context.intparms > 4)
	{
		return bad_arg(context.curarg);
	}
	InitializeRestart();

	_visible = true;
	if ((context.totparms > 0) && (context.floatval[0] > 0.001))
	{
		_reduction = float(context.floatval[0]);
	}
	if ((context.totparms > 1) && (context.floatval[1] > 0.001))
	{
		_aspectRatio = float(context.floatval[1]);
	}
	if ((context.totparms > 2) && (context.yesnoval[2] == 0))
	{
		_viewCrop = false;
	}
	if ((context.totparms > 3) && (context.intval[3] > 0))
	{
		_width = context.intval[3];
	}
	if ((context.totparms == 5) && (context.intval[4] > 0))
	{
		_height = context.intval[4];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

std::string ViewWindow::CommandParameters() const
{
	return _visible ?
		str(format(" viewwindows=%g/%g/%s/%d/%d")
			% _reduction % _aspectRatio
			% (_viewCrop ? "/yes" : "/no")
			% _width % _height)
		: "";
}

void ViewWindow::InitializeRestart()
{
	_visible = false;
	_reduction = DEFAULT_REDUCTION;
	_viewCrop = true;
	_aspectRatio = g_screen_aspect_ratio;
	_width = 0;
	_height = 0;
}

int ViewWindow::GetParameters()
{
	bool old_viewwindow = _visible;
	float old_viewreduction = _reduction;
	float old_aspectratio = _aspectRatio;
	int old_viewxdots = _width;
	int old_viewydots = _height;

get_view_restart:
	{
		UIChoices dialog(HELPVIEW, "View Window Options", 16);

		dialog.push("Preview display? (no for full screen)", _visible);
		dialog.push("Auto window size reduction factor", _reduction);
		dialog.push("Final media overall aspect ratio, y/x", _aspectRatio);
		dialog.push("Crop starting coordinates to new aspect ratio?", _viewCrop);
		dialog.push("Explicit size x pixels (0 for auto size)", _width);
		dialog.push("              y pixels (0 to base on aspect ratio)", _height);
		dialog.push("");
		dialog.push("Press F4 to reset view parameters to defaults.");

		int i = dialog.prompt();
		if (i < 0)
		{
			return -1;
		}

		if (i == FIK_F4)
		{
			InitializeRestart();
			goto get_view_restart;
		}

		int k = -1;
		_visible = (dialog.values(++k).uval.ch.val != 0);
		_reduction = float(dialog.values(++k).uval.dval);
		_aspectRatio = float(dialog.values(++k).uval.dval);
		_viewCrop = (dialog.values(++k).uval.ch.val != 0);
		_width = dialog.values(++k).uval.ival;
		_height = dialog.values(++k).uval.ival;
	}

	if (_width != 0 && _height != 0 && _visible && _aspectRatio == 0.0f)
	{
		_aspectRatio = float(_height)/float(_width);
	}
	else if (_aspectRatio == 0.0 && (_width == 0 || _height == 0))
	{
		_aspectRatio = old_aspectratio;
	}

	if (_aspectRatio != old_aspectratio && _viewCrop)
	{
		aspect_ratio_crop(old_aspectratio, _aspectRatio);
	}

	return (_visible != old_viewwindow
		|| (_visible
			&& (_reduction != old_viewreduction
				|| _aspectRatio != old_aspectratio
				|| _width != old_viewxdots
				|| (_height != old_viewydots && _width)))) ? 1 : 0;
}

void ViewWindow::SetReductionFromVideoEntry(const VIDEOINFO &entry)
{
	if (_width)
	{
		_reduction = float(entry.x_dots/_width);
		_width = 0;
		_height = 0; /* easier to use auto reduction */
	}
	_reduction = std::floor(_reduction + 0.5f); /* need integer value */
}

void ViewWindow::SetFromVideoEntry()
{
	SetFromVideoMode(g_file_x_dots, g_file_y_dots,
		g_file_aspect_ratio, g_screen_aspect_ratio,
		g_.VideoEntry());
}

void ViewWindow::SetFromVideoMode(int file_x_dots, int file_y_dots,
								  float file_aspect_ratio, float screen_aspect_ratio,
								  VIDEOINFO const &video)
{
	_aspectRatio = file_aspect_ratio;
	if (_aspectRatio == 0) /* assume display correct */
	{
		_aspectRatio = float(video_mode_aspect_ratio(file_x_dots, file_y_dots));
	}
	if (_aspectRatio >= screen_aspect_ratio - 0.02f
		&& _aspectRatio <= screen_aspect_ratio + 0.02f)
	{
		_aspectRatio = screen_aspect_ratio;
	}
	int i = int(_aspectRatio*1000.0 + 0.5);
	_aspectRatio = float(i/1000.0); /* chop precision to 3 decimals */

	/* setup view window stuff */
	_visible = false;
	_width = 0;
	_height = 0;

	if (file_x_dots != video.x_dots || file_y_dots != video.y_dots)
	{
		/* image not exactly same size as screen */
		_visible = true;
		double ftemp = _aspectRatio*double(video.y_dots)/double(video.x_dots)/screen_aspect_ratio;
		int x_dots, y_dots;
		float view_reduction;
		if (_aspectRatio <= screen_aspect_ratio)
		{
			x_dots = int(double(video.x_dots)/double(file_x_dots)*20.0 + 0.5);
			view_reduction = float(x_dots/20.0); /* chop precision to nearest .05 */
			x_dots = int(double(video.x_dots)/view_reduction + 0.5);
			y_dots = int(double(x_dots)*ftemp + 0.5);
		}
		else
		{
			x_dots = int(double(video.y_dots)/double(file_y_dots)*20.0 + 0.5);
			view_reduction = float(x_dots/20.0); /* chop precision to nearest .05 */
			y_dots = int(double(video.y_dots)/view_reduction + 0.5);
			x_dots = int(double(y_dots)/ftemp + 0.5);
		}
		if (x_dots != file_x_dots || y_dots != file_y_dots)  /* too bad, must be explicit */
		{
			_width = file_x_dots;
			_height = file_y_dots;
		}
		else
		{
			_reduction = view_reduction; /* ok, this works */
		}
	}
}

void ViewWindow::SetSizeFromGrid(int width, int height, int grid_size)
{
	_width = (width/grid_size) - 2;
	_height = (height/grid_size) - 2;
	if (!_visible)
	{
		_width = 0;
		_height = 0;
	}
}

void ViewWindow::FullScreen(int width, int height)
{
	_visible = false;
	_width = width;
	_height = height;
}

ViewWindow g_viewWindow;
