/*
 *  IDVBDmxTSFeed.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include <IDVBDemuxFeed.h>

namespace Video4Mac
{
	class IDVBDemuxTSFeed : public IDVBDemuxFeed
	{
		friend class IDVBDemux;
		friend class IDVBDemuxFeed;
	public:
		int							SWFilterPayload(const UInt8 *buf);
		
		// API functions
		int							Set(UInt16 pid, int ts_type,  kDVBDmxTSPES pes_type, size_t circular_buffer_size, struct timespec timeout);
		int							StartFiltering();
		int							StopFiltering();
	private:

		IDVBDemuxTSFeedCallback m_Callback;
	};
};