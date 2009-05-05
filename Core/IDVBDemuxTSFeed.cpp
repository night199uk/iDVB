/*
 *  IDVBDmxTSFeed.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBDemuxTSFeed.h"
#include "IDVBDmxDevFilter.h"

using namespace Video4Mac;

static inline UInt16 section_length(const UInt8 *buf)
{
	return 3 + ((buf[1] & 0x0f) << 8) + buf[2];
}

static inline UInt16 ts_pid(const UInt8 *buf)
{
	return ((buf[1] & 0x1f) << 8) + buf[2];
}

static inline UInt8 payload(const UInt8 *tsp)
{
	if (!(tsp[3] & 0x10))	// no payload?
		return 0;
	
	if (tsp[3] & 0x20) {	// adaptation field?
		if (tsp[4] > 183)	// corrupted data?
			return 0;
		else
			return 184 - 1 - tsp[4];
	}
	
	return 184;
}

int IDVBDemuxTSFeed::SWFilterPayload(const UInt8 *buf)
{
	int count = payload(buf);
	int p;
	//int ccok;
	//UInt8 cc;
	
	if (count == 0)
		return -1;
	
	p = 188 - count;
	
	/*
	 cc = buf[3] & 0x0f;
	 ccok = ((feed->cc + 1) & 0x0f) == cc;
	 feed->cc = cc;
	 if (!ccok)
	 printk("missed packet!\n");
	 */
	
	if (buf[1] & 0x40)	// PUSI ?
		m_PESLen = 0xfffa;
	
	m_PESLen += count;

	return m_DmxFilter->TSCallback(&buf[p], count, NULL, 0, this, DMX_OK);
//	return m_Callback(&buf[p], count, NULL, 0, this, DMX_OK);
}



int IDVBDemuxTSFeed::Set(UInt16 pid, int ts_type,  kDVBDmxTSPES pes_type,
						   size_t circular_buffer_size, struct timespec timeout)
{
	if (pid > DMX_MAX_PID)
		return -EINVAL; /* -EINVAL; */
	
	if (pthread_mutex_lock(&m_Demux->m_Mutex))
		return -EINVAL; /*-ERESTARTSYS; */ 
	
	if (ts_type & TS_DECODER) {
		if (pes_type >= DMX_TS_PES_OTHER) {
			pthread_mutex_unlock(&m_Demux->m_Mutex);
			return -EINVAL; /* -EINVAL; */
		}
		
		if (m_Demux->m_PESFilter[pes_type] &&
		    m_Demux->m_PESFilter[pes_type] != (IDVBDemuxFeed *)this) {
			pthread_mutex_unlock(&m_Demux->m_Mutex);
			return -EINVAL; /* -EINVAL; */
		}
		
		m_Demux->m_PESFilter[pes_type] = (IDVBDemuxFeed *)this;
		m_Demux->m_PIDs[pes_type] = pid;
	}

	m_Demux->FeedAdd(this);
	
	m_PID = pid;
	m_BufferSize = circular_buffer_size;
	m_Timeout = timeout;
	m_TSType = ts_type;
	m_PESType = pes_type;
	
	if (m_BufferSize) {
#ifdef NOBUFS
		m_Buffer = NULL;
#else
		m_Buffer = (UInt8 *)malloc(m_BufferSize);
		if (!m_Buffer) {
			pthread_mutex_unlock(&m_Demux->m_Mutex);
			return -ENOMEM;
		}
#endif
	}
	
	m_State = DMX_STATE_READY;
	pthread_mutex_unlock(&m_Demux->m_Mutex);
	
	return 0;
}

int IDVBDemuxTSFeed::StartFiltering()
{
	int ret;

	if (pthread_mutex_lock(&m_Demux->m_Mutex))
		return -EINVAL; /*-ERESTARTSYS; */ 
	
	if (m_State != DMX_STATE_READY || m_Type != DMX_TYPE_TS) {
		pthread_mutex_unlock(&m_Demux->m_Mutex);
		return -EINVAL;
	}

	if ((ret = m_Demux->StartFeed(this)) < 0) {
		pthread_mutex_unlock(&m_Demux->m_Mutex);
		return ret;
	}
	
	//	spin_lock_irq(&demux->lock);
	m_IsFiltering = 1;
	m_State = DMX_STATE_GO;
	//	spin_unlock_irq(&demux->lock);
	
	pthread_mutex_unlock(&m_Demux->m_Mutex);
	
	return 0;
}

int IDVBDemuxTSFeed::StopFiltering()
{
	int ret;
	
	pthread_mutex_lock(&m_Demux->m_Mutex);
	
	if (m_State < DMX_STATE_GO) {
		pthread_mutex_unlock(&m_Demux->m_Mutex);
		return -EINVAL;
	}
	
	/*
	if (!demux->stop_feed) {
		pthread_mutex_unlock(&demux->mutex);
		return -ENODEV;
	}
	*/
	
	ret = m_Demux->StopFeed(this);
	
	//	spin_lock_irq(&demux->lock);
	m_IsFiltering = 0;
	m_State = DMX_STATE_ALLOCATED;
	//	spin_unlock_irq(&demux->lock);
	pthread_mutex_unlock(&m_Demux->m_Mutex);
	
	return ret;
}
