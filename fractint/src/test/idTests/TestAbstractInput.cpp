#include "stdafx.h"

#include <boost/test/unit_test.hpp>

#include "id.h"
#include "drivers.h"
#include "AbstractInput.h"
#include "FakeDriver.h"

class AbstractDialogTester : public AbstractDialog
{
public:
	AbstractDialogTester(AbstractDriver *driver) : AbstractDialog(driver),
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

private:
	int m_lastKey;
	bool m_processWaitingKeyCalled;
	bool m_processWaitingKeyValue;
	bool m_processIdleCalled;
	bool m_processIdleValue;
};

BOOST_AUTO_TEST_CASE(AbstractInput_ProcessInput)
{
	FakeDriver driver;
	driver.SetKeyStroke(IDK_ESC);
	AbstractDialogTester dialog(&driver);
	dialog.SetProcessWaitingKeyValue(true);
	dialog.SetProcessIdleValue(false);
	dialog.ProcessInput();
	BOOST_CHECK(dialog.GetProcessWaitingKeyCalled());
	BOOST_CHECK(!dialog.GetProcessIdleCalled());
	BOOST_CHECK(IDK_ESC == dialog.GetLastKey());
}

BOOST_AUTO_TEST_CASE(AbstractInput_ProcessIdle)
{
	FakeDriver driver;
	driver.SetKeyStroke(0);
	AbstractDialogTester dialog(&driver);
	dialog.SetProcessWaitingKeyValue(false);
	dialog.SetProcessIdleValue(true);
	dialog.ProcessInput();
	BOOST_CHECK(!dialog.GetProcessWaitingKeyCalled());
	BOOST_CHECK(dialog.GetProcessIdleCalled());
}
