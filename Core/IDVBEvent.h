/*
 *  IDVBEventSource.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 19/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <Video4Mac/IDVBPrimitives.h>

/* File include_type.h */ 
#define AND 1 
#define OR 0 
#define      NULLP ((void *)0) 

using namespace std;

namespace Video4Mac
{
	class IDVBEventSource;

	typedef struct 
	{
		IDVBEventSource *Source;
		unsigned int Events;
		unsigned int REvents;
	} DVBPollDescriptor;



	class IDVBEventListener
	{
	public:
		IDVBEventListener();
		unsigned int			Poll(DVBPollDescriptor* poll_list, int num_pds, unsigned int timeout);
	private:
		inline unsigned int		PollPD(DVBPollDescriptor* pollpd, CDVBCondition *cond);
		
		CDVBCondition m_Cond;
	};
	
	class IDVBEventSource
	{
	public:
//		virtual int				WakeUp();
//		virtual int				Wait();
//		virtual int				WaitTimeout();
//		virtual int				Signal();
		
		virtual unsigned int	Poll(CDVBCondition *Condition);

	protected:
		void					PollWait(CDVBCondition *Condition);
//		virtual int				Register();
//		virtual int				Unregister();
		CDVBWaitQueue			m_WaitQueue;
		
	private:
	};
};

