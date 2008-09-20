
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include "DynamicMaker.h"

#include <windows.h>

#include "Fit/ResolutionException.h"

DynamicMaker::DynamicMaker()
{}

DynamicMaker::~DynamicMaker()
{
  for(map<string,HINSTANCE>::iterator it = libraryHandles.begin(); it != libraryHandles.end(); ++it)
    {
      unloadLibrary(it->second);
    }
}


HINSTANCE DynamicMaker::loadLibrary(const string& libraryName)
{

  HINSTANCE handle = LoadLibrary(libraryName.c_str());

  if (handle == 0)
    throw ResolutionException("Unable to load \""
                              + libraryName
                              + "\"");

  return handle;
}


void DynamicMaker::unloadLibrary(HINSTANCE handle)
{
  FreeLibrary(handle);
}


HINSTANCE DynamicMaker::getLibraryHandle(const string& libraryName)
{
  if (libraryHandles.find(libraryName) == libraryHandles.end())
    libraryHandles[libraryName] = LoadLibrary(libraryName.c_str());
  return libraryHandles[libraryName];
}

Fixture *DynamicMaker::make(const string& name)
{
  typedef Fixture *(*MAKER)(const char *);

  string		libraryName		= splitName(name).first;
  string		fixtureName		= splitName(name).second;
  HINSTANCE	library			= getLibraryHandle(libraryName);

  MAKER maker = (MAKER)GetProcAddress(library, "fixtureMaker");

  if (maker == 0)
    throw ResolutionException("Unable to find entry point in \""
                              + libraryName
                              + "\"");

  Fixture *fixture = maker(fixtureName.c_str());

  if (fixture == 0)
    throw ResolutionException("Unable to find fixture \""
                              + fixtureName
                              + "\" in \""
                              + libraryName
                              + "\"");

  return fixture;
}


