#pragma once

class IteratedDynamics
{
public:
	virtual ~IteratedDynamics() { }

	virtual int Main() = 0;
};
