#include "SimpleString.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const int DEFAULT_SIZE = 20;

SimpleString::SimpleString()
	: m_buffer(new char[1])
{
	m_buffer[0] = '\0';
}

SimpleString::SimpleString(const char *otherBuffer)
	: m_buffer(new char[strlen(otherBuffer) + 1])
{
	strcpy(m_buffer, otherBuffer);
}

SimpleString::SimpleString(const SimpleString &other)
	: m_buffer(new char[other.size() + 1])
{
	strcpy(m_buffer, other.m_buffer);
}

SimpleString SimpleString::operator=(const SimpleString &other)
{
	delete[] m_buffer;
	m_buffer = new char[other.size() + 1];
	strcpy(m_buffer, other.m_buffer);
	return *this;
}

char *SimpleString::asCharString() const
{
	return m_buffer;
}

int SimpleString::size() const
{
	return int(strlen(m_buffer));
}

SimpleString::~SimpleString()
{
	delete[] m_buffer;
}

bool operator==(const SimpleString &left, const SimpleString &right)
{
	return !strcmp(left.asCharString(), right.asCharString());
}

SimpleString StringFrom(bool value)
{
	const char *values[] = { "false", "true" };
	return SimpleString(values[value ? 1 : 0]);
}

SimpleString StringFrom(const char *value)
{
	return SimpleString(value);
}

SimpleString StringFrom(long value)
{
	char buffer[DEFAULT_SIZE];
	sprintf(buffer, "%ld", value);
	return SimpleString(buffer);
}

SimpleString StringFrom(double value)
{
	char buffer[DEFAULT_SIZE];
	sprintf(buffer, "%lf", value);
	return SimpleString(buffer);
}

SimpleString StringFrom(const SimpleString &value)
{
	return SimpleString(value);
}
