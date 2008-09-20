
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef DOMAINOBJECTWRAPPER_H
#define DOMAINOBJECTWRAPPER_H

#include "MemberAdapter.h"
#include "Parse.h" // for ParsePtr MCF
#include "Fixture.h"

#define PUBLISH_MEMBER_OF(testClass,type,name,host)\
		adapters.push_back(new MemberAdapter<testClass,type>(#name, &testClass::name, host));

#include <vector>
#include <string>
#include <map>

class TypeAdapter;

template<typename DOMAINCLASS>
class DomainObjectWrapper : public Fixture
  {
  public:
    DOMAINCLASS					*object;

    DomainObjectWrapper(DOMAINCLASS *o) : object(o)
    {}

  private:
    DomainObjectWrapper(const DomainObjectWrapper&);
    DomainObjectWrapper&		operator=(DomainObjectWrapper&);


  };


#endif
