/*
 *  IDVBDmxDevFilter.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 14/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBDmxDevFilter.h"
#include <IDVBAdapter.h>
#include <CDVBRingBuffer.h>
#include <CDVBLog.h>
#include <IDVBDemux.h>
#include <IDVBDemuxTSFeed.h>
#include <IDVBDemuxSectionFeed.h>
#include <pthread.h>


using namespace Video4Mac;

void IDVBDmxDevFilter::Initialize()
{
	pthread_mutex_init(&m_Mutex, 0);
	//	file->private_data = dmxdevfilter;
	
	m_Buffer = new CDVBRingBuffer();
	
	m_Buffer->Init(188000);
	m_Type = DMXDEV_TYPE_NONE;
	StateSet(DMXDEV_STATE_ALLOCATED);
	m_Feed = NULL;
}

IDVBDmxDevFilter::~IDVBDmxDevFilter()
{
	delete m_Buffer;
}

int IDVBDmxDevFilter::SetBufferSize(unsigned long size)
{
	if (m_State >= DMXDEV_STATE_GO)
		return -EBUSY;
	
	m_Buffer->SetBufferSize(size);
}

inline void IDVBDmxDevFilter::StateSet(int state)
{
	//	spin_lock_irq(&dmxdevfilter->dev->lock);
	m_State = (kDVBDmxDevState) state;
	//	spin_unlock_irq(&dmxdevfilter->dev->lock);
}


/*
void IDVBDmxDevFilter::Timeout(unsigned long data)
{
	struct dmxdev_filter *dmxdevfilter = (struct dmxdev_filter *)data;
	
	dmxdevfilter->buffer.error = -ETIMEDOUT;
	//	spin_lock_irq(&dmxdevfilter->dev->lock);
	dmxdevfilter->state = DMXDEV_STATE_TIMEDOUT;
	//	spin_unlock_irq(&dmxdevfilter->dev->lock);
	//	wake_up(&dmxdevfilter->buffer.queue);
}
*/

void IDVBDmxDevFilter::Timer()
{
	IDVBDmxSctFilterParams *para = &m_Params.Sec;
	
	//	del_timer(&dmxdevfilter->timer);
	//	if (para->timeout) {
	//		dmxdevfilter->timer.function = dvb_dmxdev_filter_timeout;
	//		dmxdevfilter->timer.data = (unsigned long)dmxdevfilter;
	//		dmxdevfilter->timer.expires =
	//		    1/* jiffies */ + 1 + (HZ / 2 + HZ * para->timeout) / 1000;
	//		add_timer(&dmxdevfilter->timer);
	//	}
}

ssize_t IDVBDmxDevFilter::Read(char /* __user */ *buf, size_t count, long long *ppos)
{
	int ret;
	
	if (pthread_mutex_lock(&m_Mutex))
		return -EINTR;
	
	/*	if (dmxdevfilter->type == DMXDEV_TYPE_SEC)
	 ret = dvb_dmxdev_read_sec(dmxdevfilter, file, buf, count, ppos);
	 else*/
	ret = m_Buffer->BufferRead(/*file->f_flags & O_NONBLOCK*/ 1,
					buf, count, ppos);
	
	pthread_mutex_unlock(&m_Mutex);
	return ret;
}

/* stop feed but only mark the specified filter as stopped (state set) */
int IDVBDmxDevFilter::FeedStop()
{
	StateSet(DMXDEV_STATE_SET);
	
	m_Feed->StopFiltering();

	return 0;
}

// start feed associated with the specified filter
int IDVBDmxDevFilter::FeedStart()
{
	StateSet(DMXDEV_STATE_GO);
	
	m_Feed->StartFiltering();

	return 0;
}

// restart section feed if it has filters left associated with it, otherwise release the feed
int IDVBDmxDevFilter::FeedRestart()
{
	int i;
		UInt16 pid = m_Params.Sec.PID;
/*
	for (i = 0; i < m_DmxDev->m_FilterNum; i++)
		if (m_DmxDev->m_Filter[i].m_State >= DMXDEV_STATE_GO &&
		    m_DmxDev->m_Filter[i].type == DMXDEV_TYPE_SEC &&
		    m_DmxDev->m_Filter[i].params.sec.pid == pid) {
			FeedStart(&dmxdev->filter[i]);
			return 0;
		}
	
	filter->dev->demux->release_section_feed(dmxdev->demux,
											 filter->feed.sec);
*/	
	return 0;
}

