// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef HURLINGPOINTER_H
#define HURLINGPOINTER_H

#include "IllegalAccessException.h"

template<typename AType>
class HurlingPointer
  {
  public:
    HurlingPointer(AType *pointer)
        : pointer(pointer)
    {}

    operator bool() const
      {
        return pointer != 0;
      }

    AType *operator->()
    {
      return safeGet();
    }

    AType const* operator->() const
      {
        return safeGet();
      }

    AType *safeGet() const
      {
        if (pointer == 0)
          throw IllegalAccessException("bad dereference");
        return pointer;
      }

    void free()
    {
      delete pointer;
      pointer = 0;
    }

    AType *pointer;
  };


#endif
