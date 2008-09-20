//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

///////////////////////////////////////////////////////////////////////////////
//
// SIMPLESTRINGEXTENSIONS.H
//
// One of the design goals of CppUnitLite is to compilation with very old C++
// compilers.  For that reason, the simple string class that provides
// only the operations needed in UnitTestHarness.  If you want to extend
// SimpleString for your types, you can write a StringFrom function like these
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLE_STRING_EXTENSIONS
#define SIMPLE_STRING_EXTENSIONS

#include "UnitTestHarness/SimpleString.h"
#include <string>

SimpleString StringFrom (const std::string& other);

#endif