int IDVBDmxDevFilter::Stop()
{
	IDVBDemuxSectionFeed *Sec = (IDVBDemuxSectionFeed *) m_Feed;
	IDVBDemuxTSFeed *TS = (IDVBDemuxTSFeed *) m_Feed;

	if (m_State < DMXDEV_STATE_GO)
		return 0;
	
	switch (m_Type) {
		case DMXDEV_TYPE_SEC:

			if (!Sec)
				break;
			FeedStop();
			if (m_Filter.Sec)
				Sec->ReleaseFilter(m_Filter.Sec);
			FeedRestart();
			m_Feed = NULL;
			break;
		case DMXDEV_TYPE_PES:
			if (!TS)
				break;
			FeedStop();
//			m_DmxDev->ReleaseTSFeed(dmxdevfilter->dev->demux,
//							dmxdevfilter->feed.ts);
			m_Feed = NULL;
			break;
		default:
			if (m_State == DMXDEV_STATE_ALLOCATED)
				return 0;
			return -EINVAL;
	}
	
	m_Buffer->Flush();
	return 0;
}

int IDVBDmxDevFilter::Reset()
{
	if (m_State < DMXDEV_STATE_SET)
		return 0;
	
	m_Type = DMXDEV_TYPE_NONE;
	StateSet(DMXDEV_STATE_ALLOCATED);
	return 0;
}


static int dvb_dmxdev_section_callback(const UInt8 *buffer1, size_t buffer1_len,
									   const UInt8 *buffer2, size_t buffer2_len,
									   IDVBDmxSectionFilter *filter,
									   kDVBDemuxSuccess success)
{
	
	fprintf(stderr, "section stream callback called.\n");
/*
	IDVBDmxDevFilter *dmxdevfilter = (IDVBDmxDevFilter *) filter->priv;
	int ret;
	
	if (dmxdevfilter->m_Buffer.error) {
		//		wake_up(&dmxdevfilter->buffer.queue);
		return 0;
	}
	//	spin_lock(&dmxdevfilter->dev->lock);
	if (dmxdevfilter->GetState() != DMXDEV_STATE_GO) {
		//		spin_unlock(&dmxdevfilter->dev->lock);
		return 0;
	}
	//	del_timer(&dmxdevfilter->timer);
	CDVBLog::Log(kDVBLogDemux, "dmxdev: section callback %02x %02x %02x %02x %02x %02x\n",
			buffer1[0], buffer1[1],
			buffer1[2], buffer1[3], buffer1[4], buffer1[5]);
	ret = dmxdevfilter->m_Buffer->BufferWrite(buffer1, buffer1_len);
	if (ret == buffer1_len) {
		ret = dmxdevfilter->m_Buffer->BufferWrite(buffer2, buffer2_len);
	}
	if (ret < 0) {
		dmxdevfilter->m_Buffer->Flush();
		dmxdevfilter->buffer.error = ret;
	}
	if (dmxdevfilter->params.sec.flags & DMX_ONESHOT)
		dmxdevfilter->state = DMXDEV_STATE_DONE;
	//	spin_unlock(&dmxdevfilter->dev->lock);
	//	wake_up(&dmxdevfilter->buffer.queue);
	return 0;
*/
}

