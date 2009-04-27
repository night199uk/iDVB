/*
 *  CDVBUSBDemux.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <IDVBDemux.h>

namespace Video4Mac
{
	class CDVBUSBDemux : public IDVBDemux
	{
	public:
		CDVBUSBDemux(IDVBAdapter* Adapter, int FilterNum, int FeedNum);
		~CDVBUSBDemux();
		
		int			ControlFeed(IDVBDemuxFeed* dvbdmxfeed, int onoff);
		int			StartFeed(IDVBDemuxFeed* dvbdmxfeed);
		int			StopFeed(IDVBDemuxFeed* dvbdmxfeed);
	private:
	};
};