/*
 *  IDVBUSBAdapter.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <IDVBAdapter.h>

#define MAX_NO_URBS_FOR_DATA_STREAM 10

#define DVB_USB_ADAP_STATE_INIT 0x000
#define DVB_USB_ADAP_STATE_DVB  0x001

#define USB_BULK  1
#define USB_ISOC  2

typedef struct {
	int Type;
	int Count;
	int Endpoint;
	union  {
		struct {
			int BufferSize; // per URB
		} Bulk;
		struct {
			int FramesPerURB;
			int FrameSize;
			int Interval;
		} Isoc;
	} u;
} IDVBUSBStreamProperties;

namespace Video4Mac
{
	class IDVBUSBDevice;
	
	class IDVBUSBAdapter : public IDVBAdapter
	{
	public:
		IDVBUSBAdapter(IDVBUSBDevice *Device, int Id);
		virtual ~IDVBUSBAdapter();
		
		virtual int		Initialize();
		
		void			SetUSBDevice(IDVBUSBDevice *USBDevice)			{ m_USBDevice = USBDevice; }
		IDVBUSBDevice*	GetUSBDevice(void)								{ return m_USBDevice;     }
		
		void			SetId(int Id)									{ m_Id = Id; }
		int				GetId(void)										{ return m_Id;	}
		
		void			SetFeedcount(int			Feedcount)			{ m_Feedcount = Feedcount; }
		int				GetFeedcount(void)								{ return m_Feedcount;	}
		
		void			SetState(int			State)					{ m_State = State; }
		int				GetState(void)									{ return m_State;	}
		
		virtual int StreamingControl(int) { return 0; };
		virtual int PIDFilterControl(int) { return 0; };
		virtual int PIDFilter(int, UInt16, int) { return 0; };
		virtual int	FrontendAttach()		{ return 0; };

		int				USBURBKill();
		int				USBURBSubmit();

	protected:
		IDVBUSBDevice*	m_USBDevice;
        int				m_Id;
		int				m_PIDFiltering;
		int				m_MaxFeedCount;
		int				m_Feedcount;
		int				m_State;
		
		// Stream handling
		int				m_URBsSubmitted;
		int				m_URBsInitialized;
		int				m_BufNum;
		unsigned long	m_BufSize;

		// USB
		int				USBURBInit(IDVBUSBStreamProperties *props);
		
	private:
		
		void			DataComplete(UInt8 *buffer, size_t length);
		int				AllocateStreamBuffers(int num, unsigned long size);
		int				USBBulkURBInit();
		
		IDVBUSBStreamProperties	m_StreamProperties;
		
		struct urb    *m_URBList[MAX_NO_URBS_FOR_DATA_STREAM];
		UInt8         *m_BufList[MAX_NO_URBS_FOR_DATA_STREAM];		
 	};
};