int	IDVBDmxDevFilter::TSCallback(const UInt8 *buffer1, size_t buffer1_len,
						   const UInt8 *buffer2, size_t buffer2_len,
						   IDVBDemuxTSFeed *feed,
						   kDVBDemuxSuccess success)
{
	int ret;
	CDVBRingBuffer *buffer;
	
	//	spin_lock(&dmxdevfilter->dev->lock);
	if (m_Params.PES.Output == kDVBDmxOutDecoder) {
		//		spin_unlock(&dmxdevfilter->dev->lock);
		return 0;
	}
	
	if (m_Params.PES.Output == kDVBDmxOutTap
	    || m_Params.PES.Output == kDVBDmxOutTSDemuxTap)
	{
		buffer = m_Buffer;
	}
	else
	{
		buffer = m_Adapter->GetBuffer();
	}

	if (buffer->Error()) {
		//		spin_unlock(&dmxdevfilter->dev->lock);
		//		wake_up(&buffer->queue);
		return 0;
	}
	
	ret = buffer->BufferWrite(buffer1, buffer1_len);
	if (ret == buffer1_len)
		ret = buffer->BufferWrite(buffer2, buffer2_len);
	if (ret < 0) {
		buffer->Flush();
//		buffer->error = ret;
	}
	//	spin_unlock(&dmxdevfilter->dev->lock);
	//	wake_up(&buffer->queue);
	return 0;
}
int	IDVBDmxDevFilter::SectionCallback(const UInt8 *buffer1, size_t buffer1_len,
								const UInt8 *buffer2, size_t buffer2_len,
								IDVBDmxSectionFilter *filter,
								kDVBDemuxSuccess success)
{
	
}
int IDVBDmxDevFilter::Start()
{
	void *mem;
	int ret, i;
	
	if (m_State < DMXDEV_STATE_SET)
		return -EINVAL;
	
	if (m_State >= DMXDEV_STATE_GO)
		Stop();
	
	/*
	if (!m_Buffer->m_Data) {
		mem = malloc(filter->buffer.size);
		if (!mem)
			return -ENOMEM;
		//		spin_lock_irq(&filter->dev->lock);
		filter->buffer.data = (UInt8 *) mem;
		//		spin_unlock_irq(&filter->dev->lock);
	}
	*/
	
	m_Buffer->Flush();

	switch (m_Type) {
		case DMXDEV_TYPE_SEC:
		{
			IDVBDmxSctFilterParams *para = &m_Params.Sec;
			IDVBDmxSectionFilter **secfilter = &m_Filter.Sec;
			IDVBDemuxSectionFeed **secfeed = (IDVBDemuxSectionFeed **) &m_Feed;
			
			*secfilter = NULL;
			*secfeed = NULL;
			
			/* find active filter/feed with same PID */
			/*
			for (i = 0; i < dmxdev->filternum; i++) {
				if (dmxdev->filter[i].state >= DMXDEV_STATE_GO &&
					dmxdev->filter[i].type == DMXDEV_TYPE_SEC &&
					dmxdev->filter[i].params.sec.pid == para->pid) {
					*secfeed = dmxdev->filter[i].feed.sec;
					break;
				}
			}
			*/
			
			/* if no feed found, try to allocate new one */
			if (!*secfeed) {
				ret = m_Demux->AllocateSectionFeed(secfeed, dvb_dmxdev_section_callback);
				if (ret < 0) {
					CDVBLog::Log(kDVBLogDemux, "DVB (%s): could not alloc feed\n", __func__);
					return ret;
				}
				
				ret = (*secfeed)->Set(para->PID, 32768, (para->Flags & DMX_CHECK_CRC) ? 1 : 0);
				if (ret < 0) {
					CDVBLog::Log(kDVBLogDemux, "DVB (%s): could not set feed\n", __func__);
					FeedRestart();
					return ret;
				}
			} else {
				FeedStop();
			}
			
			ret = (*secfeed)->AllocateFilter(secfilter);
			if (ret < 0) {
				FeedRestart();
				m_Feed->StartFiltering();
				CDVBLog::Log(kDVBLogDemux, "could not get filter\n");
				return ret;
			}
			
			(*secfilter)->priv = this;
			
			memcpy(&((*secfilter)->FilterValue[3]),
				   &(para->Filter.Filter[1]), DMX_FILTER_SIZE - 1);
			memcpy(&(*secfilter)->FilterMask[3],
				   &para->Filter.Mask[1], DMX_FILTER_SIZE - 1);
			memcpy(&(*secfilter)->FilterMode[3],
				   &para->Filter.Mode[1], DMX_FILTER_SIZE - 1);
			
			(*secfilter)->FilterValue[0] = para->Filter.Filter[0];
			(*secfilter)->FilterMask[0] = para->Filter.Mask[0];
			(*secfilter)->FilterMode[0] = para->Filter.Mode[0];
			(*secfilter)->FilterMask[1] = 0;
			(*secfilter)->FilterMask[2] = 0;
			
			m_Todo = 0;
			
			ret = m_Feed->StartFiltering();
			if (ret < 0)
				return ret;
			
//			FilterTimer();
			break;
		}
		case DMXDEV_TYPE_PES:
		{
			struct timespec timeout = { 0 };
			IDVBDmxPESFilterParams *para = &m_Params.PES;
			kDVBDmxOutput otype;
			int ts_type;
			kDVBDmxTSPES ts_pes;
			struct IDVBDemuxTSFeed **tsfeed = (IDVBDemuxTSFeed **) &m_Feed;
			
			otype = para->Output;
			
			ts_pes = (kDVBDmxTSPES) para->PESType;
			
			if (ts_pes < kDVBDmxPESOther)
				ts_type = TS_DECODER;
			else
				ts_type = 0;
			
			if (otype == kDVBDmxOutTSTap)
				ts_type |= TS_PACKET;
			else if (otype == kDVBDmxOutTSDemuxTap)
				ts_type |= TS_PACKET | TS_DEMUX;
			else if (otype == kDVBDmxOutTap)
				ts_type |= TS_PACKET | TS_DEMUX | TS_PAYLOAD_ONLY;
			
			ret = m_Demux->AllocateTSFeed(tsfeed, NULL);
			if (ret < 0)
				return ret;
			
			(*tsfeed)->m_DmxFilter = this;
			
			ret = (*tsfeed)->Set(para->PID, ts_type, ts_pes, 32768, timeout);
			if (ret < 0) {
				m_Demux->ReleaseTSFeed(*tsfeed);
				return ret;
			}
			
			ret = m_Feed->StartFiltering();
			if (ret < 0) {
				m_Demux->ReleaseTSFeed(*tsfeed);
				return ret;
			}
			
			break;
		}
		default:
			return -EINVAL;
	}
	
	StateSet(DMXDEV_STATE_GO);
	return 0;
}


