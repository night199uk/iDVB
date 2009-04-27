/*
 *  IEventQueue.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 11/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <Video4Mac/CDVBLog.h>


namespace Video4Mac
{
	template <typename T>
	class IEventQueue
	{
	public:
		IEventQueue(unsigned int size=8)	: m_Size(size), m_Events(new T[size]), m_EventR(0), m_EventW(0)
		{ 
			pthread_cond_init(&m_Condition, NULL);
			pthread_mutex_init(&m_Mutex, NULL);
		}
		
		~IEventQueue()						{	delete[] m_Events;		};
		void AddEvent(T Event);
		void Wait();
		void WaitTimeout(double timeout);
		bool HasEvents();
		bool GetEvent(T *event, int flags);

	private:
		//    wait_queue_head_t     wait_queue;
		//    struct mutex          mtx;
		pthread_cond_t	m_Condition;
		pthread_mutex_t	m_Mutex;
	};
};

using namespace Video4Mac;


template <typename T>
void IEventQueue<T>::AddEvent(T Event)
{
	int wp;
	if (pthread_mutex_lock(&m_Mutex))
		return;
	wp = (m_EventW + 1) % m_Size;
	if (wp == m_EventR) {
		m_Overflow = 1;
		m_EventR = (m_EventR + 1) % m_Size;
	}
	m_Events[m_EventW] = Event;
	m_EventW = wp;
	pthread_mutex_unlock(&m_Mutex);
	CDVBLog::Log(kDVBLogFrontend, "Event added to the EventQueue\n");
	pthread_cond_broadcast(&m_Condition);
}

template <typename T>
void IEventQueue<T>::Wait()
{
	if (pthread_mutex_lock(&m_Mutex))
		return;
	
	pthread_cond_wait(&m_Condition, &m_Mutex);
	pthread_mutex_unlock(&m_Mutex);
};

template <typename T>
void IEventQueue<T>::WaitTimeout(double timeout)
{
	int rc;
	struct timespec ts;
	struct timeval tp;
	
	if (pthread_mutex_lock(&m_Mutex))
		return;
	
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
	
	pthread_cond_timedwait(&m_Condition, &m_Mutex, &ts);
	CDVBLog::Log(kDVBLogFrontend, "EventQueue got a condition on timedwait.\n");
	pthread_mutex_unlock(&m_Mutex);
}

template <typename T>
bool IEventQueue<T>::HasEvents()
{
	return m_EventW != m_EventR;
}

template <typename T>
bool IEventQueue<T>::GetEvent(T *event, int flags)
{
    fprintf(stderr, "%s\n", __func__);
	
    if (m_Overflow) {
        m_Overflow = 0;
        return -EOVERFLOW;
    }

	pthread_mutex_lock(&m_Mutex);	
    if (m_EventW == m_EventR) {
        int ret;
		
        if (flags & O_NONBLOCK)
		{
			pthread_mutex_unlock(&m_Mutex);
            return -EWOULDBLOCK;
		}
		
		ret = pthread_cond_wait(&m_Condition, &m_Mutex);
		
        if (ret < 0)
            return ret;
    }
	
    memcpy (event, &m_Events[m_EventR],
			sizeof(T));
	
    m_EventR = (m_EventR + 1) % m_Size;
	
	fprintf(stderr, "EventR: %d, Size: %d\n", m_EventR, m_Size);
	
    pthread_mutex_unlock(&m_Mutex);
	
    return 0;
}
