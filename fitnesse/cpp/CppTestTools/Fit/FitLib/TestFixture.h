
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef TESTFIXTURE_H
#define TESTFIXTURE_H

#include "ColumnFixture.h"
#include "TestFixtureMaker.h"

using namespace std;


class TestFixture : public ColumnFixture
  {
  public:
    int sampleInt;
    int arg1;
    int arg2;
    string stringInput;
    int invokeCount;

    TestFixtureMaker maker;

    int sum()
    {
      return arg1 + arg2;
    }

    string stringOutput()
    {
      invokeCount++;
      return stringInput;
    }

    TestFixture() : arg1(3), arg2(4) , stringInput(""), invokeCount(0)
    {
      PUBLISH(TestFixture,int,sampleInt);
      PUBLISH(TestFixture,int,arg1);
      PUBLISH(TestFixture,int,arg2);
      PUBLISH(TestFixture,int,sum);
      PUBLISH(TestFixture,string,stringInput);
      PUBLISH(TestFixture,string,stringOutput);

      setMaker(&maker);
    }

  };

#endif
