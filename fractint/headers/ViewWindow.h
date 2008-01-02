#pragma once

#include "CommandParser.h"

class ViewWindow
{
public:
	ViewWindow() : _visible(false)
	{
	}

	std::string CommandParameters() const;
	float Reduction() const				{ return _reduction; }
	void SetReduction(float value)		{ _reduction = value; }
	bool Visible() const				{ return _visible; }

	int CommandArgument(cmd_context const &context);
	void Hide()							{ _visible = false; }
	void InitializeRestart();
	void Show()							{ _visible = true; }

private:
	static float const DEFAULT_REDUCTION;
	bool _visible;
	float _reduction;
};

extern ViewWindow g_viewWindow;
