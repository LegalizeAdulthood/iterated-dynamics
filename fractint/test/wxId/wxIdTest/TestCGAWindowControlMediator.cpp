#include "stdafx.h"

#include <boost/test/unit_test.hpp>

#include <wx/dcclient.h>
#include <wx/fontdlg.h>

#include "CGAWindowControl.h"
#include "CGAWindowControlMediator.h"

class FakeFontDialog : public wxFontDialog
{
public:
	FakeFontDialog() : wxFontDialog(),
		_showModalCalled(false),
		_showModalFakeResult(0),
		_setTitleCalled(false),
		_setTitleLastTitle()
	{
	}
	virtual ~FakeFontDialog() { }

	virtual int ShowModal()
	{
		_showModalCalled = true;
		return _showModalFakeResult;
	}
	bool ShowModalCalled() const			{ return _showModalCalled; }
	void SetShowModalFakeResult(int value)	{ _showModalFakeResult = value; }

	virtual void SetTitle( const wxString& title)
	{
		_setTitleCalled = true;
		_setTitleLastTitle = title;
	}
	bool SetTitleCalled() const				{ return _setTitleCalled; }

private:
	bool _showModalCalled;
	int _showModalFakeResult;
	bool _setTitleCalled;
	wxString _setTitleLastTitle;
};

class FakeCGAWindowControl : public wxId::CGAWindowControl
{
public:
	FakeCGAWindowControl() : CGAWindowControl(),
		_setBackgroundColorCalled(false),
		_setBackgroundColorLastColor(),
		_setBackgroundColorFakeResult(false),
		_setInitialSizeCalled(false),
		_setInitialSizeLastSize(),
		_getClientRectCalled(false),
		_getClientRectFakeResult(),
		_fontDialogCalled(false),
		_fontDialogFakeResult(0),
		_getEventHandlerCalled(false),
		_getEventHandlerFakeResult(0)
	{
	}
	virtual ~FakeCGAWindowControl() { }

	virtual bool SetBackgroundColour(const wxColour& colour)
	{
		_setBackgroundColorCalled = true;
		_setBackgroundColorLastColor = colour;
		return _setBackgroundColorFakeResult;
	}
	bool SetBackgroundColorCalled() const	{ return _setBackgroundColorCalled; }

	virtual void SetInitialSize(const wxSize& size)
	{
		_setInitialSizeCalled = true;
		_setInitialSizeLastSize = size;
	}
	bool SetInitialSizeCalled() const		{ return _setInitialSizeCalled; }

	virtual wxRect GetClientRect() const
	{
		Mutate()._getClientRectCalled = true;
		return _getClientRectFakeResult;
	}
	bool GetClientRectCalled() const		{ return _getClientRectCalled; }
	void SetGetClientRectFakeResult(wxRect const &value) { _getClientRectFakeResult = value; }

	virtual wxFontDialog *FontDialog(wxWindow *parent, wxFontData const &data)
	{
		_fontDialogCalled = true;
		return _fontDialogFakeResult;
	}
	bool FontDialogCalled() const			{ return _fontDialogCalled; }
	void SetFontDialogFakeResult(FakeFontDialog *value)	{ _fontDialogFakeResult = value; }

	wxEvtHandler *GetEventHandler() const
	{
		Mutate()._getEventHandlerCalled = true;
		return _getEventHandlerFakeResult;
	}
	bool GetEventHandlerCalled() const		{ return _getEventHandlerCalled; }
	void SetGetEventHandlerFakeResult(wxEvtHandler *value) { _getEventHandlerFakeResult = value; }

private:
	FakeCGAWindowControl &Mutate() const
	{ return *const_cast<FakeCGAWindowControl *>(this); }

	bool _setBackgroundColorCalled;
	wxColor _setBackgroundColorLastColor;
	bool _setBackgroundColorFakeResult;
	bool _setInitialSizeCalled;
	wxSize _setInitialSizeLastSize;
	bool _getClientRectCalled;
	wxRect _getClientRectFakeResult;
	bool _fontDialogCalled;
	FakeFontDialog *_fontDialogFakeResult;
	bool _getEventHandlerCalled;
	wxEvtHandler *_getEventHandlerFakeResult;
};

class FakePaintDC : public wxPaintDC
{
public:
	FakePaintDC() : wxPaintDC(),
		_setFontCalled(false),
		_setFontLastFont(),
		_setBackgroundModeCalled(false),
		_setBackgroundModeLastMode(0),
		_setTextForegroundCalled(false),
		_setTextForegroundLastColor(),
		_doDrawTextCalled(false),
		_doDrawTextLastText(),
		_doDrawTextLastX(0),
		_doDrawTextLastY(0)
	{ }
	virtual ~FakePaintDC()
	{ }

