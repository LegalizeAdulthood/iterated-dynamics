#pragma once

#include "CommandParser.h"

class ViewWindow
{
public:
	ViewWindow() : _visible(false),
		_reduction(0.0f),
		_aspectRatio(ASPECTRATIO_3x4)
	{
	}

	float AspectRatio() const			{ return _aspectRatio; }
	void SetAspectRatio(float value)	{ _aspectRatio = value; }
	std::string CommandParameters() const;
	float Reduction() const				{ return _reduction; }
	void SetReduction(float value)		{ _reduction = value; }
	bool Visible() const				{ return _visible; }

	int CommandArgument(cmd_context const &context);
	void Hide()							{ _visible = false; }
	void InitializeRestart();
	void Show()							{ _visible = true; }

private:
	static float const ASPECTRATIO_3x4;
	static float const DEFAULT_REDUCTION;
	bool _visible;
	float _reduction;
	float _aspectRatio;					// for view shape and rotation
};

extern ViewWindow g_viewWindow;
