#pragma once

#include "CommandParser.h"

class ViewWindow
{
public:
	ViewWindow() : _visible(false)
	{
	}

	bool Visible() const				{ return _visible; }
	std::string CommandParameters() const;

	int CommandArgument(cmd_context const &context);
	void Hide()							{ _visible = false; }
	void Show()							{ _visible = true; }

private:
	bool _visible;
};

extern ViewWindow g_viewWindow;
