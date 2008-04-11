#if !defined(ENDIAN_H)
#define ENDIAN_H

#include <ostream>

template <bool littleEndian>
class LittleEndian
{
public:
	std::ostream &write(std::ostream &stream, int value);
};

template <>
class LittleEndian<true>
{
public:
	std::ostream &write(std::ostream &stream, int value)
	{
		return stream.write(reinterpret_cast<char *>(&value), sizeof(int));
	}
};

#endif
