/*
 *  IDVBDemuxFeed.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBDemuxFeed.h"
#include <IDVBDemuxTSFeed.h>
#include <IDVBDemuxSectionFeed.h>
#include <IDVBDmxDevFilter.h>

using namespace Video4Mac;


/******************************************************************************
 * Software filter functions
 ******************************************************************************/

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

void IDVBDemuxFeed::memcopy(UInt8 *d, const UInt8 *s, size_t len)
{
	memcpy(d, s, len);
};

void IDVBDemuxFeed::SWFilterPacketType(const UInt8 *buf)
{
	IDVBDemuxTSFeed *TS = (IDVBDemuxTSFeed *)this;
	IDVBDemuxSectionFeed *Sec = (IDVBDemuxSectionFeed *)this;
	
	switch (m_Type) {
		case DMX_TYPE_TS:
			if (!m_IsFiltering)
				break;
			
			if (m_TSType & TS_PACKET)
				if (m_TSType & TS_PAYLOAD_ONLY)
					TS->SWFilterPayload(buf);
				else
					m_DmxFilter->TSCallback(buf, 188, NULL, 0, TS, DMX_OK);
			
			if (m_TSType & TS_DECODER)
				m_Demux->WriteToDecoder(this, buf, 188);

			break;
		case DMX_TYPE_SEC:
			if (!m_IsFiltering)
				break;
			if (Sec->SWFilterSectionPacket(buf) < 0)
				Sec->m_SecLen = Sec->m_SecBufP = 0;
			break;
			
		default:
			break;
	}
}

/*
static int IDVBDemuxFeed::FeedFind()
{
	
	struct dvb_demux_feed *entry;
	
	list_for_each_entry(entry, &feed->demux->feed_list, list_head)
	if (entry == feed)
		return 1;
	
	return 0;
}

static void dvb_demux_feed_add(struct dvb_demux_feed *feed)
{
	//	spin_lock_irq(&feed->demux->lock);
	if (dvb_demux_feed_find(feed)) {
		printf("%s: feed already in list (type=%x state=%x pid=%x)\n",
		       __func__, feed->type, feed->state, feed->pid);
		goto out;
	}
	
	list_add(&feed->list_head, &feed->demux->feed_list);
out:
	;
	//	spin_unlock_irq(&feed->demux->lock);
}

static void dvb_demux_feed_del(struct dvb_demux_feed *feed)
{
	//	spin_lock_irq(&feed->demux->lock);
	if (!(dvb_demux_feed_find(feed))) {
		printf("%s: feed not in list (type=%x state=%x pid=%x)\n",
		       __func__, feed->type, feed->state, feed->pid);
		goto out;
	}
	
	list_del(&feed->list_head);
out:
	;
	//	spin_unlock_irq(&feed->demux->lock);
}
*/