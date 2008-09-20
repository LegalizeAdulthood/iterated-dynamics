
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "Parse.h"

#include "ParseException.h"
#include <algorithm>
#include <ctype.h>

using namespace std;


Parse::Parse(const string& text, const vector<string>& tags)
    : parts(0), more(0)
{
  parse(text, tags, 0, 0);
}


Parse::Parse(const string& text)
    : parts(0), more(0)
{
  vector<string> tags;

  tags.push_back("table");
  tags.push_back("tr");
  tags.push_back("td");

  parse(text, tags, 0, 0);
}


Parse::Parse(const string& text,
             const vector<string>& tags,
             int level,
             int offset)
    : parts(0), more(0)
{
  parse(text, tags, level, offset);
}

Parse::Parse(const string& tag, const string& body, ParsePtr parts, ParsePtr more)
    : parts(parts),more(more)
{
  this->leader = "\n";
  this->tag = "<"+tag+">";
  this->body = body;
  this->end = "</"+tag+">";
  this->trailer = "";
}

Parse::~Parse()
{
  parts.free();
  more.free();
}


string Parse::lowercase(const string& text)
{
  string result = text;
  transform(text.begin(), text.end(), result.begin(), (int(*)(int))tolower);
  return result;
}

void Parse::parse(const string& text,
                  const vector<string>& tags,
                  unsigned int level,
                  int offset)
{
  string lc = lowercase(text);
  int startTag = lc.find("<" + tags[level]);
  int endTag = lc.find(">", startTag) + 1;
  int startEnd = lc.find("</" + tags[level], endTag);
  int endEnd = lc.find(">", startEnd) + 1;
  int startMore = lc.find("<" + tags[level], endEnd);

  if (startTag < 0 || endTag < 0 || startEnd < 0 || endEnd < 0)
    {
      throw ParseException("Can't find tag: " + tags[level], offset);
    }

  leader = text.substr(0, startTag);
  tag = text.substr(startTag, endTag - startTag);
  body = text.substr(endTag, startEnd - endTag);
  end = text.substr(startEnd, endEnd - startEnd);
  trailer = text.substr(endEnd);

  if (level+1 < tags.size())
    {
      parts.free();
      parts = new Parse (body, tags, level+1, offset+endTag);
      body = "";
    }

  if (startMore >= 0)
    {
      more.free();
      more = new Parse (trailer, tags, level, offset+endEnd);
      trailer = "";
    }
}


Parse *Parse::at(int i)
{
  return i == 0 || !more ? this : more->at(i - 1);
}


Parse *Parse::at(int i, int j)
{
  return at(i)->parts->at(j);
}


Parse *Parse::at(int i, int j, int k)
{
  return at(i, j)->parts->at(k);
}


int Parse::size() const
  {
    return more == 0 ? 1 : more->size() + 1;
  }


Parse *Parse::leaf()
{
  return parts == 0 ? this : parts->leaf();
}


Parse *Parse::last()
{
  return more == 0 ? this : more->last();
}


string Parse::text() const
  {
    return trim(unescape(unformat(body)));
  }



string Parse::unescape(const string& s)
{
  int startPos = 0;
  string result = s;

  while(true)
    {
      string::size_type begin = result.find("&",startPos);
      string::size_type end = result.find(";",startPos);
      if (begin == string::npos || end == string::npos)
        break;
      int tokenSize = ((end - begin) + 1) - 2;
      string token = result.substr(begin+1,tokenSize);
      if (replacement(token) != "")
        {
          result = result.substr(0,begin) + replacement(token) + result.substr(end+1);
          startPos = begin + replacement(token).size();
        }
      else
        startPos++;
    }

  return result;
}


string Parse::replacement(const string& from)
{
  if (from == "lt")
    return "<";
  else if (from == "gt")
    return ">";
  else if (from == "amp")
    return "&";
  else if (from == "nbsp")
    return " ";
  else
    return "";
}

string Parse::unformat(const string& s)
{
  int startPos = 0;
  string result = s;

  while(true)
    {
      string::size_type begin = result.find("<",startPos);
      string::size_type end = result.find(">",startPos);
      if (begin == string::npos || end == string::npos)
        break;
      result = result.substr(0,begin) + result.substr(end+1);
    }

  return result;
}


string Parse::trim(const string& s)
{
  string result = s;
  while(result.size() > 0 && isspace(result[0]))
    result = result.substr(1);
  while(result.size() > 0 && isspace(result[result.size()-1]))
    result = result.substr(0, result.size() -1);
  return result;
}

void Parse::addToTag(const string& newText)
{
  int last = tag.length()-1;
  tag = tag.substr(0,last) + newText + ">";
}


void Parse::addToBody(const string& newText)
{
  body += newText;
}


void Parse::print(string& out)
{
  out += leader;
  out += tag;
  if (parts)
    {
      parts->print(out);
    }
  else
    {
      out += body;
    }
  out += end;
  if (more)
    {
      more->print(out);
    }
  else
    {
      out += trailer;
    }
}


