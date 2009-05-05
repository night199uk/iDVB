/*
 *  IDVBPrimitives.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 05/05/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <list>
#include <deque>
#include <pthread.h>
#include <mach/semaphore.h>
#include <mach/mach.h>
#include <sys/time.h>

namespace Video4Mac
{
	
	class IDVBMutex
		{
		public:
			IDVBMutex()									{ pthread_mutex_init(&m_Mutex, NULL); }
			void	Lock()								{ pthread_mutex_lock(&m_Mutex); };
			void	Unlock()							{ pthread_mutex_unlock(&m_Mutex); }; 
			void	TryLock()							{ pthread_mutex_trylock(&m_Mutex); };
		private:
			pthread_mutex_t			m_Mutex;
		};
	
	class IDVBCondition
		{
		public:
			IDVBCondition()								{ pthread_cond_init(&m_Cond, NULL); pthread_mutex_init(&m_Mutex, NULL); }
			void	Lock()								{ pthread_mutex_lock(&m_Mutex); };
			void	Unlock()							{ pthread_mutex_unlock(&m_Mutex); }; 
			int		Wait()								{ return pthread_cond_wait(&m_Cond, &m_Mutex); };
			int		TimedWait(unsigned int timeout);
			void	Signal()							{ pthread_cond_signal(&m_Cond); };
		private:
			pthread_mutex_t			m_Mutex;
			pthread_cond_t			m_Cond;
		};
	
	class IDVBSemaphore
		{
		public:
			IDVBSemaphore()								{ 	semaphore_create(mach_task_self(), &m_Sem, SYNC_POLICY_FIFO, 1); count = 1; };
			void	Up()								{	semaphore_signal(m_Sem); };
			void	Down()								{	semaphore_wait(m_Sem); };
		private:
			semaphore_t		m_Sem;
			int count;
		};
	
	class IDVBWaitQueue
		{
		public:
			IDVBWaitQueue()								{	m_Queue.clear(); };
			void	Add(IDVBCondition* cond)			{	m_Queue.push_back(cond); };
			void	SignalOne()							{	m_Queue.front()->Signal(); m_Queue.pop_front(); };
			void	SignalAll()							{	IDVBCondition *elem; while (!m_Queue.empty())	{	elem = m_Queue.front(); elem->Signal(); m_Queue.pop_front();	};	};
		private:
			std::deque<IDVBCondition *>	m_Queue;
		};
};
