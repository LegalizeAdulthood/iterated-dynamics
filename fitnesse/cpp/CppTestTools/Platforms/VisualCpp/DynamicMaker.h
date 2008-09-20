
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.


//Loading ActionFixture from a DLL can cause memory leaks.  It appears that
//loading through a DLL results in multiple instances of static variables
//USer beware.
#ifndef DYNAMICMAKER_H
#define DYNAMICMAKER_H

#include "Fit/FixtureMaker.h"

#include <string>
#include <windows.h>
#include <map>

using namespace std;


class DynamicMaker : public FixtureMaker
  {
  public:
    virtual Fixture				*make(const string& name);

    DynamicMaker();
    virtual						~DynamicMaker();


  private:
    map<string,HINSTANCE>		libraryHandles;

    HINSTANCE					getLibraryHandle(const string& libraryName);

    HINSTANCE					loadLibrary(const string& libraryName);
    void						unloadLibrary(HINSTANCE handle);


    static string	libraryName(const string& name);
    static string	fixtureName(const string& name);


  };

#endif
