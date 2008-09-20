//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Mutex.h"
#include <pthread.h>

struct MutexInnards
  {
public:
    pthread_mutex_t mutex;
    int nestLevel;
  };


Mutex::Mutex()
{
  pthread_mutexattr_t attr;
    
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    
  innards = new MutexInnards();
  innards->nestLevel = 0;
  pthread_mutex_init(&innards->mutex, &attr);
}

Mutex::~Mutex()
{
  pthread_mutex_destroy(&innards->mutex);
  delete innards;
}

void Mutex::acquire()
{
  pthread_mutex_lock(&innards->mutex);
//  innards->nestLevel++;
}

void Mutex::release()
{
//  innards->nestLevel--;
//  if (innards->nestLevel <= 0)
//    {
//      innards->nestLevel = 0;
      pthread_mutex_unlock(&innards->mutex);
//    }
}



