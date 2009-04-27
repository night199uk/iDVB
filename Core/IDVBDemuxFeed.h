/*
 *  IDVBDemuxFeed.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <IDVBDemux.h>

namespace Video4Mac
{
	class IDVBDmxDevFilter;
	
	class IDVBDemuxFeed {
	public:
		inline void memcopy(IDVBDemuxFeed *f, UInt8 *d, const UInt8 *s,
									 size_t len);
		
		void		SetPID(UInt16	PID)				{ m_PID = PID;	};
		UInt16		GetPID(void)						{ return m_PID;  };
		
		void		SetType(int	Type)					{ m_Type = Type;	};
		int			GetType(void)						{ return m_Type;	};
		
		void		SetTSType(int	TSType)				{ m_TSType = TSType;	};
		int			GetTSType(void)						{ return m_TSType;		};

		void		SetState(int	State)				{ m_State = State;		};
		int			GetState(void)						{ return m_State;		};
		
		int			GetIsFiltering()					{ return m_IsFiltering;	};
		
		void		SWFilterPacketType(const UInt8 *buf);
		int			SWFilterSectionCopyDump(const UInt8 *buf, UInt8 len);
		
		virtual int	StartFiltering() = 0;
		virtual int StopFiltering() = 0;
		
		IDVBDmxDevFilter	*m_DmxFilter;
		
	protected:
		
		void				memcopy(UInt8 *d, const UInt8 *s, size_t len);
		
		// Moved from the lower level modules (TSFeed & SectionFeed)
		int					m_IsFiltering;

		IDVBDemux*			m_Demux;
		IDVBDemuxFilter*	m_Filter;

		
		int					m_CC;
		int					m_PusiSeen;		/* prevents feeding of garbage from previous section */
		int					m_Type;
		UInt16				m_PID;
		int					m_TSType;
		struct timespec		m_Timeout;
		
		UInt8*				m_Buffer;
		int					m_BufferSize;
		kDVBDmxTSPES		m_PESType;
		int					m_State;
		UInt16				m_PESLen;
		void*				m_Priv;

	private:
		unsigned int m_Index;	/* a unique index for each feed (can be used as hardware pid filter index) */
		
	

	} ;	
}
