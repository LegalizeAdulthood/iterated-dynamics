#include "stdafx.h"

#include "id.h"
#include "drivers.h"
#include "AbstractInput.h"

class AbstractDialogTester : public AbstractDialog
{
public:
	AbstractDialogTester() : AbstractDialog(),
		m_fakeDriverKeyPressed(0),
		m_lastKey(0),
		m_processWaitingKeyCalled(false),
		m_processWaitingKeyValue(false),
		m_processIdleCalled(false),
		m_processIdleValue(false)
	{
	}
	~AbstractDialogTester()
	{
	}

	bool GetProcessWaitingKeyCalled() const		{ return m_processWaitingKeyCalled; }
	int GetLastKey() const						{ return m_lastKey; }
	bool GetProcessIdleCalled() const			{ return m_processIdleCalled; }

	void SetProcessWaitingKeyValue(bool value)	{ m_processWaitingKeyValue = value; }
	void SetProcessIdleValue(bool value)		{ m_processIdleValue = value; }
	void SetFakeDriverKeyPressed(int value)		{ m_fakeDriverKeyPressed = value; }

	virtual bool ProcessWaitingKey(int key)
	{
		m_processWaitingKeyCalled = true;
		m_lastKey = key;
		return m_processWaitingKeyValue;
	}
	virtual bool ProcessIdle()
	{
		m_processIdleCalled = true;
		return m_processIdleValue;
	}

protected:
	virtual int driver_key_pressed()		{ return m_fakeDriverKeyPressed; }

private:
	int m_fakeDriverKeyPressed;
	int m_lastKey;
	bool m_processWaitingKeyCalled;
	bool m_processWaitingKeyValue;
	bool m_processIdleCalled;
	bool m_processIdleValue;
};

TEST(AbstractDialog, ProcessInput)
{
	AbstractDialogTester dialog;
	dialog.SetFakeDriverKeyPressed(IDK_ESC);
	dialog.SetProcessWaitingKeyValue(true);
	dialog.SetProcessIdleValue(false);
	dialog.ProcessInput();
	CHECK(dialog.GetProcessWaitingKeyCalled());
	CHECK(!dialog.GetProcessIdleCalled());
	LONGS_EQUAL(IDK_ESC, dialog.GetLastKey());
}

TEST(AbstractDialog, ProcessIdle)
{
	AbstractDialogTester dialog;
	dialog.SetFakeDriverKeyPressed(0);
	dialog.SetProcessWaitingKeyValue(false);
	dialog.SetProcessIdleValue(true);
	dialog.ProcessInput();
	CHECK(!dialog.GetProcessWaitingKeyCalled());
	CHECK(dialog.GetProcessIdleCalled());
}
