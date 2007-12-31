#include "port.h"
#include "id.h"

#include "globals.h"

Globals g_;

Globals::Globals()
	: _adapter(0),
	_initialAdapter(0)
{
	VIDEOINFO zero = { 0 };
	for (int i = 0; i < NUM_OF(_videoTable); i++)
	{
		_videoTable[i] = zero;
	}
}

Globals::~Globals()
{
}
