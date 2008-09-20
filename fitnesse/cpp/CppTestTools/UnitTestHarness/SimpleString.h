//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

///////////////////////////////////////////////////////////////////////////////
//
// SIMPLESTRING.H
//
// One of the design goals of CppUnitLite is to compilation with very old C++
// compilers.  For that reason, the simple string class that provides
// only the operations needed in CppUnitLite.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLE_STRING
#define SIMPLE_STRING


class SimpleString
  {
    friend bool	operator== (const SimpleString& left, const SimpleString& right);
    friend bool	operator!= (const SimpleString& left, const SimpleString& right);

  public:
    SimpleString ();
    SimpleString (const char *value);
    SimpleString (const SimpleString& other);
    ~SimpleString ();

    SimpleString& operator= (const SimpleString& other);
    bool contains(const SimpleString& other) const;
    SimpleString operator+(const SimpleString&);
    SimpleString& operator+=(const SimpleString&);
    SimpleString& operator+=(const char*);

    const char *asCharString () const;
    int	size() const;

  private:
    char *buffer;
  };



SimpleString StringFrom (bool value);
SimpleString StringFrom (const char *value);
SimpleString StringFrom (long value);
SimpleString StringFrom (double value);
SimpleString StringFrom (const SimpleString& other);

#endif
