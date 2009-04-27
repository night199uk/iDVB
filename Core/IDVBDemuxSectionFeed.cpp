/*
 *  IDVBDmxSectionFeed.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBDemuxSectionFeed.h"
using namespace Video4Mac;

#define CRCPOLY_BE 0x04c11db7

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

inline UInt32 crc32_be(UInt32 crc, unsigned char const *p, size_t len)
{
    int i;
    while (len--) {
        crc ^= *p++ << 24;
        for (i = 0; i < 8; i++)
            crc =
			(crc << 1) ^ ((crc & 0x80000000) ? CRCPOLY_BE :
						  0);
    }
    return crc;
}


UInt32 IDVBDemuxSectionFeed::CRC32(const UInt8 *src, size_t len)
{
	return (m_CRCVal = crc32_be(m_CRCVal, src, len));
};

int IDVBDemuxSectionFeed::SWFilterSectionFilter(IDVBDemuxFilter *f)
{
	UInt8 neq = 0;
	int i;
	
	for (i = 0; i < DVB_DEMUX_MASK_MAX; i++) {
		UInt8 txor = f->Filter.FilterValue[i] ^ m_SecBuf[i];
		
		if (f->MaskAndMode[i] & txor)
			return 0;
		
		neq |= f->MaskAndNotMode[i] & txor;
	}
	
	if (f->DoneQ && !neq)
		return 0;
	
	return m_Callback(m_SecBuf, m_SecLen,
					 NULL, 0, &f->Filter, DMX_OK);
}


inline int IDVBDemuxSectionFeed::SWFilterSectionFeed()
{
//	IDVBDemux *demux = m_Demux;
	IDVBDemuxFilter *f = m_Filter;
//	IDVBDemuxSectionFeed *sec = (IDVBDemuxSectionFeed *)this;
	int section_syntax_indicator;
	
	if (!m_IsFiltering)
		return 0;
	
	if (!f)
		return 0;
	
	if (m_CheckCRC) {
		section_syntax_indicator = ((m_SecBuf[1] & 0x80) != 0);
		if (section_syntax_indicator &&
		    CRC32(m_SecBuf, m_SecLen))
			return -1;
	}
	
	do {
		if (SWFilterSectionFilter(f) < 0)
			return -1;
	} while ((f = f->Next) && m_IsFiltering);
	
	m_SecLen = 0;
	
	return 0;
}

void IDVBDemuxSectionFeed::SWFilterSectionNew()
{

#ifdef DVB_DEMUX_SECTION_LOSS_LOG
	if (m_SecBufP < m_TSFeedP) {
		int i, n = m_TSFeedP - m_SecBufP;
		
		/*
		 * Section padding is done with 0xff bytes entirely.
		 * Due to speed reasons, we won't check all of them
		 * but just first and last.
		 */
		if (m_SecBuf[0] != 0xff || m_SecBuf[n - 1] != 0xff) {
			printk("dvb_demux.c section ts padding loss: %d/%d\n",
			       n, m_TSFeedP);
			printk("dvb_demux.c pad data:");
			for (i = 0; i < n; i++)
				printk(" %02x", m_SecBuf[i]);
			printk("\n");
		}
	}
#endif
	
	m_TSFeedP = m_SecBufP = m_SecLen = 0;
	m_SecBuf = m_SecBufBase;
}

int IDVBDemuxSectionFeed::SWFilterSectionCopyDump(const UInt8 *buf, UInt8 len)
{
//	struct dvb_demux *demux = feed->demux;
	UInt16 limit, seclen, n;
	
	if (m_TSFeedP >= DMX_MAX_SECFEED_SIZE)
		return 0;
	
	if (m_TSFeedP + len > DMX_MAX_SECFEED_SIZE) {
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
		printk("dvb_demux.c section buffer full loss: %d/%d\n",
		       m_TSFeedP + len - DMX_MAX_SECFEED_SIZE,
		       DMX_MAX_SECFEED_SIZE);
#endif
		len = DMX_MAX_SECFEED_SIZE - m_TSFeedP;
	}
	
	if (len <= 0)
		return 0;
	
	memcopy(m_SecBufBase + m_TSFeedP, buf, len);
	m_TSFeedP += len;
	
	/*
	 * Dump all the sections we can find in the data (Emard)
	 */
	limit = m_TSFeedP;
	if (limit > DMX_MAX_SECFEED_SIZE)
		return -1;	/* internal error should never happen */
	
	/* to be sure always set secbuf */
	m_SecBuf = m_SecBufBase + m_SecBufP;
	
	for (n = 0; m_SecBufP + 2 < limit; n++) {
		seclen = section_length(m_SecBuf);
		if (seclen <= 0 || seclen > DMX_MAX_SECTION_SIZE
		    || seclen + m_SecBufP > limit)
			return 0;
		m_SecLen = seclen;
		m_CRCVal = ~0;
		/* dump [secbuf .. secbuf+seclen) */
		if (m_PusiSeen)
			SWFilterSectionFeed();
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
		else
			printk("dvb_demux.c pusi not seen, discarding section data\n");
#endif
		m_SecBufP += seclen;	/* secbufp and secbuf moving together is */
		m_SecBuf += seclen;	/* redundant but saves pointer arithmetic */
	}
	
	return 0;
}

