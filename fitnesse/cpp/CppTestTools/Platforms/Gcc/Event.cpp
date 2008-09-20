//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Event.h"
#include <pthread.h>

using namespace std;

struct EventInnards
  {
    pthread_cond_t condition;
    pthread_mutex_t mutex;
    bool hasOccurred;
  };

Event::Event()
{
  innards = new EventInnards;
  pthread_cond_init(&innards->condition, 0);
  pthread_mutex_init(&innards->mutex, 0);
  innards->hasOccurred = false;
}

Event::~Event()
{
  pthread_cond_destroy(&innards->condition);
  pthread_mutex_destroy(&innards->mutex);
  delete innards;
}


void Event::signal()
{
  pthread_mutex_lock(&innards->mutex);
  innards->hasOccurred = true;
  pthread_cond_signal(&innards->condition);
  pthread_mutex_unlock(&innards->mutex);
}


void Event::wait()
{
  pthread_mutex_lock(&innards->mutex);
  if (!innards->hasOccurred)
    {
      pthread_cond_wait(&innards->condition, &innards->mutex);
    }

  innards->hasOccurred = false;
  pthread_mutex_unlock(&innards->mutex);
}

bool Event::isSignaled()
{
  return innards->hasOccurred;;
}

