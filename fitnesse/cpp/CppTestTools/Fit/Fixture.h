// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef FIXTURE_H
#define FIXTURE_H

#include "MemberAdapter.h"
#include "Parse.h" // for ParsePtr MCF

#define PUBLISH(testClass,type,name)\
		adapters.push_back(new MemberAdapter<testClass,type>(#name, &testClass::name, this));

#include <vector>
#include <string>
#include <map>

using std::vector;
using std::string;
using std::map;

class TypeAdapter;
class FixtureMaker;

class Fixture
  {
  public:
    Fixture();
    virtual						~Fixture();

    virtual void				doTables(ParsePtr tables);

    virtual TypeAdapter			*adapterFor(const string& member) const;
    virtual bool				hasAdapterFor(const string& member) const;

    virtual Fixture				*makeFixture(const string& name) const;

    virtual void				doTable(ParsePtr table);
    virtual void				doRows(ParsePtr rows);
    virtual void				doRow(ParsePtr row);
    virtual void				doCells(ParsePtr cells);
    virtual void				doCell(ParsePtr cell, int columnNumber);

    virtual void				handleException(ParsePtr cell, exception& e);
    virtual void				check(ParsePtr cell, TypeAdapter *a);

    virtual void				setMaker(FixtureMaker *maker);

    void					ignore(ParsePtr cell);
    void					right(ParsePtr cell);
    void					wrong(ParsePtr cell);
    void					wrong(ParsePtr cell, string actual);
    void					color(ParsePtr cell, string colorCode);

    string				label(string s);
    string				escape(string s);
    string				escape(string s, char from, string to);
    bool					endsWith(string s, string suffix);
    string				counts();
    string        friendlyName(const string&) const;
    string        friendlyFixtureName(const string&) const;

    static string				currentTime();



    static const string			rightColor;
    static const string			wrongColor;
    static const string			exceptionColor;
    static const string			ignoreColor;

    static int fixtureCount;

    static map<string,string>	summary;

    vector<TypeAdapter *>		adapters;
    FixtureMaker				*maker;
    int                 rights;
    int                 wrongs;
    int                 ignores;
    int                 exceptions;


  private:
    Fixture(const Fixture&);
    Fixture&					operator=(Fixture&);


  };


#endif
