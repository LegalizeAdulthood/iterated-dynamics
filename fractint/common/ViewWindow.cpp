#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"

#include "ViewWindow.h"

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

ViewWindow g_viewWindow;