static inline void invert_mode(IDVBDmxFilter *filter)
{
	int i;
	
	for (i = 0; i < DMX_FILTER_SIZE; i++)
		filter->Mode[i] ^= 0xff;
}



int IDVBDmxDevFilter::Set(IDVBDmxSctFilterParams *params)
{
	CDVBLog::Log(kDVBLogDemux, "function : %s\n", __func__);
	
	Stop();
	
	m_Type = DMXDEV_TYPE_SEC;
	memcpy(&m_Params.Sec,
	       params, sizeof(IDVBDmxSctFilterParams));
	invert_mode(&m_Params.Sec.Filter);
	StateSet(DMXDEV_STATE_SET);
	
	if (params->Flags & DMX_IMMEDIATE_START)
		return Start();
	
	return 0;
}

int IDVBDmxDevFilter::PESFilterSet(IDVBDmxPESFilterParams *params)
{
	Stop();
	
	
	if (params->PESType > kDVBDmxPESOther || params->PESType < 0)
	{
		return -EINVAL;
	}
	
	m_Type = DMXDEV_TYPE_PES;
	memcpy(&m_Params, params, sizeof(IDVBDmxPESFilterParams));
	
	StateSet(DMXDEV_STATE_SET);
	
	if (params->Flags & DMX_IMMEDIATE_START)
	{
		return Start();
	}

	return 0;
}


