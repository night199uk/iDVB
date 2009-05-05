/*
 *  IDVBPrimitives.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 05/05/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "CDVBPrimitives.h"

using namespace Video4Mac;

int CDVBCondition::TimedWait(unsigned int timeout)
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
