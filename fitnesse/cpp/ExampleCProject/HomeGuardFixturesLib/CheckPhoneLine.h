//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_CheckPhoneLine_H
#define D_CheckPhoneLine_H


#include "Fit/ColumnFixture.h"
#include "HomeGuardInit.h"


class CheckPhoneLine : public ColumnFixture
{
public:
  CheckPhoneLine()
  {
    PUBLISH(CheckPhoneLine,std::string,lastNumberDialed);
  }

	void doRow(ParsePtr row) 
  {
    ColumnFixture::doRow(row);
  }

  std::string lastNumberDialed() 
  {
  	return "none"; 
  	}

};

#endif