/*
 * Losless Section Demux 1.4.1 by Emard
 * Valsecchi Patrick:
 *  - middle of section A  (no PUSI)
 *  - end of section A and start of section B
 *    (with PUSI pointing to the start of the second section)
 *
 *  In this case, without feed->pusi_seen you'll receive a garbage section
 *  consisting of the end of section A. Basically because tsfeedp
 *  is incemented and the use=0 condition is not raised
 *  when the second packet arrives.
 *
 * Fix:
 * when demux is started, let feed->pusi_seen = 0 to
 * prevent initial feeding of garbage from the end of
 * previous section. When you for the first time see PUSI=1
 * then set feed->pusi_seen = 1
 */

int IDVBDemuxSectionFeed::SWFilterSectionPacket(const UInt8 *buf)
{
	UInt8 p, count;
	int ccok, dc_i = 0;
	UInt8 cc;
	
	count = payload(buf);
	
	if (count == 0)		/* count == 0 if no payload or out of range */
		return -1;
	
	p = 188 - count;	/* payload start */
	
	cc = buf[3] & 0x0f;
	ccok = ((m_CC + 1) & 0x0f) == cc;
	m_CC = cc;
	
	if (buf[3] & 0x20) {
		/* adaption field present, check for discontinuity_indicator */
		if ((buf[4] > 0) && (buf[5] & 0x80))
			dc_i = 1;
	}
	
	if (!ccok || dc_i) {
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
		printk("dvb_demux.c discontinuity detected %d bytes lost\n",
		       count);
		/*
		 * those bytes under sume circumstances will again be reported
		 * in the following IDVBDmx::swfilter_section_new
		 */
#endif
		/*
		 * Discontinuity detected. Reset pusi_seen = 0 to
		 * stop feeding of suspicious data until next PUSI=1 arrives
		 */
		m_PusiSeen = 0;
		SWFilterSectionNew();
	}
	
	if (buf[1] & 0x40) {
		/* PUSI=1 (is set), section boundary is here */
		if (count > 1 && buf[p] < count) {
			const UInt8 *before = &buf[p + 1];
			UInt8 before_len = buf[p];
			const UInt8 *after = &before[before_len];
			UInt8 after_len = count - 1 - before_len;
			
			SWFilterSectionCopyDump(before,	before_len);
			/* before start of new section, set pusi_seen = 1 */
			m_PusiSeen = 1;
			SWFilterSectionNew();
			SWFilterSectionCopyDump(after, after_len);
		}
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
		else if (count > 0)
			printk("dvb_demux.c PUSI=1 but %d bytes lost\n", count);
#endif
	} else {
		/* PUSI=0 (is not set), no section boundary */
		SWFilterSectionCopyDump(&buf[p], count);
	}
	
	return 0;
}




//////////////////////////////////////
//
// API functions
//
//////////////////////////////////////

int IDVBDemuxSectionFeed::AllocateFilter(IDVBDmxSectionFilter **filter)
{
	struct IDVBDemuxFilter *dvbdmxfilter;
	
	if (pthread_mutex_lock(&m_Demux->m_Mutex))
		return -EINVAL; /*-ERESTARTSYS; */ 
	
	dvbdmxfilter = m_Demux->FilterAlloc();
	if (!dvbdmxfilter) {
		pthread_mutex_unlock(&m_Demux->m_Mutex);
		return -EBUSY;
	}
	
	//	spin_lock_irq(&dvbdemux->lock);
	*filter = &dvbdmxfilter->Filter;
	(*filter)->Parent = this;
	(*filter)->priv = NULL;
	dvbdmxfilter->Feed = this;
	dvbdmxfilter->Type = DMX_TYPE_SEC;
	dvbdmxfilter->State = DMX_STATE_READY;
	dvbdmxfilter->Next = m_Filter;
	m_Filter = dvbdmxfilter;
	//	spin_unlock_irq(&dvbdemux->lock);
	
	pthread_mutex_unlock(&m_Demux->m_Mutex);
	return 0;
}

