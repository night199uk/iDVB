/*
 *  IThread.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 10/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#pragma once

namespace Video4Mac
{
	class IThread
	{
	public:
		IThread();
		virtual ~IThread();
		
		void			Create(bool bAutoDelete = false, unsigned stacksize = 0);
		void			Sleep(unsigned long dwMilliseconds);
		
		virtual void	StopThread();

	protected:
		virtual void	OnStartup(){};
		virtual void	OnExit(){};
		virtual void	OnException(){} // signal termination handler
		virtual void	Process() {}; 
		
		static void		TermHandler(int signum);
		
		volatile bool	m_bStop;
		pthread_t		m_ThreadHandle;		

	private:
		static void*	StaticThread(void *data);
	};
}