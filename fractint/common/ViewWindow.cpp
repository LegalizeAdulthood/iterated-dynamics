#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"

#include "ViewWindow.h"

using namespace boost;

float const ViewWindow::DEFAULT_REDUCTION = 4.2f;

int ViewWindow::CommandArgument(cmd_context const &context)
{
	if (context.totparms > 5
		|| (context.floatparms - context.intparms) > 2
		|| context.intparms > 4)
	{
		return bad_arg(context.curarg);
	}
	InitializeRestart();
	Show();

	if ((context.totparms > 0) && (context.floatval[0] > 0.001))
	{
		_reduction = float(context.floatval[0]);
	}
	if ((context.totparms > 1) && (context.floatval[1] > 0.001))
	{
		g_final_aspect_ratio = float(context.floatval[1]);
	}
	if ((context.totparms > 2) && (context.yesnoval[2] == 0))
	{
		g_view_crop = false;
	}
	if ((context.totparms > 3) && (context.intval[3] > 0))
	{
		g_view_x_dots = context.intval[3];
	}
	if ((context.totparms == 5) && (context.intval[4] > 0))
	{
		g_view_y_dots = context.intval[4];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

std::string ViewWindow::CommandParameters() const
{
	return Visible() ?
		str(format(" viewwindows=%g/%g/%s/%d/%d")
			% _reduction % g_final_aspect_ratio
			% (g_view_crop ? "/yes" : "/no")
			% g_view_x_dots % g_view_y_dots)
		: "";
}

void ViewWindow::InitializeRestart()
{
	Hide();
	_reduction = DEFAULT_REDUCTION;
	g_view_crop = true;
	g_final_aspect_ratio = g_screen_aspect_ratio;
	g_view_x_dots = 0;
	g_view_y_dots = 0;
}

ViewWindow g_viewWindow;
