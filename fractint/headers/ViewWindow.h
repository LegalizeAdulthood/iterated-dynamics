#if !defined(VIEW_WINDOW_H)
#define VIEW_WINDOW_H

#include "CommandParser.h"

class ViewWindow
{
public:
	virtual ~ViewWindow() { }

	virtual float AspectRatio() const = 0;
	virtual void SetAspectRatio(float value) = 0;
	virtual std::string CommandParameters() const = 0;
	virtual bool Crop() const = 0;
	virtual int Height() const = 0;
	virtual float Reduction() const = 0;
	virtual void SetReductionFromVideoEntry(const VIDEOINFO &entry) = 0;
	virtual bool Visible() const = 0;
	virtual int Width() const = 0;

	virtual int CommandArgument(cmd_context const &context) = 0;
	virtual void FullScreen(int width, int height) = 0;
	virtual int GetParameters() = 0;
	virtual void Hide() = 0;
	virtual void InitializeRestart() = 0;
	virtual void SetFromVideoEntry() = 0;
	virtual void SetFromVideoMode(int file_x_dots, int file_y_dots,
		float file_aspect_ratio, float screen_aspect_ratio,
		VIDEOINFO const &video) = 0;
	virtual void SetSizeFromGrid(int width, int height, int gridSize) = 0;
	virtual void Show() = 0;
	virtual void Show(bool value) = 0;
};

extern ViewWindow &g_viewWindow;

#endif
