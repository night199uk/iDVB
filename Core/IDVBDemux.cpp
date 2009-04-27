/*
 *  IDVBDemux.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBDemux.h"
#include <IDVBDemuxFeed.h>
#include <IDVBDemuxSectionFeed.h>
#include <IDVBDmxDevFilter.h>
#include <IDVBDemuxTSFeed.h>
#include <CDVBLog.h>

using namespace Video4Mac;

IDVBDemux::IDVBDemux(IDVBAdapter* Adapter, int FilterNum = 255, int FeedNum = 255) : m_Adapter(Adapter), m_FilterNum(FilterNum), m_FeedNum(FeedNum)
{
	m_FeedList.clear();
}

IDVBDemux::~IDVBDemux()
{
}

int IDVBDemux::Initialize()
{
    int i;
//    struct dmx_demux *dmx = &dvbdemux->dmx;
	
    m_Users = 0;
    m_Filter = (IDVBDemuxFilter *)malloc(m_FilterNum * sizeof(IDVBDemuxFilter));
	
    if (!m_Filter)
        return -ENOMEM;
	
//    m_Feed = (struct dvb_demux_feed *)malloc(dvbdemux->feednum * sizeof(struct dvb_demux_feed));
//    if (!m_Feed) {
//        free(dvbdemux->filter);
//        return -ENOMEM;
//    }
	
    for (i = 0; i < m_FilterNum; i++) {
        m_Filter[i].State = DMX_STATE_FREE;
        m_Filter[i].Index = i;
    }
    for (i = 0; i < m_FeedNum; i++) {
//        m_Feed[i].state = DMX_STATE_FREE;
//        m_Feed[i].index = i;
    }

	// Uses std::vector
//    INIT_LIST_HEAD(&dvbdemux->frontend_list);
	
	/*
    for (i = 0; i < DMX_TS_PES_OTHER; i++) {
        dvbdemux->pesfilter[i] = NULL;
        dvbdemux->pids[i] = 0xffff;
    }
	*/
	
//    INIT_LIST_HEAD(&dvbdemux->feed_list);
	
    m_Playing = 0;
    m_Recording = 0;
    m_TSBufP = 0;
	
	/*
    if (!dvbdemux->check_crc32)
        dvbdemux->check_crc32 = dvb_dmx_crc32;
	
    if (!dvbdemux->memcopy)
        dvbdemux->memcopy = dvb_dmx_memcopy;
	*/
/*    dmx->frontend = NULL;
    dmx->priv = dvbdemux;
    dmx->open = dvbdmx_open;
    dmx->close = dvbdmx_close;
    dmx->write = dvbdmx_write;
    dmx->allocate_ts_feed = dvbdmx_allocate_ts_feed;
    dmx->release_ts_feed = dvbdmx_release_ts_feed;
    dmx->allocate_section_feed = dvbdmx_allocate_section_feed;
    dmx->release_section_feed = dvbdmx_release_section_feed;
	
    dmx->add_frontend = dvbdmx_add_frontend;
    dmx->remove_frontend = dvbdmx_remove_frontend;
    dmx->get_frontends = dvbdmx_get_frontends;
    dmx->connect_frontend = dvbdmx_connect_frontend;
    dmx->disconnect_frontend = dvbdmx_disconnect_frontend;
	dmx->get_pes_pids = dvbdmx_get_pes_pids;
*/	
    pthread_mutex_init(&m_Mutex, 0);
	//    spin_lock_init(&dvbdemux->lock);
	
    return 0;
}

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

#define DVR_FEED(f)							\
(((f)->GetType() == DMX_TYPE_TS) &&					\
((f)->GetIsFiltering()) &&					\
(((f)->GetTSType() & (TS_PACKET | TS_DEMUX)) == TS_PACKET))

void IDVBDemux::SWFilterPacket(const UInt8 *buf)
{
	UInt16 pid = ts_pid(buf);
	int dvr_done = 0;
	IDVBDemuxFeed *Feed;
	//	list_for_each_entry(feed, &demux->feed_list, list_head) {
	//		if ((feed->pid != pid) && (feed->pid != 0x2000))
	//			continue;
	if (pid == 0)
		fprintf(stderr, "going to search for feeds, pid == %d, feedlist contains %d entries.\n", pid, m_FeedList.size());
	
	for (int i = 0; i < m_FeedList.size(); i++)
	{
		Feed = m_FeedList[i];
		
		if (Feed->GetPID() != pid && Feed->GetPID() != 0x2000)
			continue;
		
		/* copy each packet only once to the dvr device, even
		 * if a PID is in multiple filters (e.g. video + PCR) */
		if ((DVR_FEED(Feed)) && (dvr_done++))
			continue;
		
		if (Feed->GetPID() == pid) {
			Feed->SWFilterPacketType(buf);
			if (DVR_FEED(Feed))
				continue;
		}
		
		if (Feed->GetPID() == 0x2000)
		{
			Feed->m_DmxFilter->TSCallback(buf, 188, NULL, 0, (IDVBDemuxTSFeed *) Feed, DMX_OK);
//			TSFeed->m_Callback(buf, 188, NULL, 0, TSFeed, DMX_OK);
		}
			
	}
}

