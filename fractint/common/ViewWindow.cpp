#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"

#include "ViewWindow.h"

using namespace boost;

int ViewWindow::CommandArgument(cmd_context const &context)
{
	if (context.totparms > 5
		|| (context.floatparms - context.intparms) > 2
		|| context.intparms > 4)
	{
		return bad_arg(context.curarg);
	}
	Show();
	g_view_reduction = 4.2f;  /* reset default values */
	g_final_aspect_ratio = g_screen_aspect_ratio;
	g_view_crop = true;
	g_view_x_dots = 0;
	g_view_y_dots = 0;

	if ((context.totparms > 0) && (context.floatval[0] > 0.001))
	{
		g_view_reduction = float(context.floatval[0]);
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
	return g_viewWindow.Visible() ?
		str(format(" viewwindows=%g/%g/%s/%d/%d")
			% g_view_reduction % g_final_aspect_ratio
			% (g_view_crop ? "/yes" : "/no")
			% g_view_x_dots % g_view_y_dots)
		: "";
}

ViewWindow g_viewWindow;
