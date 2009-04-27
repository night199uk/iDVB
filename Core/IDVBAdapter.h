/*
 *  IDVBAdapter.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <Video4Mac/IDVBDmxDevFilter.h>
#include <string>

namespace Video4Mac
{
	class IDVBFrontend;
	class IDVBDemux;
	class IDVBDmxDevFilter;
	class CDVBRingBuffer;
	
	class IDVBAdapter
	{
	public:
		IDVBAdapter();
		virtual ~IDVBAdapter();

		// Buffer
		virtual CDVBRingBuffer*	GetBuffer()								{	return m_Buffer;};
		
		// Name
		virtual std::string		GetName()								{	return m_Name;	};
		virtual void			SetName(std::string Name)				{	m_Name = Name;  };
		
		// Frontend
		virtual void			SetDemux(IDVBDemux* Demux)				{	m_Demux = Demux;  };
		virtual IDVBDemux*		GetDemux()								{	return m_Demux;	};

		virtual void			SetFrontend(IDVBFrontend* Frontend)		{	m_Frontend = Frontend;  };
		virtual IDVBFrontend*	GetFrontend()							{	return m_Frontend;	};

		virtual void			SetFilterNum(int			FilterNum)	{ m_FilterNum = FilterNum; m_Filter = new IDVBDmxDevFilter[m_FilterNum];  }
		virtual int				GetFilterNum(void)						{ return m_FilterNum;	}
		
		IDVBDmxDevFilter*		GetFilter();
		int						Release(IDVBDmxDevFilter *dmxdevfilter);
		
		virtual int	FrontendAttach() = 0;
		ssize_t					Read(char /* __user */ *buf, size_t count, long long *ppos);
		
	protected:
		std::string		m_Name;
		IDVBFrontend*	m_Frontend;
		IDVBDemux*		m_Demux;
//		IDVBDmxDev*		m_DmxDev;
		
		CDVBRingBuffer*		m_Buffer;
		IDVBDmxDevFilter*	m_Filter;
		pthread_mutex_t		m_Mutex;
		int					m_FilterNum;
	private:
//		bool			m_MFEShared;
	};
};