void IDVBDemux::SWFilterPackets(const UInt8 *buf, size_t count)
{
	//	spin_lock(&demux->lock);
	
	while (count--) {
		if (buf[0] == 0x47)
			SWFilterPacket(buf);
		buf += 188;
	}
	
	//	spin_unlock(&demux->lock);
}

void IDVBDemux::SWFilter(const UInt8 *buf, size_t count)
{
	int p = 0, i, j;
	
	//	spin_lock(&demux->lock);
	
	if (m_TSBufP) {
		i = m_TSBufP;
		j = 188 - i;
		if (count < j) {
			memcpy(&m_TSBuf[i], buf, count);
			m_TSBufP += count;
			goto bailout;
		}
		memcpy(&m_TSBuf[i], buf, j);
		if (m_TSBuf[0] == 0x47)
			SWFilterPacket(m_TSBuf);
		m_TSBufP = 0;
		p += j;
	}
	
	while (p < count) {
		if (buf[p] == 0x47) {
			if (count - p >= 188) {
				SWFilterPacket(&buf[p]);
				p += 188;
			} else {
				i = count - p;
				memcpy(m_TSBuf, &buf[p], i);
				m_TSBufP = i;
				goto bailout;
			}
		} else
			p++;
	}
	
bailout:
	;
	//	spin_unlock(&demux->lock);
}

void IDVBDemux::SWFilter204(const UInt8 *buf, size_t count)
{
	int p = 0, i, j;
	UInt8 tmppack[188];
	
	//	spin_lock(&demux->lock);
	
	if (m_TSBufP) {
		i = m_TSBufP;
		j = 204 - i;
		if (count < j) {
			memcpy(&m_TSBuf[i], buf, count);
			m_TSBufP += count;
			goto bailout;
		}
		memcpy(&m_TSBuf[i], buf, j);
		if ((m_TSBuf[0] == 0x47) || (m_TSBuf[0] == 0xB8)) {
			memcpy(tmppack, m_TSBuf, 188);
			if (tmppack[0] == 0xB8)
				tmppack[0] = 0x47;
			SWFilterPacket(tmppack);
		}
		m_TSBufP = 0;
		p += j;
	}
	
	while (p < count) {
		if ((buf[p] == 0x47) || (buf[p] == 0xB8)) {
			if (count - p >= 204) {
				memcpy(tmppack, &buf[p], 188);
				if (tmppack[0] == 0xB8)
					tmppack[0] = 0x47;
				SWFilterPacket(tmppack);
				p += 204;
			} else {
				i = count - p;
				memcpy(m_TSBuf, &buf[p], i);
				m_TSBufP = i;
				goto bailout;
			}
		} else {
			p++;
		}
	}
	
bailout:
	;
	//	spin_unlock(&demux->lock);
}

IDVBDemuxFilter *IDVBDemux::FilterAlloc()
{
	int i;
	
	for (i = 0; i < m_FilterNum; i++)
		if (m_Filter[i].State == DMX_STATE_FREE)
			break;
	
	if (i == m_FilterNum)
		return NULL;
	
	m_Filter[i].State = DMX_STATE_ALLOCATED;
	
	return &m_Filter[i];
}

/*
IDVBDemuxFeed *IDVBDemux::FeedAlloc()
{
	int i;
	
	for (i = 0; i < m_FeedNum; i++)
		if (m_Feed[i].GetState() == DMX_STATE_FREE)
			break;
	
	if (i == m_FeedNum)
		return NULL;
	
	m_Feed[i].SetState(DMX_STATE_ALLOCATED);

	return &m_Feed[i];
}
*/

