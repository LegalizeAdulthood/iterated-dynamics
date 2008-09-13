#include "stdafx.h"

#include <wx/dcclient.h>
#include <wx/dialog.h>
#include <wx/fontdlg.h>
#include <wx/frame.h>
#include <wx/settings.h>

#include "CGAWindowControl.h"
#include "CGAWindowControlMediator.h"

namespace wxId {

static wxString s_text;

CGAWindowControlMediator::CGAWindowControlMediator(CGAWindowControl &control)
	: _control(control),
	_screenFont(),
	_width(DEFAULT_WIDTH),
	_height(DEFAULT_HEIGHT),
	_attributes(),
	_text()
{
	_attributes.resize(_width*_height);
	_text.resize(_width*_height);
}

void CGAWindowControlMediator::Create(wxWindow *parent, wxWindowID id,
		const wxPoint &pos, const wxSize &size,
		long style, const wxValidator &validator)
{
	_control.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

	_screenFont.SetInitialFont(_control.GetFont());
	_screenFont.SetChosenFont(_control.GetFont());
	_screenFont.SetColour(_control.GetForegroundColour());
	s_text = _T("This is a fine mess you've gotten us into.");

	_control.SetInitialSize(size);
}

void CGAWindowControlMediator::OnPaint(wxPaintEvent &event, wxPaintDC &dc)
{
	wxRect rect = _control.GetClientRect();
	int topMargin = 2;
	int leftMargin = 2;

	dc.SetFont(_screenFont.GetChosenFont());
	wxCoord width, height;
	dc.GetTextExtent(s_text, &width, &height);
	int x = wxMax(leftMargin, (rect.GetWidth() - width)/2);
	int y = wxMax(topMargin, (rect.GetHeight() - height)/2);

	dc.SetBackgroundMode(wxTRANSPARENT);
	dc.SetTextForeground(_screenFont.GetColour());
	dc.DrawText(s_text, x, y);
	dc.SetFont(wxNullFont);
}

wxSize CGAWindowControlMediator::DoGetBestSize() const
{
	// TODO: figure out character cell size from font	
	wxSize characterCell(16,9);
	//characterCell = _screenFont.GetChosenFont().GetPixelSize();
	const int COLUMNS_PER_LINE = 80;
	const int LINES_PER_SCREEN = 25;
	return characterCell.Scale(COLUMNS_PER_LINE, LINES_PER_SCREEN);
}

void CGAWindowControlMediator::OnMouseEvent(wxMouseEvent &event)
{
	if (event.LeftDown())
	{
		wxWindow *parent = _control.GetParent();
		while (parent != 0 &&
			!parent->IsKindOf(CLASSINFO(wxDialog)) &&
			!parent->IsKindOf(CLASSINFO(wxFrame)))
		{
			parent = parent->GetParent();
		}
		wxFontDialog *dialog = _control.FontDialog(parent, _screenFont);
		wxString title = _T("Please choose a font");
		dialog->SetTitle(title);
		if (dialog->ShowModal() == wxID_OK)
		{
			_screenFont = dialog->GetFontData();
			_screenFont.SetInitialFont(_screenFont.GetChosenFont());
			_control.Refresh();
			CGAWindowFontChangedEvent fontChanged(
				wxEVT_COMMAND_CGA_WINDOW_FONT_CHANGED, _control.GetId());
			fontChanged.SetEventObject(&_control);
			_control.GetEventHandler()->ProcessEvent(fontChanged);
		}
		delete dialog;
	}
}

}
