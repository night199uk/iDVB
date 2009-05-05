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


unsigned int IDVBEventListener::Poll(DVBPollDescriptor* poll_list, int num_pds, unsigned int timeout)
{
	unsigned int count;
	CDVBCondition *pt;
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

inline unsigned int IDVBEventListener::PollPD(DVBPollDescriptor* pollpd, CDVBCondition* cond)
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

unsigned int IDVBEventSource::Poll(CDVBCondition *Condition)
{
	return POLLERR;
}

void IDVBEventSource::PollWait(CDVBCondition *Condition)
{
	m_WaitQueue.Add(Condition);
}
