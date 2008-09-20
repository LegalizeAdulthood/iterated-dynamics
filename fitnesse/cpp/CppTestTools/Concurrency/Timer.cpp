#include "Timer.h"
#include "Action.h"

Timer::Timer()
    : itsEntry(0)
{}

struct TimeEntry
  {
    TimeEntry(unsigned long period, Action* callback)
        : period(period), currentCount(0), callback(callback)
    {}
    ~TimeEntry()
    {
      delete callback;
    }
    unsigned long period;
    unsigned long currentCount;
    Action* callback;
  };

Timer::~Timer()
{
  delete itsEntry;
}

void Timer::notifyPeriodically(long period, Action* callback)
{
  delete itsEntry;
  itsEntry = new TimeEntry(period, callback);
}

void Timer::tic()
{
  if (itsEntry)
    {
      itsEntry->currentCount++;
      if (itsEntry->currentCount == itsEntry->period)
        {
          itsEntry->currentCount = 0;
          itsEntry->callback->execute();
        }
    }
}

