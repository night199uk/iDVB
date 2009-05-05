/*
 *  IDVBEventSource.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 19/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBEvent.h"
#include <poll.h>

using namespace Video4Mac;

IDVBEventListener::IDVBEventListener()
{
}

int DoubleToTimeSpec(double timeout)
{
	struct timespec ts;
	struct timeval tp;
	int rc;
	if (timeout < 0) {					/* Negative time? */
		ts.tv_sec = 0 ;  ts.tv_nsec = 0 ;
	} else if (timeout > (double) LONG_MAX) {		/* Time too large? */
		fprintf(stderr, "timeout too long.\n");
		ts.tv_sec = LONG_MAX ;  ts.tv_nsec = 999999999L ;
	} else {						/* Valid time. */
		ts.tv_sec = (time_t) timeout;
		ts.tv_nsec = (long) ((timeout - (double) ts.tv_sec)
							 * 1000000000.0) ;
	}
	
	rc = gettimeofday(&tp, NULL);
	
	ts.tv_sec += tp.tv_sec;
	ts.tv_nsec += tp.tv_usec * 1000;
	
	while (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++ ;  ts.tv_nsec = ts.tv_nsec - 1000000000L ;
	}
}

int IDVBCondition::TimedWait(unsigned int timeout)
{	
	struct timespec ts;
	struct timeval tp;
	int rc;
	
	ts.tv_sec = (time_t)(timeout / 1000); 
	ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
	rc = gettimeofday(&tp, NULL);
	
	ts.tv_sec += tp.tv_sec;
	ts.tv_nsec += tp.tv_usec * 1000;
	
	while (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++ ;  ts.tv_nsec = ts.tv_nsec - 1000000000L ;
	}
	
	return pthread_cond_timedwait(&m_Cond, &m_Mutex, &ts); 
}

unsigned int IDVBEventListener::Poll(DVBPollDescriptor* poll_list, int num_pds, unsigned int timeout)
{
	unsigned int count;
	IDVBCondition *pt;
	bool timed_out;
	count = 0;
	timed_out = false;
	
	m_Cond.Lock();
	

	pt = &m_Cond;
	for (;;)
	{
		for (int i = 0; i < num_pds; i++)
		{
			if (PollPD(&poll_list[i], &m_Cond))
			{
				count++;
				pt = NULL;
			}
		}
		pt = NULL;
		if (count || timed_out)
			break;
		
		int test = 0;
		test = m_Cond.TimedWait(timeout);
		if (test == ETIMEDOUT)
			timed_out = true;
	}

	m_Cond.Unlock();
	return count;
};

inline unsigned int IDVBEventListener::PollPD(DVBPollDescriptor* pollpd, IDVBCondition* cond)
{
	unsigned int mask;

	mask = 0;
	if (pollpd->Source)
	{
		mask = pollpd->Source->Poll(cond);
		mask &= pollpd->Events | POLLERR | POLLHUP;
	}
	pollpd->REvents = mask;
	
	return mask;
}

unsigned int IDVBEventSource::Poll(IDVBCondition *Condition)
{
	return POLLERR;
}

void IDVBEventSource::PollWait(IDVBCondition *Condition)
{
	m_WaitQueue.Add(Condition);
}
