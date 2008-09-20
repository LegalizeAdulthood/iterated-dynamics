
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef RESOLUTIONEXCEPTION_H
#define RESOLUTIONEXCEPTION_H

#include <string>

class ResolutionException : public exception
  {
  public:
    ResolutionException(const string& text)
        : message(text)
    {}

    virtual ~ResolutionException() throw()
    {}

    virtual const char *what() const throw()
    {
      return message.c_str();
    }

  private:
    string message;

  };

#endif
