#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

#include "drivers.h"
#include "fihelp.h"
#include "intro.h"
#include "realdos.h"

#include "AbstractInput.h"

AbstractDialog::AbstractDialog(AbstractDriver *driver) : _driver(driver)
{
}

void AbstractDialog::ProcessInput()
{
	bool done = false;
	while (!done)
	{
		int key = _driver->key_pressed();
		if (key)
		{
			done = ProcessWaitingKey(key);
		}
		else
		{
			done = ProcessIdle();
		}
	}
}
