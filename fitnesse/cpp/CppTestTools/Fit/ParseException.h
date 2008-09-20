
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef PARSEEXCEPTION_H
#define PARSEEXCEPTION_H

class ParseException : public exception
  {
  public:
    ParseException(const string& text, int offset)
        : message(text), offset(offset)
    {}

    virtual ~ParseException() throw()
    {}

    virtual const char *what() const throw()
    {
      return message.c_str();
    }

    virtual int getErrorOffset() const
      {
        return offset;
      }

  private:
    string message;
    int	   offset;
  };

#endif
