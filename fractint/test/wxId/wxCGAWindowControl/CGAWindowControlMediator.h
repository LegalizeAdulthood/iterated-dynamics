#pragma once

#include <vector>

#include <wx/cmndata.h>

class wxPaintDC;

namespace wxId {

class ICGAWindowControl;

class CGAWindowControlMediator
{
public:
	CGAWindowControlMediator(CGAWindowControl &control);

	void Create(wxWindow *parent, wxWindowID id,
		const wxPoint &pos = wxDefaultPosition,
		const wxSize &size = wxDefaultSize,
		long style = wxSUNKEN_BORDER,
		const wxValidator &validator = wxDefaultValidator);
	void OnPaint(wxPaintEvent &event, wxPaintDC &dc);
	void OnMouseEvent(wxMouseEvent &event);
	wxSize DoGetBestSize() const;

private:
	enum
	{
		DEFAULT_WIDTH = 80,
		DEFAULT_HEIGHT = 25
	};
	CGAWindowControl &_control;
	wxFontData _screenFont;
	int _width;
	int _height;
	std::vector<int> _attributes;
	std::vector<char> _text;
};

}
