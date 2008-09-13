#include "stdafx.h"
#include "wxId.h"
#include "CGAWindowControl.h"
#include "wx/evtloop.h"

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int MainLoop();
};

template <typename T>
class TPtr
{
private:
    T * m_ptr;

    TPtr(TPtr const &);
    TPtr & operator=(TPtr const &);

public:
    wxEXPLICIT TPtr(T * ptr = NULL)
    : m_ptr(ptr) { }

	~TPtr()
	{
		typedef char complete[sizeof(*m_ptr)];
		delete m_ptr;
	}

    void reset(T * ptr = NULL)
    {
        if (m_ptr != ptr)
        {
            delete m_ptr;
            m_ptr = ptr;
        }
    }

    T *release()
    {
        T *ptr = m_ptr;
        m_ptr = NULL;
        return ptr;
    }

    T & operator*() const
    {
        wxASSERT(m_ptr != NULL);
        return *m_ptr;
    }

    T * operator->() const
    {
        wxASSERT(m_ptr != NULL);
        return m_ptr;
    }

    T * get() const
    {
        return m_ptr;
    }

    void swap(TPtr & ot)
    {
        T * tmp = ot.m_ptr;
        ot.m_ptr = m_ptr;
        m_ptr = tmp;
    }
};

template <typename T>
class TiedPtr : public TPtr<T>
{
public:
    TiedPtr(T **pp, T *p = new T)
        : TPtr(p), m_pp(pp), m_pOld(*pp)
    {
        *pp = p;
    }

    ~ TiedPtr()
    {
        *m_pp = m_pOld;
    }

private:
    T **m_pp;
    T *m_pOld;
};

class IdEventLoop : public wxEventLoop
{
public:
	virtual ~IdEventLoop() { }

protected:
	virtual void OnNextIteration()
	{
		//if (!Pending())
		//{
		//	m_shouldExit = true;
		//}
	}
};

int MyApp::MainLoop()
{
    TiedPtr<wxEventLoop> mainLoop(&m_mainLoop, new IdEventLoop);

    int result = m_mainLoop->Run();
	//while (!result)
	//{
	//	result = m_mainLoop->Run();
	//}
	return result;
}

class MyFrame: public wxFrame
{
public:
	MyFrame(wxString const &title, wxPoint const &pos, wxSize const &size);

	void OnQuit(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

enum
{
	ID_Quit = 1,
	ID_About,
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MENU(ID_Quit, MyFrame::OnQuit)
	EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	MyFrame *frame = new MyFrame(_T("Hello World"), wxPoint(50,50), wxSize(450,340));
	frame->Show(TRUE);
	SetTopWindow(frame);
	return TRUE;
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;

	menuFile->Append(ID_About, _T("&About..."));
	menuFile->AppendSeparator();
	menuFile->Append(ID_Quit, _T("E&xit"));

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, _T("&File"));

	SetMenuBar(menuBar);

	CreateStatusBar();
	SetStatusText(_T("Welcome to wxWidgets!"));

	wxId::CGAWindowControl *cgaWindow = new wxId::CGAWindowControl(this, ID_CGAWindow);
}

void MyFrame::OnQuit(wxCommandEvent &)
{
	Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent &)
{
	wxMessageBox(_T("This is a wxWidgets Hello world sample"),
		_T("About Hello World"), wxOK | wxICON_INFORMATION, this);
}
