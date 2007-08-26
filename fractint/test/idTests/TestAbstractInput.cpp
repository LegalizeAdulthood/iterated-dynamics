#include "stdafx.h"

#include "fractint.h"
#include "drivers.h"
#include "AbstractInput.h"

int driver_key_pressed()
{
	return 0;
}
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

TEST(ProcessInput, AbstractDialog)
{
	AbstractDialogTester dialog;
	dialog.SetFakeDriverKeyPressed(FIK_ESC);
	dialog.SetProcessWaitingKeyValue(true);
	dialog.SetProcessIdleValue(false);
	dialog.ProcessInput();
	CHECK(dialog.GetProcessWaitingKeyCalled());
	CHECK(!dialog.GetProcessIdleCalled());
	LONGS_EQUAL(FIK_ESC, dialog.GetLastKey());
}

TEST(ProcessIdle, AbstractDialog)
{
	AbstractDialogTester dialog;
	dialog.SetFakeDriverKeyPressed(0);
	dialog.SetProcessWaitingKeyValue(false);
	dialog.SetProcessIdleValue(true);
	dialog.ProcessInput();
	CHECK(!dialog.GetProcessWaitingKeyCalled());
	CHECK(dialog.GetProcessIdleCalled());
}
