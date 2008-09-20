//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_TestInstaller_h
#define D_TestInstaller_h

///////////////////////////////////////////////////////////////////////////////
//
//  TestInstaller.h
//
//  This class is used by the TEST macros to install
//  tests into the list of tests.  It is designed to
//  create file scope class instances that install tests
//  in the test list before main runs
//
///////////////////////////////////////////////////////////////////////////////

class Utest;

class TestInstaller
  {
  public:
    explicit TestInstaller(Utest*);
    virtual ~TestInstaller();

    void unDo();

  private:

    TestInstaller(const TestInstaller&);
    TestInstaller& operator=(const TestInstaller&);

  };

#endif  // D_TestInstaller_h
