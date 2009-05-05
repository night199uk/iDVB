/*
 *  IDVBDmxSectionFeed.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <IDVBDemuxFeed.h>

/*
 * DMX_MAX_SECFEED_SIZE: Maximum length (in bytes) of a private section feed filter.
 */

#ifndef DMX_MAX_SECTION_SIZE
#define DMX_MAX_SECTION_SIZE 4096
#endif
#ifndef DMX_MAX_SECFEED_SIZE
#define DMX_MAX_SECFEED_SIZE (DMX_MAX_SECTION_SIZE + 188)
#endif

namespace Video4Mac
{
	
	class IDVBDemuxSectionFeed : public IDVBDemuxFeed {
		friend class IDVBDemux;
		friend class IDVBDemuxFeed;
	
	public:
		inline UInt32			CRC32(const UInt8 *src, size_t len);
		
		// Main entry point from the demuxer
		int						SWFilterSectionPacket(const UInt8 *buf);
		
		// API Functions
		int						Set(UInt16 pid, size_t circular_buffer_size, int check_crc);
		int						AllocateFilter(IDVBDmxSectionFilter **filter);
		int						ReleaseFilter(IDVBDmxSectionFilter *filter);
		int						StartFiltering();
		int						StopFiltering();

	protected:
		UInt16	m_SecLen;
		//		struct dmx_demux* parent; /* Back-pointer */
		//		void* priv; /* Pointer to private data of the API client */
		
		int		m_CheckCRC;
		UInt32	m_CRCVal;
		
		UInt8*	m_SecBuf;
		UInt8	m_SecBufBase[DMX_MAX_SECFEED_SIZE];
		UInt16	m_SecBufP;

		
		
		int		m_CC;
		int		m_PusiSeen;		/* prevents feeding of garbage from previous section */
		UInt16	m_TSFeedP;
		
	private:
		void				PrepareSecFilters();
		
		int					SWFilterSectionFilter(IDVBDemuxFilter *f);
		inline int			SWFilterSectionFeed();
		void				SWFilterSectionNew();
		int					SWFilterSectionCopyDump(const UInt8 *buf, UInt8 len);
	

	

	};
}

