/*
 *  IDVBAdapter.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBAdapter.h"
#include <IDVBDmxDevFilter.h>
#include <IDVBFrontend.h>
#include <CDVBRingBuffer.h>
#include <CDVBLog.h>

using namespace Video4Mac;

IDVBAdapter::IDVBAdapter()
{
	m_Frontend = NULL;
	m_Demux = NULL;

	m_Filter = NULL;
	m_FilterNum = 0;

	m_Buffer = new CDVBRingBuffer;
	m_Buffer->Init(188000);
	
	for (int i = 0; i < m_FilterNum; i++) {
		//		m_Filter[i].dev = this;
		//		m_Filter[i].Buffer.Data = NULL;
		m_Filter[i].SetState(DMXDEV_STATE_FREE);
	}

	
	pthread_mutex_init(&m_Mutex, 0);
	
	//	spin_lock_init(&dmxdev->lock);
	
	

}

IDVBAdapter::~IDVBAdapter()
{
}

IDVBDmxDevFilter *IDVBAdapter::GetFilter()
{
	int i;
	IDVBDmxDevFilter *dmxdevfilter;
	
	if (!m_Filter)
	{
		CDVBLog::Log(kDVBLogDemux, "no dmxdev filter!\n");
		return NULL;
	}
	
	if (pthread_mutex_lock(&m_Mutex))
	{
		CDVBLog::Log(kDVBLogDemux, "couldn't lock mutex!\n");
		return	NULL;
	}
	
	for (i = 0; i < m_FilterNum; i++)
		if (m_Filter[i].GetState() == DMXDEV_STATE_FREE)
			break;
	
	if (i == m_FilterNum) {
		CDVBLog::Log(kDVBLogDemux, "too many filters!\n");
		pthread_mutex_unlock(&m_Mutex);
		return NULL;
	}
	
	m_Filter[i].SetDemux(m_Demux);
	m_Filter[i].SetAdapter(this);
	
	dmxdevfilter = &m_Filter[i];
	
	dmxdevfilter->Initialize();
	//	init_timer(&dmxdevfilter->timer);
	
	//	dvbdev->users++;
	
	pthread_mutex_unlock(&m_Mutex);
	return dmxdevfilter;
}

int IDVBAdapter::Release(IDVBDmxDevFilter *dmxdevfilter)
{
	int ret;

	pthread_mutex_lock(&m_Mutex);
	ret = dmxdevfilter->Free();
	
	pthread_mutex_unlock(&m_Mutex);
	//		wake_up(&dmxdev->dvbdev->wait_queue);
	
	return ret;
}


ssize_t IDVBAdapter::Read(char /* __user */ *buf, size_t count, long long *ppos)
{
	int ret;
	
	if (pthread_mutex_lock(&m_Mutex))
		return -EINTR;
	
	ret = m_Buffer->BufferRead(/*file->f_flags & O_NONBLOCK*/ 0,
							   buf, count, ppos);
	
	pthread_mutex_unlock(&m_Mutex);
	return ret;
}
