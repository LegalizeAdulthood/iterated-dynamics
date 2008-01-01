#pragma once

class ViewWindow
{
public:
	ViewWindow() : _visible(false)
	{
	}

	bool Visible() const				{ return _visible; }
	void Show()							{ _visible = true; }
	void Hide()							{ _visible = false; }

private:
	bool _visible;
};

extern ViewWindow g_viewWindow;
