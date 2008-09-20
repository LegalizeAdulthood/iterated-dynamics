//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Delay.h"
#include <iostream>
#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

static int nanoSleep(long ns)
{
	struct timespec sleepTime;;
	
	sleepTime.tv_nsec = ns;
	sleepTime.tv_sec = 0;
    return nanosleep(&sleepTime, 0);	
}

void Delay::ms(long time)
{	
	nanoSleep(time * 1000000);
}

void Delay::yield()
{
    sched_yield();
}

