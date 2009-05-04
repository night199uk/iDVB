/*
 *  IDVBDemux.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

// Needed for the #defines
#include <vector>

#define DMX_MAX_PID 0x2000


#define DMX_STATE_FREE      0
#define DMX_STATE_ALLOCATED 1
#define DMX_STATE_SET       2
#define DMX_STATE_READY     3
#define DMX_STATE_GO        4

#ifndef DMX_MAX_FILTER_SIZE
#define DMX_MAX_FILTER_SIZE 18
#endif

// From demux.h

#define TS_PACKET       1   /* send TS packets (188 bytes) to callback (default) */
#define	TS_PAYLOAD_ONLY 2   /* in case TS_PACKET is set, only send the TS
payload (<=184 bytes per packet) to callback */
#define TS_DECODER      4   /* send stream to built-in decoder (if present) */
#define TS_DEMUX        8   /* in case TS_PACKET is set, send the TS to
the demux device, not to the dvr device */

// From DVB_DEMUX.h
#define DMX_TYPE_TS  0
#define DMX_TYPE_SEC 1
#define DMX_TYPE_PES 2

#define DMX_STATE_FREE      0
#define DMX_STATE_ALLOCATED 1
#define DMX_STATE_SET       2
#define DMX_STATE_READY     3
#define DMX_STATE_GO        4

#define DVB_DEMUX_MASK_MAX 18

#define DMX_TS_FILTERING                        1
#define DMX_PES_FILTERING                       2
#define DMX_SECTION_FILTERING                   4
#define DMX_MEMORY_BASED_FILTERING              8    /* write() available */
#define DMX_CRC_CHECKING                        16
#define DMX_TS_DESCRAMBLING                     32

namespace Video4Mac
{
	class IDVBAdapter;
	class IDVBFrontend;
	class IDVBDemuxFeed;
	class IDVBDemuxSectionFeed;
	class IDVBDemuxTSFeed;
	
	typedef struct IDVBDmxSectionFilter {
		UInt8 FilterValue [DMX_MAX_FILTER_SIZE];
		UInt8 FilterMask [DMX_MAX_FILTER_SIZE];
		UInt8 FilterMode [DMX_MAX_FILTER_SIZE];
		IDVBDemuxSectionFeed* Parent; /* Back-pointer */
		void* priv; /* Pointer to private data of the API client */
	} IDVBDmxSectionFilter;
	
	
	// This is defined twice overall. TBC
	typedef enum 
	{  /* also send packets to decoder (if it exists) */
		DMX_TS_PES_AUDIO0,
		DMX_TS_PES_VIDEO0,
		DMX_TS_PES_TELETEXT0,
		DMX_TS_PES_SUBTITLE0,
		DMX_TS_PES_PCR0,
			
		DMX_TS_PES_AUDIO1,
		DMX_TS_PES_VIDEO1,
		DMX_TS_PES_TELETEXT1,
		DMX_TS_PES_SUBTITLE1,
		DMX_TS_PES_PCR1,
			
		DMX_TS_PES_AUDIO2,
		DMX_TS_PES_VIDEO2,
		DMX_TS_PES_TELETEXT2,
		DMX_TS_PES_SUBTITLE2,
		DMX_TS_PES_PCR2,
			
		DMX_TS_PES_AUDIO3,
		DMX_TS_PES_VIDEO3,
		DMX_TS_PES_TELETEXT3,
		DMX_TS_PES_SUBTITLE3,
		DMX_TS_PES_PCR3,
		
		DMX_TS_PES_OTHER
	} kDVBDmxTSPES;

	typedef struct IDVBDemuxFilter {
		IDVBDmxSectionFilter Filter;
		UInt8 MaskAndMode[DMX_MAX_FILTER_SIZE];
		UInt8 MaskAndNotMode[DMX_MAX_FILTER_SIZE];
		int DoneQ;
		
		IDVBDemuxFilter*Next;
		IDVBDemuxFeed*	Feed;
		int				Index;
		int				State;
		int				Type;
		
		UInt16			hw_handle;
		//	struct timer_list timer;
	} IDVBDemuxFilter;
	
	typedef enum {
		DMX_OK = 0, /* Received Ok */
		DMX_LENGTH_ERROR, /* Incorrect length */
		DMX_OVERRUN_ERROR, /* Receiver ring buffer overrun */
		DMX_CRC_ERROR, /* Incorrect CRC */
		DMX_FRAME_ERROR, /* Frame alignment error */
		DMX_FIFO_ERROR, /* Receiver FIFO overrun */
		DMX_MISSED_ERROR /* Receiver missed packet */
	} kDVBDemuxSuccess;
	
