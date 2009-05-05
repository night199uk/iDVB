/*
 *  IDVBDmxDevFilter.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 14/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once 
#include <Video4Mac/IDVBDmxPublic.h>
#include <Video4Mac/IDVBDemux.h>
#include <Video4Mac/IDVBEvent.h>
#include <Video4Mac/IDVBPrimitives.h>

#define DMX_START				41
#define DMX_STOP				42
#define DMX_SET_FILTER			43
#define DMX_SET_PES_FILTER		44
#define DMX_SET_BUFFER_SIZE		45
#define DMX_GET_PES_PIDS		47
#define DMX_GET_CAPS			48
#define DMX_SET_SOURCE			49
#define DMX_GET_STC				50

namespace Video4Mac
{
	class CDVBRingBuffer;
	
	typedef enum  {
		DMXDEV_TYPE_NONE,
		DMXDEV_TYPE_SEC,
		DMXDEV_TYPE_PES,
	} kDVBDmxDevType;
	
	typedef enum  {
		DMXDEV_STATE_FREE,
		DMXDEV_STATE_ALLOCATED,
		DMXDEV_STATE_SET,
		DMXDEV_STATE_GO,
		DMXDEV_STATE_DONE,
		DMXDEV_STATE_TIMEDOUT
	} kDVBDmxDevState;
	
	class IDVBDmxDevFilter : public IDVBEventSource
	{
	public:
		virtual ~IDVBDmxDevFilter();
		
		// Frontend
		virtual void			SetDemux(IDVBDemux* Demux)					{	m_Demux = Demux;  };
		virtual IDVBDemux*		GetDemux()									{	return m_Demux;	};		

		virtual void			SetAdapter(IDVBAdapter*		Adapter)		{ m_Adapter = Adapter; }
		virtual IDVBAdapter*	GetAdapter(void)							{ return m_Adapter;	}
		
		void			Initialize();
		
		void			SetState(kDVBDmxDevState state) { m_State = state; };
		kDVBDmxDevState	GetState()						{ return m_State; }
		int				Stop();
		inline int		Reset();
		int				Start();
		int				IOCtl(unsigned int cmd, void *parg);
		int				TSCallback(const UInt8 *buffer1, size_t buffer1_len,
								   const UInt8 *buffer2, size_t buffer2_len,
								   IDVBDemuxTSFeed *feed,
								   kDVBDemuxSuccess success);
		
		int				SectionCallback(const UInt8 *buffer1, size_t buffer1_len,
										const UInt8 *buffer2, size_t buffer2_len,
										IDVBDmxSectionFilter *filter,
										kDVBDemuxSuccess success);

		ssize_t			Read(char *buf, size_t count, long long *ppos);
		int				Free();
		
	private:
		ssize_t			ReadSec(char  *buf, size_t count, long long *ppos);
		int				SetBufferSize(unsigned long size);
		inline void		StateSet(int state);
		void			Timeout(unsigned long data);
		void			Timer();

		unsigned int	Poll(CDVBCondition *Condition);
		int				FeedStop();
		int				FeedStart();
		int				FeedRestart();

		int				Set(IDVBDmxSctFilterParams *params);
		int				PESFilterSet(IDVBDmxPESFilterParams *params);
		
		union {
			IDVBDmxSectionFilter *Sec;
		} m_Filter;
		
		IDVBDemuxFeed *m_Feed;
		
		union {
			IDVBDmxSctFilterParams Sec;
			IDVBDmxPESFilterParams PES;
		} m_Params;
		
		IDVBAdapter*			m_Adapter;
		IDVBDemux*				m_Demux;
		kDVBDmxDevType			m_Type;
		kDVBDmxDevState			m_State;
		CDVBRingBuffer*			m_Buffer;
		CDVBWaitQueue			m_WaitQueue;
		pthread_mutex_t			m_Mutex;
		
		/* only for sections */
		//	struct timer_list timer;
		int						m_Todo;
		UInt8					m_SecHeader[3];
		
	};
};