int IDVBDmxDevFilter::IOCtl(unsigned int cmd, void *parg)
{
	//	struct dmxdev_filter *dmxdevfilter = file->private_data;
	// struct dmxdev *dmxdev = dmxdevfilter->dev;
	unsigned long arg = (unsigned long)parg;
	int ret = 0;
	
/*	if (pthread_mutex_lock(&dmxdev->mutex))
	{
		printf("mutex error.\n");
		return -EINTR;
	}*/
	
	switch (cmd) {
		case DMX_START:
			if (pthread_mutex_lock(&m_Mutex)) {
//				pthread_mutex_unlock(&dmxdev->mutex);
				return -EINTR; 
			}
			if (m_State < DMXDEV_STATE_SET)
				ret = -EINVAL;
			else
				ret = Start();
			pthread_mutex_unlock(&m_Mutex);
			break;
			
		case DMX_STOP:
			if (pthread_mutex_lock(&m_Mutex)) {
//				pthread_mutex_unlock(&dmxdev->mutex);
				return -EINTR;
			}
			ret = Stop();
			pthread_mutex_unlock(&m_Mutex);
			break;
			
		case DMX_SET_FILTER:
			if (pthread_mutex_lock(&m_Mutex)) {
//				pthread_mutex_unlock(&dmxdev->mutex);
				return -EINTR;
			}
			ret = Set((IDVBDmxSctFilterParams *) parg);
			pthread_mutex_unlock(&m_Mutex);
			break;
			
		case DMX_SET_PES_FILTER:
			if (pthread_mutex_lock(&m_Mutex)) {
//				pthread_mutex_unlock(&dmxdev->mutex);
				return -EINTR;
			}
			ret = PESFilterSet((IDVBDmxPESFilterParams *)parg);
			pthread_mutex_unlock(&m_Mutex);
			break;
			
		case DMX_SET_BUFFER_SIZE:
			if (pthread_mutex_lock(&m_Mutex)) {
//				pthread_mutex_unlock(&dmxdev->mutex);
				return -EINTR;
			}
			ret = SetBufferSize(arg);
			pthread_mutex_unlock(&m_Mutex);
			break;
			
/*		case DMX_GET_PES_PIDS:
			if (!dmxdev->demux->get_pes_pids) {
				ret = kIOReturnBadArgument;
				break;
			}
			dmxdev->demux->get_pes_pids(dmxdev->demux, (UInt16 *)parg);
			break;
			
		case DMX_GET_CAPS:
			if (!dmxdev->demux->get_caps) {
				ret = kIOReturnBadArgument;
				break;
			}
			ret = dmxdev->demux->get_caps(dmxdev->demux, (dmx_caps *)parg);
			break;
			
		case DMX_SET_SOURCE:
			if (!dmxdev->demux->set_source) {
				ret = kIOReturnBadArgument;
				break;
			}
			ret = dmxdev->demux->set_source(dmxdev->demux, (dmx_source_t *)parg);
			break;
			
		case DMX_GET_STC:
			if (!dmxdev->demux->get_stc) {
				ret = kIOReturnBadArgument;
				break;
			}
			ret = dmxdev->demux->get_stc(dmxdev->demux,
										 ((struct dmx_stc *)parg)->num,
										 &((struct dmx_stc *)parg)->stc,
										 &((struct dmx_stc *)parg)->base);
			break;
*/			
		default:
			ret = -EINVAL;
			break;
	}
//	pthread_mutex_unlock(&dmxdev->mutex);
	return ret;
}

unsigned int IDVBDmxDevFilter::Poll(IDVBCondition *Condition)
{
	//	struct dmxdev_filter *dmxdevfilter = file->private_data;
	unsigned int mask = 0;
	
	PollWait(Condition);
	//	poll_wait(file, &dmxdevfilter->buffer.queue, wait);
	
	if (m_State != DMXDEV_STATE_GO &&
	    m_State != DMXDEV_STATE_DONE &&
	    m_State != DMXDEV_STATE_TIMEDOUT)
		return 0;
	
	//	if (dmxdevfilter->buffer.error)
	//		mask |= (POLLIN | POLLRDNORM | POLLPRI | POLLERR);
	
	//	if (!dvb_ringbuffer_empty(&dmxdevfilter->buffer))
	//		mask |= (POLLIN | POLLRDNORM | POLLPRI);
	
	return mask;
}


int IDVBDmxDevFilter::Free()
{
	pthread_mutex_lock(&m_Mutex);
	//	pthread_mutex_lock(&dmxdevfilter->m_Mutex);
	
	Stop();
	Reset();
	
	//	if (dmxdevfilter->buffer.data) {
	//		void *mem = dmxdevfilter->buffer.data;
	
	//		spin_lock_irq(&dmxdev->lock);
	//		dmxdevfilter->buffer.data = NULL;
	//		spin_unlock_irq(&dmxdev->lock);
	//		free(mem);
	//	}
	
	SetState(DMXDEV_STATE_FREE);
	
	//	dvb_dmxdev_filter_state_set(dmxdevfilter, DMXDEV_STATE_FREE);
	//	wake_up(&dmxdevfilter->buffer.queue);
	//	pthread_mutex_unlock(&dmxdevfilter->mutex);
	pthread_mutex_unlock(&m_Mutex);
	return 0;
}

