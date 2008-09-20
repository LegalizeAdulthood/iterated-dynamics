
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef PARSE_H
#define PARSE_H


#include "HurlingPointer.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

class Parse;

typedef HurlingPointer<Parse>	ParsePtr;


class Parse
  {
  public:

    string		leader;
    string		tag;
    string		body;
    string		trailer;
    string		end;

    ParsePtr	parts;
    ParsePtr	more;

    Parse(const string& text, const vector<string>& tags);
    Parse(const string& text);
    Parse(const string& text, const vector<string>& tags, int level, int offset);
    Parse(const string& tag, const string& body, ParsePtr parts, ParsePtr more);

    virtual		~Parse();

    void		parse(const string& text, const vector<string>& tags, unsigned int level, int offset);

    virtual Parse		*at(int i);
    virtual Parse		*at(int i, int j);
    virtual Parse		*at(int i, int j, int k);

    virtual int			size() const;
    virtual Parse		*leaf();
    virtual Parse		*last();

    virtual string		text() const;


    void		addToTag(const string& newText);
    void		addToBody(const string& newText);
    void		print(string& out);


    static string unescape(const string& text);
    static string unformat(const string& text);
    static string trim(const string& text);
    static string lowercase(const string& text);
    static string replacement(const string& text);

  private:
    Parse(const Parse&);
    Parse&		operator=(Parse&);


  };

#endif
