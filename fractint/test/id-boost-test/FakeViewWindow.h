#pragma once

#include "ViewWindow.h"

class FakeViewWindow : public ViewWindow
{
public:
	virtual ~FakeViewWindow() { }

	virtual float AspectRatio() const { return 0.0f; }
	virtual void SetAspectRatio(float value){ }
	virtual std::string CommandParameters() const { return ""; }
	virtual bool Crop() const { return false; }
	virtual int Height() const { return 0; }
	virtual float Reduction() const { return 0.0f; }
	virtual void SetReductionFromVideoEntry(const VIDEOINFO &entry) { }
	virtual bool Visible() const { return false; }
	virtual int Width() const { return 0 ; }

	virtual int CommandArgument(cmd_context const &context) { return 0; }
	virtual void FullScreen(int width, int height) { }
	virtual int GetParameters() { return 0; }
	virtual void Hide() { }
	virtual void InitializeRestart() { }
	virtual void SetFromVideoEntry() { }
	virtual void SetFromVideoMode(int file_x_dots, int file_y_dots,
		float file_aspect_ratio, float screen_aspect_ratio,
		VIDEOINFO const &video) { }
	virtual void SetSizeFromGrid(int width, int height, int gridSize) { }
	virtual void Show() { }
	virtual void Show(bool value) { }
};
