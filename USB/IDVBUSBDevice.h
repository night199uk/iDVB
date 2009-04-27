/*
 *  IDVBUSBDevice.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 01/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <MacDVB.h>
#include <DVBUSB.h>
#include <DVBFirmware.h>
#include <IDVBI2CAdapter.h>
#include <IThread.h>

namespace Video4Mac
{
	class IDVBUSBAdapter;

	struct urb
	{
		/* private: usb core and host controller only fields in the urb */
		//	struct kref kref;               /* reference count of the URB */
		
		pthread_mutex_t lock;			/* lock for the URB */
		//	spinlock_t lock;                /* lock for the URB */
		
		//	void *hcpriv;                   /* private data for host controller */
		//	int bandwidth;                  /* bandwidth for INT/ISO request */
		//	atomic_t use_count;             /* concurrent submissions counter */
		//	u8 reject;                      /* submissions will fail */
		
		/* public: documented fields in the urb that can be used by drivers */
		IOUSBInterfaceInterface300 **intf;         /* (in) pointer to associated device */
		
		IDVBUSBDevice*			device;
		IDVBUSBAdapter*			adap;
		void (IDVBUSBAdapter::*method)(UInt8 *, size_t);
		
		unsigned int pipe;					/* (in) pipe information */
		int status;                     /* (return) non-ISO status */
		unsigned int transfer_flags;    /* (in) URB_SHORT_NOT_OK | ...*/
		void *transfer_buffer;          /* (in) associated data buffer */
		//	dma_addr_t transfer_dma;        /* (in) dma addr for transfer_buffer */
		int transfer_buffer_length;     /* (in) data buffer length */
		int actual_length;              /* (return) actual transfer length */
		unsigned char *setup_packet;    /* (in) setup packet (control only) */
		//	dma_addr_t setup_dma;           /* (in) dma addr for setup_packet */
		int start_frame;                /* (modify) start frame (ISO) */
		int number_of_packets;          /* (in) number of ISO packets */
		int interval;                   /* (modify) transfer interval
		 * (INT/ISO) */
		int error_count;                /* (return) number of ISO errors */
		void *context;                  /* (in) context for completion */
	};
	
	
	class IDVBUSBDevice : public IThread
	{
	public:
		IDVBUSBDevice(DVBUSB *DVBUSB, IOUSBDeviceInterface300 **intf);
		virtual ~IDVBUSBDevice();
		
		
		static bool		MatchDevice(IOUSBDeviceInterface300 **dev) { return 0; };
		virtual bool	MatchInterface(IOUSBInterfaceInterface300 **intf) = 0;
		virtual int		IdentifyState(bool *cold) = 0;
		virtual bool	DownloadFirmware(Firmware *firmware) = 0;
		virtual char *	GetFirmware() = 0;
		virtual int		PowerControl(int onoff);
		virtual int		Initialize();
		virtual int		I2CInitialize() { return 0; };
		virtual int		AdapterInitialize();
		DVBUSB* m_DVBUSB;
		
		// USB Functionality
		int					USBOpenDevice();
		void				USBCloseDevice();
		int					USBSetupInterface();
		int					USBOpenInterface();
		void				USBCloseInterface();
		void				USBInitURB(struct urb *urb);
		struct urb *		USBAllocURB(int iso_packet);
		int					USBControlMsg(int pipeRef, __uint8_t request, __uint8_t requesttype, __uint16_t value, __uint16_t index, void *data, __uint16_t size, int timeout);
		IOReturn			USBControlRead(UInt8 *tx, UInt8 txlen, UInt8 *rx, UInt8 rxlen);
		int					USBBulkMsg(int pipeRef, void *data, int len, int timeout);
		int					USBClearHalt(int pipeRef);
		int					USBKillURB(struct urb *urb);
		IOReturn			USBSubmitURB(struct urb *urb);
		static void			URBCompleted(void *data, IOReturn result, void *io_size);

	protected:
		IOUSBDeviceInterface300**		m_USBDevice;
		IOUSBInterfaceInterface300**	m_USBInterface;
		char *							m_Firmware;
		int								m_Powered;
		IDVBI2CAdapter*					m_I2CAdapter;
		IDVBUSBAdapter*					m_USBAdapters[5];
	
	private:
		// IThread functionality
		/* virtual */ void	OnStartup();
//		/* virtual */ void	OnExit(){};
//		/* virtual */ void	OnException(){} // signal termination handler
		/* virtual */ void	Process(); 
		
		int				m_Initialized;
//		struct urb		urb_list;
//		pthread_mutex_t urb_list_mutex;
		CFRunLoopRef	m_ThreadRunLoop;
//		int URBIsRunning;
	};
};