int IDVBDemux::AllocateTSFeed(IDVBDemuxTSFeed **feed, IDVBDemuxTSFeedCallback callback)
{

	if (pthread_mutex_lock(&m_Mutex)) // interruptible
		return -EINVAL; //  -ERESTARTSYS; 
		
	if (m_FeedList.size() < m_FeedNum)
		*feed = new IDVBDemuxTSFeed();

	if (!*feed) {
		pthread_mutex_unlock(&m_Mutex);
		return -EBUSY;
	}

	m_FeedList.push_back(*feed);
	
	(*feed)->m_Type = DMX_TYPE_TS;
	(*feed)->m_Callback = callback;
	(*feed)->m_Demux = this;
	(*feed)->m_PID = 0xffff;
	(*feed)->m_PESLen = 0xfffa;
	(*feed)->m_Buffer = NULL;
	
//	(*feed)->m_Parent = dmx;
	(*feed)->m_Priv = NULL;
	(*feed)->m_IsFiltering = 0;
	
	if (!((*feed)->m_Filter = FilterAlloc())) {
		(*feed)->m_State = DMX_STATE_FREE;
		pthread_mutex_unlock(&m_Mutex);
		return -EBUSY;
	}
	
	(*feed)->m_Filter->Type = DMX_TYPE_TS;
	(*feed)->m_Filter->Feed = *feed;
	(*feed)->m_Filter->State = DMX_STATE_READY;
	
	pthread_mutex_unlock(&m_Mutex);
	
	return 0;
}

int IDVBDemux::ReleaseTSFeed(IDVBDemuxTSFeed *feed)
{
	
	pthread_mutex_lock(&m_Mutex);
	
	if (feed->m_State == DMX_STATE_FREE) {
		pthread_mutex_unlock(&m_Mutex);
		return -EINVAL;
	}
#ifndef NOBUFS
//	free(feed->m_Buffer);
	feed->m_Buffer = NULL;
#endif
	
	feed->m_State = DMX_STATE_FREE;
	feed->m_Filter->State = DMX_STATE_FREE;
	
//	dvb_demux_feed_del(feed);
	
	feed->m_PID = 0xffff;
	
	if (feed->m_TSType & TS_DECODER && feed->m_PESType < DMX_TS_PES_OTHER)
		m_PESFilter[feed->m_PESType] = NULL;
	
	pthread_mutex_unlock(&m_Mutex);
	return 0;
}

int IDVBDemux::AllocateSectionFeed(IDVBDemuxSectionFeed **feed,
								   IDVBDemuxSectionFeedCallback callback)
{
	if (pthread_mutex_lock(&m_Mutex))
		return -EINVAL;  //-ERESTARTSYS;
	if (m_FeedList.size() < m_FeedNum)
		*feed = new IDVBDemuxSectionFeed();
	
	if (!*feed) {
		pthread_mutex_unlock(&m_Mutex);
		return -EBUSY;
	}
	
	(*feed)->m_Type = DMX_TYPE_SEC;
	(*feed)->m_Callback = callback;
	(*feed)->m_Demux = this;
	(*feed)->m_PID = 0xffff;
	(*feed)->m_SecBuf = (*feed)->m_SecBufBase;
	(*feed)->m_SecBufP = (*feed)->m_SecLen = 0;
	(*feed)->m_TSFeedP = 0;
	(*feed)->m_Filter = NULL;
	(*feed)->m_Buffer = NULL;
	
	(*feed)->m_IsFiltering = 0;
//	(*feed)->m_Parent = demux;
	(*feed)->m_Priv = NULL;
	
	pthread_mutex_unlock(&m_Mutex);
	return 0;
}

int IDVBDemux::ReleaseSectionFeed(IDVBDemuxSectionFeed *feed)
{
	IDVBDemuxFeed *dvbdmxfeed = (IDVBDemuxFeed *)feed;
	pthread_mutex_lock(&m_Mutex);
	
	if (dvbdmxfeed->GetState() == DMX_STATE_FREE) {
		pthread_mutex_unlock(&m_Mutex);
		return -EINVAL;
	}
#ifndef NOBUFS
//	delete dvbdmxfeed->m_Buffer;
//	dvbdmxfeed->buffer = NULL;
#endif
	dvbdmxfeed->SetState(DMX_STATE_FREE);
	
//	FeedDel(dvbdmxfeed);
	
//	dvbdmxfeed->pid = 0xffff;
	
	pthread_mutex_unlock(&m_Mutex);
	return 0;
}


/*
struct dmx_frontend *IDVBDemux::GetFE(struct dmx_demux *demux, int type)
{
	struct list_head *head, *pos;
	
	head = demux->get_frontends(demux);
	if (!head)
	{
		return NULL;
		
	}
	list_for_each(pos, head)
	if (DMX_FE_ENTRY(pos)->source == type)
		return DMX_FE_ENTRY(pos);
	
	return NULL;
}
*/