int IDVBDemuxSectionFeed::Set(UInt16 pid, size_t circular_buffer_size, int check_crc)
{
	struct IDVBDemux *dvbdmx = m_Demux;
	
	if (pid > 0x1fff)
		return -EINVAL;
	
	if (pthread_mutex_lock(&dvbdmx->m_Mutex))
		return -EINVAL; // -ERESTARTSYS;
	
//	dvb_demux_feed_add(dvbdmxfeed);
	
	m_PID = pid;
	m_BufferSize = circular_buffer_size;
	m_CheckCRC = check_crc;
	
#ifdef NOBUFS
	m_Buffer = NULL;
#else
	m_Buffer = (UInt8 *)malloc(m_BufferSize);
	if (!m_Buffer) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return -ENOMEM;
	}
#endif
	
	m_State = DMX_STATE_READY;
	pthread_mutex_unlock(&dvbdmx->m_Mutex);
	return 0;
}

void IDVBDemuxSectionFeed::PrepareSecFilters()
{
	int i;
	IDVBDemuxFilter *f;
	IDVBDmxSectionFilter *sf;
	
	UInt8 mask, mode, doneq;
	
	if (!(f = m_Filter))
		return;
	
	do {
		sf = &f->Filter;
		doneq = 0;
		for (i = 0; i < DVB_DEMUX_MASK_MAX; i++) {
			mode = sf->FilterMode[i];
			mask = sf->FilterMask[i];
			f->MaskAndMode[i] = mask & mode;
			doneq |= f->MaskAndNotMode[i] = mask & ~mode;
		}
		f->DoneQ = doneq ? 1 : 0;
	} while ((f = f->Next));
}

int IDVBDemuxSectionFeed::StartFiltering()
{
	struct IDVBDemux *dvbdmx = m_Demux;
	int ret;
	
	if (pthread_mutex_lock(&dvbdmx->m_Mutex))
		return -EINVAL; /*-ERESTARTSYS; */ 
	
	if (m_IsFiltering) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return -EBUSY;
	}
	
	if (!m_Filter) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return -EINVAL;
	}
	
	m_TSFeedP = 0;
	m_SecBuf = m_SecBufBase;
	m_SecBufP = 0;
	m_SecLen = 0;

	/*
	if (!dvbdmx->start_feed) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return -ENODEV;
	}
	*/
	PrepareSecFilters();
	
	if ((ret = dvbdmx->StartFeed(this)) < 0) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return ret;
	}
	
	//	spin_lock_irq(&dvbdmx->lock);
	m_IsFiltering = 1;
	m_State = DMX_STATE_GO;
	//	spin_unlock_irq(&dvbdmx->lock);
	
	pthread_mutex_unlock(&dvbdmx->m_Mutex);
	return 0;
}

int IDVBDemuxSectionFeed::StopFiltering()
{
	struct IDVBDemux *dvbdmx = m_Demux;
	int ret;
	
	pthread_mutex_lock(&dvbdmx->m_Mutex);
	
	/*
	if (!dvbdmx->stop_feed) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return -ENODEV;
	}
	*/
	
	ret = dvbdmx->StopFeed(this);
	
	//	spin_lock_irq(&dvbdmx->lock);
	m_State = DMX_STATE_READY;
	m_IsFiltering = 0;
	//	spin_unlock_irq(&dvbdmx->lock);
	
	pthread_mutex_unlock(&dvbdmx->m_Mutex);
	return ret;
}

int IDVBDemuxSectionFeed::ReleaseFilter(IDVBDmxSectionFilter *filter)
{
	IDVBDemuxFilter *dvbdmxfilter = (IDVBDemuxFilter *)filter, *f;
	IDVBDemux *dvbdmx = m_Demux;
	
	pthread_mutex_lock(&dvbdmx->m_Mutex);
	
	if (dvbdmxfilter->Feed != (IDVBDemuxFeed *)this) {
		pthread_mutex_unlock(&dvbdmx->m_Mutex);
		return -EINVAL;
	}
	
	if (m_IsFiltering)
		StopFiltering();
	
	//	spin_lock_irq(&dvbdmx->lock);
	f = m_Filter;
	
	if (f == dvbdmxfilter) {
		m_Filter = dvbdmxfilter->Next;
	} else {
		while (f->Next != dvbdmxfilter)
			f = f->Next;
		f->Next = f->Next->Next;
	}
	
	dvbdmxfilter->State = DMX_STATE_FREE;
	//	spin_unlock_irq(&dvbdmx->lock);
	pthread_mutex_unlock(&dvbdmx->m_Mutex);
	return 0;
}