	typedef int (*IDVBDemuxSectionFeedCallback) (const UInt8 * buffer1,
												 size_t buffer1_len,
												 const UInt8 * buffer2,
												 size_t buffer2_len,
												 IDVBDmxSectionFilter* source,
												 kDVBDemuxSuccess success);
	
	typedef int (*IDVBDemuxTSFeedCallback) (const UInt8 * buffer1,
											size_t buffer1_length,
											const UInt8 * buffer2,
											size_t buffer2_length,
											IDVBDemuxTSFeed* source,
											kDVBDemuxSuccess success);
	class IDVBDemux
	{
		// Replace this with accessors later
		friend class IDVBDemuxFeed;
		friend class IDVBDemuxSectionFeed;
		friend class IDVBDemuxTSFeed;
		
	public:
		IDVBDemux(IDVBAdapter* Adapter, int FilterNum, int FeedNum);
		~IDVBDemux();
		
		void			SetAdapter(IDVBAdapter*		Adapter)			{ m_Adapter = Adapter; }
		IDVBAdapter*	GetAdapter(void)								{ return m_Adapter;	}
		
		void			SetFilterNum(int			FilterNum)			{ m_FilterNum = FilterNum; }
		int				GetFilterNum(void)								{ return m_FilterNum;	}
		
		void			SetFeedNum(int			FeedNum)				{ m_FeedNum = FeedNum; }
		int				GetFeedNum(void)								{ return m_FeedNum;	}
		
		int				Initialize();
		
		// API Functions that might be subclassed
		virtual int		StartFeed(IDVBDemuxFeed* dvbdmxfeed) = 0;
		virtual int		StopFeed(IDVBDemuxFeed* dvbdmxfeed) = 0;
		virtual int		WriteToDecoder(IDVBDemuxFeed* dvbdmxfeed, const UInt8 *buf, ssize_t count) { return -EINVAL; }
		int				AllocateTSFeed(IDVBDemuxTSFeed **feed, IDVBDemuxTSFeedCallback callback);
		int				ReleaseTSFeed(IDVBDemuxTSFeed *feed);
		void			SWFilter(const UInt8 *buf, size_t count);
		int				AllocateSectionFeed(IDVBDemuxSectionFeed **feed, IDVBDemuxSectionFeedCallback callback);
		int				ReleaseSectionFeed(IDVBDemuxSectionFeed *feed);
		
	private:
		void			SWFilterPacket(const UInt8 *buf);
		void			SWFilterPackets(const UInt8 *buf, size_t count);
		void			SWFilter204(const UInt8 *buf, size_t count);
		
		IDVBDemuxFilter*	FilterAlloc();
		IDVBDemuxFeed*		FeedAlloc();
		
		IDVBAdapter*		m_Adapter;
		int					m_FilterNum;
		int					m_FeedNum;
		int					m_Users;
		IDVBDemuxFilter*	m_Filter;
		
		std::vector<IDVBFrontend *>	m_FrontendList;
		
		int					m_Playing;
		int					m_Recording;
		UInt8				m_TSBuf[204];
		int					m_TSBufP;
		
		pthread_mutex_t		m_Mutex;
		
		IDVBDemuxFeed*		m_Feed;
		IDVBDemuxFeed*		m_PESFilter[DMX_TS_PES_OTHER];
		
		UInt16				m_PIDs[DMX_TS_PES_OTHER];
		std::vector<IDVBDemuxFeed *>	m_FeedList;
		
		/*
		 void *priv;
		 
		 int (*start_feed)(struct dvb_demux_feed *feed);
		 int (*stop_feed)(struct dvb_demux_feed *feed);
		 int (*write_to_decoder)(struct dvb_demux_feed *feed,
		 const UInt8 *buf, size_t len);
		 UInt32 (*check_crc32)(struct dvb_demux_feed *feed,
		 const UInt8 *buf, size_t len);
		 void (*memcopy)(struct dvb_demux_feed *feed, UInt8 *dst,
		 const UInt8 *src, size_t len);
		 
		 int users;
		 #define MAX_DVB_DEMUX_USERS 10
		 
		 struct dvb_demux_feed *feed;
		 
		 struct list_head frontend_list;
		 
		 
		 
		 int playing;
		 int recording;
		 
		 #define DMX_MAX_PID 0x2000
		 struct list_head feed_list;
		 
		 int tsbufp;
		 */
		
		//	OSSpinLock lock;
		/* spinlock_t lock; */
		
	};
};