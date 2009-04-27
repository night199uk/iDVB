/*
 *  IThread.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 10/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IThread.h"
#include <pthread.h>
#include <Video4Mac/CDVBLog.h>

using namespace Video4Mac;

static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t  tlsLocalThread = 0;

//
// Called once and only once.
//
static void MakeTLSKeys()
{
	pthread_key_create(&tlsLocalThread, NULL);
}

IThread::IThread()
{
	// Initialize thread local storage and local thread pointer.
	pthread_once(&keyOnce, MakeTLSKeys);
	
	m_bStop = false;
	
	m_ThreadHandle = NULL;
}

IThread::~IThread()
{
	if (m_ThreadHandle != NULL)
	{
		pthread_cancel(m_ThreadHandle);
	}
	m_ThreadHandle = NULL;
	
//	if (m_StopEvent)
//		CloseHandle(m_StopEvent);
}

#define LOCAL_THREAD ((IThread* )pthread_getspecific(tlsLocalThread))
void IThread::TermHandler(int signum)
{
	fprintf(stderr, "thread 0x%x (%d) got signal %d. calling OnException and terminating thread abnormally.", pthread_self(), pthread_self(), signum);
	if (LOCAL_THREAD)
	{
		LOCAL_THREAD->m_bStop = TRUE;
//		if (LOCAL_THREAD->m_StopEvent)
//			SetEvent(LOCAL_THREAD->m_StopEvent);
		
		LOCAL_THREAD->OnException();
//		if( LOCAL_THREAD->IsAutoDelete() )
//			delete LOCAL_THREAD;
	}
	
	pthread_exit(NULL);
}

void *IThread::StaticThread(void* data)
{
//	IThread* pThread = dynamic_cast<IThread *>(data);
	IThread* pThread = (IThread*)(data);
	if (!pThread)
	{
		fprintf(stderr, "%s, sanity failed. thread is NULL.\n",__FUNCTION__);
		return NULL;
	}
	
	struct sigaction action;
	action.sa_handler = TermHandler;
	sigemptyset (&action.sa_mask);
	action.sa_flags = 0;
	
	pthread_setspecific(tlsLocalThread, (void*)pThread);
	
	try 
	{
		pThread->OnStartup();
	}
	catch(...)
	{
	}
	
	try
	{
		pThread->Process();
	}
	catch(...)
	{
		fprintf(stderr, "%s - Unhandled exception caught in thread process, attemping cleanup in OnExit", __FUNCTION__); 
	}
	try
	{
		pThread->OnExit();
	}
	catch(...)
	{
		fprintf(stderr, "%s - Unhandled exception caught in thread exit", __FUNCTION__); 
	}
	
	fprintf(stderr, "Thread %u terminating", pthread_self());	
	
	return NULL;
}

void IThread::Create(bool bAutoDelete, unsigned stacksize)
{
	int ret;
	
	if (m_ThreadHandle) {
		if (!m_bStop) {
			return;
		} else {
//			Stop();
		}
	}
	
	m_bStop = false;

	ret = pthread_create(&m_ThreadHandle, NULL, StaticThread, (IThread*)this);
	if (ret)
	{
		CDVBLog::Log(kDVBLogFrontend, "could not start thread.\n");
	}
}

void IThread::StopThread()
{
	m_bStop = true;
	if (m_ThreadHandle)
	{
//
		pthread_cancel(m_ThreadHandle);
	}
}
