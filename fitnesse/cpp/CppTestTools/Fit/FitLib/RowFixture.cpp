// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>


#include "Platform.h"

#include "RowFixture.h"
#include "RowMap.h"

using namespace std;

void RowFixture::doRows(ParsePtr rows)
{
  try
    {
      bind(rows->parts);
      results = query();
      ParseList parselist = listOf(rows->more);
      match(parselist, results, 0);
      ParsePtr last = rows->last();
      last->more = buildRows(surplus);
      mark(last->more, "surplus");
      mark(missing.begin(), missing.end(), "missing");
    }
  catch(exception& e)
    {
      handleException(rows->leaf(), e);
    }
}


RowFixture::ParseList RowFixture::listOf(ParsePtr rows)
{
  ParseList result;
  while(rows)
    {
      result.push_back(rows.safeGet());
      rows = rows->more;
    }
  return result;
}



void RowFixture::match(ParseList& expected, ObjectList& computed, unsigned int col)
{
  if (col >= columnBindings.size())
    {
      check(expected, computed);
    }
  else if (columnBindings [col] == 0)
    {
      match(expected, computed, col + 1);
    }
  else
    {
      auto_ptr<RowMap<Parse> >    eMap = eSort(expected, col);
      auto_ptr<RowMap<Object> >  cMap = cSort(computed, col);

      set
        <string>		keys = unionOfKeys(*eMap.get(), *cMap.get());

      for (set
           <string>::iterator it = keys.begin(); it != keys.end(); ++it)
        {
          string key = *it;
          ParseList *eList = eMap->getRow(key);
          ObjectList *cList = cMap->getRow(key);

          if (eList == 0)
            {
              addAll<Object>(surplus,cList);
            }
          else if (cList == 0)
            {
              addAll<Parse>(missing,eList);
            }
          else if (eList->size() == 1 && cList->size() == 1)
            {
              check(*eList,*cList);
            }
          else
            {
              match(*eList, *cList, col + 1);
            }
        }
    }
}


ParsePtr RowFixture::buildRows(ObjectList& rows)
{
  Parse		root("", "", 0, 0);
  ParsePtr	next(&root);

  for (unsigned int i = 0; i < rows.size(); i++)
    {
      next = next->more = new Parse("tr", "", buildCells(rows[i]), 0);
    }

  ParsePtr more = root.more;
  root.more = 0;
  return more;
}


void RowFixture::mark(ParsePtr rows, string message)
{
  string annotation = label(message);
  while (rows != 0)
    {
      wrong(rows->parts);
      rows->parts->addToBody(annotation);
      rows = rows->more;
    }
}


auto_ptr<RowMap<Parse> > RowFixture::eSort(ParseList& parses, int col)
{
  RowMap<Parse> *result = new RowMap<Parse>;

  for (ParseList::iterator it = parses.begin();
       it != parses.end();
       ++it)
    {
      Parse *row = *it;
      Parse *cell = row->parts->at(col);
      try
        {
          result->bin(cell->text(), row);
        }
      catch(exception& e)
        {
          handleException(cell, e);
          for (ParsePtr rest = cell->more; rest; rest = rest->more)
            {
              ignore(rest);
            }
        }

    }
  return auto_ptr<RowMap<Parse> >(result);
}


auto_ptr<RowMap<RowFixture::Object> > RowFixture::cSort(ObjectList& object, int col)
{
  RowMap<Object> *result = new RowMap<Object>;

  for (ObjectList::iterator it = object.begin();
       it != object.end();
       ++it)
    {
      Object *row = *it;
      try
        {
          if (columnBindings[col] == 0)
            {
              continue;
            }
          TypeAdapter *a = row->adapterFor(columnBindings[col]->getName());
          string key = a->valueAsString();
          result->bin(key, row);
        }
      catch(exception&)
        {
          surplus.push_back(row);
        }
    }

  return auto_ptr<RowMap<RowFixture::Object> >(result);
}


void RowFixture::check(ParseList& eList, ObjectList& cList)
{
  if (eList.size() == 0)
    {
      addAll<Object>(surplus,&cList);
      return;
    }

  if (cList.size() == 0)
    {
      addAll<Parse>(missing,&eList);
      return;
    }

  ParsePtr row = eList[0];
  eList.erase(eList.begin());

  ParsePtr cell = row->parts;
  Object *object = cList[0];
  cList.erase(cList.begin());

  for (unsigned int i = 0; i < columnBindings.size() && cell; i++)
    {
      TypeAdapter *a = 0;
      if (columnBindings[i] != 0)
        {
          a = object->adapterFor(columnBindings[i]->getName());
        }
      Fixture::check(cell,a);
      cell = cell->more;
    }
}


void RowFixture::mark(ParseList::iterator current, ParseList::iterator end, string message)
{
  string annotation = label(message);
  while (current != end)
    {
      ParsePtr row = *current++;
      wrong(row->parts);
      row->parts->addToBody(annotation);
    }
}


ParsePtr RowFixture::buildCells(Object *row)
{
  if (row == 0)
    {
      ParsePtr nil(new Parse("td", "null", 0, 0));
      nil->addToBody(" colspan=" + columnBindings.size());
      return nil;
    }

  Parse    root("", "", 0, 0);
  ParsePtr next(&root);

  for (unsigned int i = 0; i < columnBindings.size(); i++)
    {
      next = next->more = new Parse("td", "&nbsp", 0, 0);
      if (columnBindings[i] == 0 || !row->hasAdapterFor(columnBindings[i]->getName()))
        {
          ignore(next);
        }
      else
        {
          try
            {
              TypeAdapter *a = row->adapterFor(columnBindings[i]->getName());
              next->body = a->valueAsString();
            }
          catch(exception& e)
            {
              handleException(next, e);
            }

        }

    }

  ParsePtr more = root.more;
  root.more = 0;
  return more;
}


set
  <string> RowFixture::unionOfKeys(const RowMap<Parse>& parses, const RowMap<Object>& objects)
  {
    set
      <string> result;

    parses.addKeysTo(result);
    objects.addKeysTo(result);

    return result;
  }


