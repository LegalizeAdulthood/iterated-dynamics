#pragma once

class Globals
{
public:
	Globals();
	~Globals();

	int Adapter() const					{ return _adapter; }
	void SetAdapter(int value)			{ _adapter = value; }

private:
	int _adapter;
};

extern Globals g_;
