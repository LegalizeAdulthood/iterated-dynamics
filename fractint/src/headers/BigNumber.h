#if !defined(BIG_NUMBER_H)
#define BIG_NUMBER_H

#include "big.h"

class BigNumber
{
public:
	explicit BigNumber(const BigNumber &rhs) : _storage(new BYTE[rhs._digits]), _digits(rhs._digits)
	{
		std::copy(&rhs._storage[0], &rhs._storage[_digits], _storage);
	}
	explicit BigNumber(int digits) : _storage(new BYTE[digits]), _digits(digits)
	{
		std::fill(&_storage[0], &_storage[_digits], 0);
	}
	~BigNumber()
	{
		delete[] _storage;
	}

	BYTE get8(int idx = 0) const			{ return _storage[idx]; }
	S8 getS8(int idx = 0) const				{ return S8(get8(idx)); }
	void set8(int idx, BYTE value)			{ _storage[idx] = value; }
	void set8(BYTE value)					{ set8(0, value); }

	U16 get16(int idx = 0) const
	{
		BYTE const *addr = &_storage[idx];
		return U16(addr[0]) | U16(addr[1] << 8);
	}
	S16 getS16(int idx = 0) const
	{
		BYTE const *addr = &_storage[idx];
		return S16(addr[0]) | S16(addr[1] << 8);
	}
	U16 set16(int idx, U16 value)
	{
		BYTE *addr = &_storage[idx];
		addr[0] = BYTE(value & 0xff);
		addr[1] = BYTE((value >> 8) & 0xff);
		return value;
	}
	U16 set16(U16 value)					{ return set16(0, value); }
	S16 setS16(int idx, S16 value)
	{
		BYTE *addr = &_storage[idx];
		addr[0] = BYTE(value & 0xff);
		addr[1] = BYTE((value >> 8) & 0xff);
		return value;
	}
	S16 setS16(S16 value)					{ return setS16(0, value); }

	U32 get32(int idx = 0) const
	{
		BYTE const *addr = &_storage[idx];
		return addr[0] | U32(addr[1] << 8) | U32(addr[2] << 16) | U32(addr[3] << 24);
	}
	U32 set32(long value)
	{
		return set32(0, value);
	}
	U32 set32(int idx, long value)
	{
		BYTE *addr = &_storage[idx];
		addr[0] = BYTE(value & 0xff);
		addr[1] = BYTE((value >> 8)  & 0xff);
		addr[2] = BYTE((value >> 16) & 0xff);
		addr[3] = BYTE((value >> 24) & 0xff);
		return value; 
	}

	void clear();
	void maximum();

private:
	BYTE *_storage;
	int _digits;
};

#endif