	virtual void SetFont(const wxFont& font)
	{
		_setFontCalled = true;
		_setFontLastFont = font;
	}
	bool SetFontCalled() const					{ return _setFontCalled; }

	virtual void SetBackgroundMode(int mode)
	{
		_setBackgroundModeCalled = true;
		_setBackgroundModeLastMode = mode;
	}
	bool SetBackgroundModeCalled() const		{ return _setBackgroundModeCalled; }

	virtual void SetTextForeground(const wxColour& colour)
	{
		_setTextForegroundCalled = true;
		_setTextForegroundLastColor = colour;
	}
	bool SetTextForegroundCalled() const		{ return _setTextForegroundCalled; }

	virtual void DoDrawText(const wxString& text, wxCoord x, wxCoord y)
	{
		_doDrawTextCalled = true;
		_doDrawTextLastText = text;
		_doDrawTextLastX = x;
		_doDrawTextLastY = y;
	}
	bool DoDrawTextCalled() const				{ return _doDrawTextCalled; }

private:
	FakePaintDC &Mutate() const
	{ return *const_cast<FakePaintDC *>(this); }
	bool _setFontCalled;
	wxFont _setFontLastFont;
	bool _setBackgroundModeCalled;
	int _setBackgroundModeLastMode;
	bool _setTextForegroundCalled;
	wxColour _setTextForegroundLastColor;
	bool _doDrawTextCalled;
	wxString _doDrawTextLastText;
	wxCoord _doDrawTextLastX;
	wxCoord _doDrawTextLastY;
};

BOOST_AUTO_TEST_CASE(CGAWindowControlMediator_ConstructDefault)
{
	FakeCGAWindowControl control;
	wxId::CGAWindowControlMediator mediator(control);
}

BOOST_AUTO_TEST_CASE(CGAWindowControlMediator_Create)
{
	FakeCGAWindowControl control;
	wxId::CGAWindowControlMediator mediator(control);
	mediator.Create(0, wxID_ANY);
	BOOST_CHECK(control.SetBackgroundColorCalled());
	BOOST_CHECK(control.SetInitialSizeCalled());
}

BOOST_AUTO_TEST_CASE(CGAWindowControlMediator_OnPaint)
{
	FakeCGAWindowControl control;
	wxId::CGAWindowControlMediator mediator(control);
	wxPaintEvent event;
	FakePaintDC dc;
	mediator.OnPaint(event, dc);
	BOOST_CHECK(control.GetClientRectCalled());
	BOOST_CHECK(dc.SetFontCalled());
	BOOST_CHECK(dc.SetBackgroundModeCalled());
	BOOST_CHECK(dc.SetTextForegroundCalled());
	BOOST_CHECK(dc.DoDrawTextCalled());
}

BOOST_AUTO_TEST_CASE(CGAWindowControlMediator_OnMouseEvent_RightClick)
{
	FakeCGAWindowControl control;
	wxId::CGAWindowControlMediator mediator(control);
	wxMouseEvent event(wxEVT_RIGHT_DOWN);
	mediator.OnMouseEvent(event);
	BOOST_CHECK(!control.FontDialogCalled());
}

BOOST_AUTO_TEST_CASE(CGAWindowControlMediator_OnMouseEvent_LeftClick_Cancel)
{
	FakeCGAWindowControl control;
	wxId::CGAWindowControlMediator mediator(control);
	wxMouseEvent event(wxEVT_LEFT_DOWN);
	FakeFontDialog *fontDialog = new FakeFontDialog();
	fontDialog->SetShowModalFakeResult(wxID_CANCEL);
	control.SetFontDialogFakeResult(fontDialog);
	mediator.OnMouseEvent(event);
	BOOST_CHECK(control.FontDialogCalled());
	BOOST_CHECK(!control.GetEventHandlerCalled());
}

BOOST_AUTO_TEST_CASE(CGAWindowControlMediator_DoGetBestSize)
{
	FakeCGAWindowControl control;
	wxId::CGAWindowControlMediator mediator(control);
	wxMouseEvent event(wxEVT_LEFT_DOWN);
	wxSize size = mediator.DoGetBestSize();
	BOOST_CHECK(size.GetWidth() > 0);
	BOOST_CHECK(size.GetHeight() > 0);
}
