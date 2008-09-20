//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Delay_h
#define D_Delay_h

#include "Delay.h"

class Delay
  {

  public:
    static void ms(long time);

    //do the shortest delay that will cause a context switch withing the same priority
    static void yield();

  private:

    Delay();
    ~Delay();
    Delay(const Delay&);
    Delay& operator=(const Delay&);

  };

#endif  // D_Delay_h


