//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Runnable_h
#define D_Runnable_h

class Runnable
  {

  public:
    Runnable();

    virtual ~Runnable();

    virtual void run() = 0;

  private:
    Runnable(const Runnable&);
    Runnable& operator=(const Runnable&);

  };

#endif  // D_Runnable_h


