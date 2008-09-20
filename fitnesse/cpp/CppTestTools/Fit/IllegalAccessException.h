
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef ILLEGALACCESSEXCEPTION_H
#define ILLEGALACCESSEXCEPTION_H

#include <string>

using std::string;
using std::exception;

class IllegalAccessException : public exception
  {
  public:
    IllegalAccessException(const string& text)
        : message(text)
    {}

    virtual ~IllegalAccessException() throw()
    {}


    virtual const char *what() const throw()
    {
      return message.c_str();
    }

  private:
    string message;
  };

#endif
