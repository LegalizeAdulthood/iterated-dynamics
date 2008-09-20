

// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef COLUMNFIXTURE_H
#define COLUMNFIXTURE_H

#include "Fixture.h"


class ColumnFixture : public Fixture
  {
  public:
    virtual					~ColumnFixture();

    void					doRows(ParsePtr row);
    void					doRow(ParsePtr row);
    virtual void  reset();  //called before each row
    virtual void  execute();//called after each row
    void					doCell(ParsePtr cell, int column);
    void					bind (ParsePtr heads);
    const virtual Fixture	*getTargetClass() const;

    vector<TypeAdapter *>	columnBindings;

  private:
    TypeAdapter* bindFunctionCall(string& name, string& suffix);
  };

#endif
