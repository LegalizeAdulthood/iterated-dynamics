#pragma once

#include "CommandParser.h"

class ViewWindow
{
public:
	ViewWindow() : _visible(false),
		_reduction(0.0f),
		_aspectRatio(ASPECTRATIO_3x4),
		_viewCrop(true),
		_width(0),
		_height(0)
	{
	}

	float AspectRatio() const			{ return _aspectRatio; }
	void SetAspectRatio(float value)	{ _aspectRatio = value; }
	std::string CommandParameters() const;
	bool Crop() const					{ return _viewCrop; }
	int Height() const					{ return _height; }
	float Reduction() const				{ return _reduction; }
	void SetReductionFromVideoEntry(const VIDEOINFO &entry);
	bool Visible() const				{ return _visible; }
	int Width() const					{ return _width; }

	int CommandArgument(cmd_context const &context);
	void FullScreen(int width, int height);
	int GetParameters();
	void Hide()							{ _visible = false; }
	void InitializeRestart();
	void SetFromVideoEntry();
	void SetFromVideoMode(int file_x_dots, int file_y_dots,
		float file_aspect_ratio, float screen_aspect_ratio,
		VIDEOINFO const &video);
	void SetSizeFromGrid(int width, int height, int gridSize);
	void Show()							{ _visible = true; }
	void Show(bool value)				{ _visible = value; }

private:
	static float const ASPECTRATIO_3x4;
	static float const DEFAULT_REDUCTION;
	bool _visible;
	float _reduction;
	float _aspectRatio;					// for view shape and rotation
	bool _viewCrop;						// true to crop default coords
	int _width;
	int _height;
};

extern ViewWindow g_viewWindow;
