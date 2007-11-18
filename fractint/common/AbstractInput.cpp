#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

#include "drivers.h"
#include "fihelp.h"
#include "intro.h"
#include "realdos.h"

#include "AbstractInput.h"


void AbstractDialog::ProcessInput()
{
	bool done = false;
	while (!done)
	{
		int key = driver_key_pressed();
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

int AbstractDialog::driver_key_pressed()
{
	return ::driver_key_pressed();
}

