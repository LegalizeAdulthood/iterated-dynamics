#pragma once

class Globals
{
public:
	Globals();
	~Globals();

	int Adapter() const					{ return _adapter; }
	void SetAdapter(int value)			{ _adapter = value; }
	int InitialAdapter() const			{ return _initialAdapter; }
	void SetInitialAdapter(int value)	{ _initialAdapter = value; }

private:
	int _adapter;
	int _initialAdapter;
};

extern Globals g_;
