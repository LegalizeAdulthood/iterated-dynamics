#pragma once

class GlobalImpl;

class ColormapTable
{
public:
	ColormapTable()
	{
		for (int i = 0; i < 256; i++)
		{
			_map[i][0] = _map[i][1] = _map[i][2] = 0;
		}
	}
	ColormapTable(const ColormapTable &rhs)
	{
		for (int i = 0; i < 256; i++)
		{
			_map[i][0] = rhs._map[i][0];
			_map[i][1] = rhs._map[i][1];
			_map[i][2] = rhs._map[i][2];
		}
	}
	~ColormapTable()
	{
	}

	int Red(int i) const				{ return _map[i][0]; }
	int Green(int i) const				{ return _map[i][1]; }
	int Blue(int i) const				{ return _map[i][2]; }
	void Set(int i, BYTE r, BYTE g, BYTE b)
	{
		_map[i][0] = r;
		_map[i][1] = g;
		_map[i][2] = b;
	}
	BYTE Channel(int idx, int chan)
	{
		return _map[idx][chan];
	}
	void SetChannel(int idx, int chan, BYTE value)
	{
		_map[idx][chan] = value;
	}
	void SetRed(int i, BYTE value)		{ _map[i][0] = value; }
	void SetGreen(int i, BYTE value)	{ _map[i][1] = value; }
	void SetBlue(int i, BYTE value)		{ _map[i][2] = value; }
	void Clear()
	{
		for (int i = 0; i < 256; i++)
		{
			_map[i][0] = _map[i][1] = _map[i][2] = 0;
		}
	}

	friend bool operator==(const ColormapTable &lhs, const ColormapTable &rhs);

private:
	BYTE _map[256][3];
};

inline bool operator==(const ColormapTable &lhs, const ColormapTable &rhs)
{
	for (int i = 0; i < 256; i++)
	{
		if ((lhs._map[i][0] != rhs._map[i][0])
			|| (lhs._map[i][1] != rhs._map[i][1])
			|| (lhs._map[i][2] != rhs._map[i][2]))
		{
			return false;
		}
	}
	return true;
}

inline bool operator!=(const ColormapTable &lhs, const ColormapTable &rhs)
{
	return !(lhs == rhs);
}

extern ColormapTable g_dac_box;
extern ColormapTable *g_map_dac_box;
extern ColormapTable g_old_dac_box;
extern int g_save_dac;
extern int g_dac_count;
extern bool g_got_real_dac;					/* loaddac worked, really got a dac */

class Globals
{
public:
	Globals();
	~Globals();

	int Adapter() const;
	void SetAdapter(int value);
	int InitialAdapter() const;
	void SetInitialAdapter(int value);
	void SetInitialAdapterNone();
	const VIDEOINFO &VideoEntry() const;
	void SetVideoEntry(const VIDEOINFO &value);
	void SetVideoEntrySize(int width, int height);
	void SetVideoEntryXDots(int value);
	void SetVideoEntryColors(int value);
	int VideoTableLength() const;
	void AddVideoModeToTable(const VIDEOINFO &mode);
	const VIDEOINFO &VideoTable(int i) const;
	void SetVideoTableKey(int i, int key);
	void SetVideoTable(int i, const VIDEOINFO &value);
	bool GoodMode() const;
	void SetGoodMode(bool value);

private:
	GlobalImpl *_impl;
};

extern Globals g_;
