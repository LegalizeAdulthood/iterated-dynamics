#include "stdafx.h"

#include <wx/dcclient.h>
#include <wx/fontdlg.h>

#include "CGAWindowControl.h"
#include "CGAWindowControlMediator.h"

namespace wxId {

BEGIN_EVENT_TABLE(CGAWindowControl, wxControl)
	EVT_PAINT(CGAWindowControl::OnPaint)
	EVT_MOUSE_EVENTS(CGAWindowControl::OnMouseEvent)
END_EVENT_TABLE()
IMPLEMENT_DYNAMIC_CLASS(CGAWindowControl, wxControl)

CGAWindowControl::CGAWindowControl() : wxControl(),
	_mediator(new CGAWindowControlMediator(*this))
{
}

CGAWindowControl::CGAWindowControl(wxWindow *parent,
		wxWindowID id,
		const wxPoint &pos,
		const wxSize &size,
		long style,
		const wxValidator &validator)
	: wxControl(),
	_mediator(new CGAWindowControlMediator(*this))
{
	Init();
	Create(parent, id, pos, size, style, validator);
}

bool CGAWindowControl::Create(wxWindow *parent, wxWindowID id,
		const wxPoint &pos, const wxSize &size,
		long style, const wxValidator &validator)
{
	if (!wxControl::Create(parent, id, pos, size, style, validator))
	{
		return false;
	}

	_mediator->Create(parent, id, pos, size, style, validator);

	return true;
}

wxSize CGAWindowControl::DoGetBestSize() const
{
	return _mediator->DoGetBestSize();
}

void CGAWindowControl::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
	_mediator->OnPaint(event, dc);
}

void CGAWindowControl::OnMouseEvent(wxMouseEvent &event)
{
	_mediator->OnMouseEvent(event);
}

void CGAWindowControl::SetInitialSize(const wxSize& size)
{
	wxControl::SetInitialSize(size);
}

wxRect CGAWindowControl::GetClientRect() const
{
	return wxControl::GetClientRect();
}

wxFontDialog *CGAWindowControl::FontDialog(wxWindow *parent, const wxFontData& data)
{
	return new wxFontDialog(parent, data);
}

wxEvtHandler *CGAWindowControl::GetEventHandler() const
{
	return wxControl::GetEventHandler();
}

DEFINE_EVENT_TYPE(wxEVT_COMMAND_CGA_WINDOW_FONT_CHANGED)
IMPLEMENT_DYNAMIC_CLASS(CGAWindowFontChangedEvent, wxNotifyEvent)

}
