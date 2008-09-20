#include "EventLogOutput.h"
#include <iostream>
#include <algorithm>

using namespace std;

EventLogOutput::EventLogOutput()
{
}

namespace
{
  void deleteIt(Fixture* f) {delete f;}
}

EventLogOutput::~EventLogOutput()
{
  for_each(rows.begin(), rows.end(), deleteIt);
}



RowFixture::ObjectList EventLogOutput::query() const
{
  int sequence = 1;
  rows.push_back(new EventHolder(sequence++, "dummy event", 42));
  rows.push_back(new EventHolder(sequence++, "another dummy", 42));
  rows.push_back(new EventHolder(sequence++, "another dummy", 42));
  rows.push_back(new EventHolder(sequence++, "another dummy", 42));
  rows.push_back(new EventHolder(sequence++, "another dummy", 42));
	return rows;
}

const Fixture *EventLogOutput::getTargetClass() const
{
	return &exampleEventHolder;
}

