#pragma once

#include <memory>
#include <wx/cmndata.h>
#include <wx/control.h>

class wxFontDialog;

namespace wxId {

class CGAWindowControlMediator;

class ICGAWindowControl
{
public:
	virtual ~ICGAWindowControl()  { }

	virtual bool SetBackgroundColour(const wxColour& colour) = 0;
	virtual wxFont GetFont() const = 0;
	virtual wxColour GetForegroundColour() = 0;
	virtual void SetInitialSize(const wxSize& size = wxDefaultSize) = 0;
	virtual wxWindow *GetWindow() = 0;
	virtual wxRect GetClientRect() const = 0;
};

class CGAWindowControl : public wxControl
{
	DECLARE_DYNAMIC_CLASS(CGAWindowControl);
	DECLARE_EVENT_TABLE();

public:
	CGAWindowControl();
	CGAWindowControl(wxWindow *parent,
		wxWindowID id,
		const wxPoint &pos = wxDefaultPosition,
		const wxSize &size = wxDefaultSize,
		long style = wxSUNKEN_BORDER,
		const wxValidator &validator = wxDefaultValidator);

	bool Create(wxWindow *parent, wxWindowID id,
		const wxPoint &pos = wxDefaultPosition,
		const wxSize &size = wxDefaultSize,
		long style = wxSUNKEN_BORDER,
		const wxValidator &validator = wxDefaultValidator);

	void Init() { }

	wxSize DoGetBestSize() const;

	void OnPaint(wxPaintEvent &event);
	void OnMouseEvent(wxMouseEvent &event);

	virtual void SetInitialSize(const wxSize& size);
	virtual wxRect GetClientRect() const;
	virtual wxFontDialog *FontDialog(wxWindow *parent, const wxFontData &data);
	virtual wxEvtHandler *GetEventHandler() const;

private:
	std::auto_ptr<CGAWindowControlMediator> _mediator;
};

class CGAWindowFontChangedEvent : public wxNotifyEvent
{
public:
	CGAWindowFontChangedEvent(wxEventType commandType = wxEVT_NULL,
			int id = 0)
		: wxNotifyEvent(commandType, id)
	{
	}

	CGAWindowFontChangedEvent(const CGAWindowFontChangedEvent &rhs)
		: wxNotifyEvent(rhs)
	{
	}

	virtual wxEvent *Clone() const
	{ return new CGAWindowFontChangedEvent(*this); }

	DECLARE_DYNAMIC_CLASS(CGAWindowFontChangedEvent);
};

typedef void (wxEvtHandler::*CGAWindowFontChangedEventHandler)(CGAWindowFontChangedEvent &event);

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_COMMAND_CGA_WINDOW_FONT_CHANGED, 801)
END_DECLARE_EVENT_TYPES()

#define EVT_CGA_WINDOW_FONT_CHANGED(id_, fn_) \
	DECLARE_EVENT_TABLE_ENTRY(wxEVT_COMMAND_CGA_WINDOW_FONT_CHANGED, \
		id_, -1, wxObjectEventFunction(wxEventFunction(CGAWindowFontChangedEventHandler(fn_))) &fn_, 0),

}
