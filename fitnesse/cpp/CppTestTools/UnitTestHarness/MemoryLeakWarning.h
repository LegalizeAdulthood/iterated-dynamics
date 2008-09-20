//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_EnableMemoryLeakWarning_h
#define D_EnableMemoryLeakWarning_h

///////////////////////////////////////////////////////////////////////////////
//
//  MemoryLeakWarning.h
//
//  MemoryLeakWarning defines the inteface to a platform specific
//  memory leak detection class.  See Platforms directory for examples
//
///////////////////////////////////////////////////////////////////////////////


class MemoryLeakWarning
  {
  public:
    static void Enable();
    static const char*  FinalReport();
    static void CheckPointUsage();
    static void IgnoreLeaks(int n);
    static bool UsageIsNotBalanced();
    static const char* Message();

  private:
  };

#define IGNORE_N_LEAKS(n) MemoryLeakWarning::IgnoreLeaks(n);

#endif
