/*
 *  IDVBUSBDevice.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 01/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBUSBDevice.h"
#include <CDVBLog.h>

using namespace Video4Mac;

IDVBUSBDevice::IDVBUSBDevice(DVBUSB *DVBUSB, IOUSBDeviceInterface300 **dev) : m_DVBUSB(DVBUSB), m_USBDevice(dev)
{
	m_USBInterface = NULL;
	m_ThreadRunLoop = NULL;
	m_Initialized = 0;
};

IDVBUSBDevice::~IDVBUSBDevice()
{
	USBCloseInterface();
	USBCloseDevice();
};

int IDVBUSBDevice::Initialize()
{
	bool Cold;
	
	USBOpenDevice();
	USBSetupInterface();
	USBOpenInterface();
	IdentifyState(&Cold);
	if (Cold)
	{
		CDVBLog::Log(kDVBLogUSB, "%s: card found in a cold state. going to download firmware.\n", __func__);
		Firmware *fw = new Firmware(GetFirmware());
		if (fw->GetSize() > 0)
		{
			if (!DownloadFirmware(fw))
			{
				CDVBLog::Log(kDVBLogUSB, "%s: failed to download firmware to device.\n", __func__);
			}
		}
		//		delete fw;
	}
	PowerControl(1);
	PowerControl(0);
}

int IDVBUSBDevice::AdapterInitialize()
{
}

int IDVBUSBDevice::USBOpenDevice()
{
	UInt8 numConfig;
	IOReturn err;
	IOUSBConfigurationDescriptorPtr desc;
	
    err = (*m_USBDevice)->GetNumberOfConfigurations(m_USBDevice, &numConfig);
    if(err != kIOReturnSuccess)
    {
		CDVBLog::Log(kDVBLogDevice, "%s: could not number of configurations from device %d\n", __func__, err);
        return(-1);
    }
	
    if(numConfig != 1)
    {
        CDVBLog::Log(kDVBLogDevice, "%s: this does not look like the right device, it has %d configurations (we want 1)\n", __func__, numConfig);
        return(-1);
    }
    
    err = (*m_USBDevice)->GetConfigurationDescriptorPtr(m_USBDevice, 0, &desc);
    if(err != kIOReturnSuccess)
    {
        CDVBLog::Log(kDVBLogDevice, "%s: could not get configuration descriptor from device %d\n", __func__, err);
        return(-1);
    }
	
#if VERBOSE
    printf("Configuration value is %d\n", desc->bConfigurationValue);
#endif
    
    // We should really try to do classic arbitration here
    
    err = (*m_USBDevice)->USBDeviceOpen(m_USBDevice);
    if(err == kIOReturnExclusiveAccess)
    {
#if VERBOSE
        CDVBLog::Log(kDVBLogDevice, "%s: exclusive error opening device, we may come back to this later\n", __func__);
#endif
        return(-2);
    }
    if(err != kIOReturnSuccess)
    {
        CDVBLog::Log(kDVBLogDevice, "%s: could not open device - %d\n", __func__, err);
        return(-1);
    }
	
	err = (*m_USBDevice)->SetConfiguration(m_USBDevice, desc->bConfigurationValue);
    if(err != kIOReturnSuccess)
    {
        CDVBLog::Log(kDVBLogDevice, "%s: could not set configuration on device - %d\n", __func__, err);
        return nil;
    }
}

void IDVBUSBDevice::USBCloseDevice()
{
	IOReturn                        err;
	
    err = (*m_USBDevice)->USBDeviceClose(m_USBDevice);
    if (err)
    {
        CDVBLog::Log(kDVBLogDevice, "%s: error closing device - %08x\n", __func__, err);
        (*m_USBDevice)->Release(m_USBDevice);
        return;
    }
    err = (*m_USBDevice)->Release(m_USBDevice);
    if (err)
    {
        CDVBLog::Log(kDVBLogDevice, "%s: error releasing device - %08x\n", __func__, err);
        return;
    }
};

int IDVBUSBDevice::USBSetupInterface()
{
	io_iterator_t					pIterator;
	IOUSBFindInterfaceRequest		req;
	IOReturn						err;
	io_service_t					usbInterface;
	IOUSBInterfaceInterface300**	intf=NULL;
	IOCFPlugInInterface**			plugInInterface=NULL;
	SInt32 score;
	HRESULT res;
	
    req.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    req.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    req.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    req.bAlternateSetting = kIOUSBFindInterfaceDontCare;
    
    err = (*m_USBDevice)->CreateInterfaceIterator(m_USBDevice, &req, &pIterator);
    if(err != kIOReturnSuccess)
    {
		CDVBLog::Log(kDVBLogUSB, "%s: could not create interface iterator - %d\n", __func__, err);
        return -ENOMEM;
    }
    
    usbInterface = IOIteratorNext(pIterator);
    if(!usbInterface)
    {
		CDVBLog::Log(kDVBLogUSB, "%s: unable to find first interface\n", __func__);
    }
    
    while(usbInterface)
    {
		err = IOCreatePlugInInterfaceForService(usbInterface, 
												kIOUSBInterfaceUserClientTypeID, 
												kIOCFPlugInInterfaceID,
												&plugInInterface, 
												&score);
		(void)IOObjectRelease(usbInterface);
		if ((kIOReturnSuccess != err) || (plugInInterface == nil) )
		{
			CDVBLog::Log(kDVBLogUSB, "%s: Unable to create plug in interface for USB interface - %d\n", __func__, err);
			return -ENODEV;
		}
		
		res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300), (void **)&intf);
		IODestroyPlugInInterface(plugInInterface);			// done with this
		
		if (res || !intf)
		{
			CDVBLog::Log(kDVBLogUSB, "%s: unable to create interface with QueryInterface %lX\n", __func__, res);
			return -ENODEV;
		}
		
        if(intf != nil && MatchInterface(intf))
        {
			m_USBInterface = intf;
			return 0;
        }

        usbInterface = IOIteratorNext(pIterator);
    }
	
    fprintf(stderr, "No interesting interfaces found\n");
    IOObjectRelease(usbInterface);
    IOObjectRelease(pIterator);
	return nil;
}

int IDVBUSBDevice::USBOpenInterface()
{
	IOReturn ret;
	
#if VERBOSE
	UInt8 n;
	int i;
	UInt8 direction;
	UInt8 number;
	UInt8 transferType;
	UInt16 maxPacketSize;
	UInt8 interval;
	static char *types[]={
        "Control",
        "Isochronous",
        "Bulk",
	"Interrupt"};
	static char *directionStr[]={
        "Out",
        "In",
	"Control"};
#endif
	
	ret = (*m_USBInterface)->USBInterfaceOpen(m_USBInterface);
    if(ret != kIOReturnSuccess)
    {
        fprintf(stderr, "Could not set configuration on device - %d\n", ret);
        return(-1);
    }
	
#if VERBOSE
    ret = (*m_USBInterface)->GetNumEndpoints(m_USBInterface, &n);
    if(ret != kIOReturnSuccess)
    {
        fprintf(stderr, "Could not get number of endpoints in interface", ret);
        return(0);
    }

    for(i = 1; i<=n; i++)
    {
        ret = (*intf)->GetPipeProperties(m_USBInterface, i, &direction, &number, &transferType, &maxPacketSize, &interval);
        if(ret != kIOReturnSuccess)
        {
            fprintf(stderr, "Endpoint %d -\n", n);
            fprintf(stderr, "Could not get endpoint properties %d\n", ret);
            return(0);
        }
        printf("Endpoint %d: %s %s %d, max packet %d, interval %d\n", i, types[transferType], directionStr[direction], number, maxPacketSize, interval);
    }
#endif
	
    return(0);
}

void IDVBUSBDevice::USBCloseInterface()
{
	IOReturn                        err;
	
    err = (*m_USBInterface)->USBInterfaceClose(m_USBInterface);
    if (err)
    {
        printf("dealWithInterface: unable to close interface. ret = %08x\n", err);
        return;
    }
    err = (*m_USBInterface)->Release(m_USBInterface);
    if (err)
    {
        printf("dealWithInterface: unable to release interface. ret = %08x\n", err);
        return;
    }
}

int IDVBUSBDevice::PowerControl(int onoff)
{
	if (onoff)
		m_Powered++;
	else
		m_Powered--;
	
	if (m_Powered == 0 || (onoff && m_Powered == 1)) { // when switching from 1 to 0 or from 0 to 1
		printf("power control: %d\n", onoff);
	}
	return 0;
}



static char *darwin_error_str (int result) {
	switch (result) {
		case kIOReturnSuccess:
			return "no error";
		case kIOReturnNotOpen:
			return "device not opened for exclusive access";
		case kIOReturnNoDevice:
			return "no connection to an IOService";
		case kIOUSBNoAsyncPortErr:
			return "no async port has been opened for interface";
		case kIOReturnExclusiveAccess:
			return "another process has device opened for exclusive access";
		case kIOUSBPipeStalled:
			return "pipe is stalled";
		case kIOReturnError:
			return "could not establish a connection to the Darwin kernel";
		case kIOUSBTransactionTimeout:
			return "transaction timed out";
		case kIOReturnBadArgument:
			return "invalid argument";
		case kIOReturnAborted:
			return "transaction aborted";
		default:
			return "unknown error";
	}
}

void IDVBUSBDevice::USBInitURB(struct urb *urb)
{
    if (urb) {
        memset(urb, 0, sizeof(struct urb));
		//        kref_init(&urb->kref);
		//        INIT_LIST_HEAD(&urb->anchor_list);
		fprintf(stderr, "setting intf to: %p\n", m_USBInterface);
		urb->device = this;
		urb->intf = m_USBInterface;
    }
}

struct urb *IDVBUSBDevice::USBAllocURB(int iso_packet)
{
    struct urb *urb;
	
    urb = (struct urb *) malloc(sizeof(struct urb));
	//				  iso_packets * sizeof(struct usb_iso_packet_descriptor),
	//				  mem_flags);
    if (!urb) {
        printf("alloc_urb: kmalloc failed\n");
        return NULL;
    }
    USBInitURB(urb);
    return urb;
}

int IDVBUSBDevice::USBControlMsg(int pipeRef, __uint8_t request, __uint8_t requesttype, __uint16_t value, __uint16_t index, void *data, __uint16_t size, int timeout)
{
	IOUSBDevRequest req;
	IOReturn err;
	
	if (requesttype & kUSBIn) {
		req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	} else {
		req.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	}
    req.bRequest = request; // SetIoPortsConfig
    req.wValue = value;	// unused
    req.wIndex = index; // unused
    req.wLength = size; // 32bit int
    req.pData = data;

    err = (*m_USBInterface)->ControlRequest(m_USBInterface, 0, &req);
    if (kIOReturnSuccess == err) 
    {
        if(req.wLenDone != size)
        {
			printf("short write on device\n");
            err = kIOReturnUnderrun;
        }
    } else {
		printf("control message failed with err: %08x - %s\n", err, darwin_error_str(err));
	}
    return req.wLenDone;
}

IOReturn IDVBUSBDevice::USBControlRead(UInt8 *tx, UInt8 txlen, UInt8 *rx, UInt8 rxlen)
{
	IOUSBDevRequest req;
	IOReturn err;
	
	if (txlen < 2) {
		printf("tx buffer length is smaller than 2. Makes no sense.\n");
		return kIOReturnError;
	}
	
	if (txlen > 4) {
		printf("tx buffer length is larger than 4. Not supported.\n");
		return kIOReturnError;
	}
	
    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
    req.bRequest = tx[0]; // From Linux
    req.wValue = ((txlen - 2) << 8) | tx[1];	// from Linux?
    req.wIndex = 0; // unused
	if (txlen > 2)
		req.wIndex |= (tx[2] << 8);
	if (txlen > 3)
		req.wIndex |= tx[3];
	
    req.wLength = rxlen; // 32bit int
    req.pData = rx;
    
    err = (*m_USBInterface)->ControlRequest(m_USBInterface, 0, &req);
    if (kIOReturnSuccess == err && req.wLenDone != rxlen) 
    {
		err = kIOReturnUnderrun;
    }

    return err;
}

int IDVBUSBDevice::USBBulkMsg(int pipeRef, void *data, int len, int timeout)
{
	IOReturn result = -1;
	
	u_int8_t  transferType;
	u_int8_t  direction, number, interval;
	u_int16_t maxPacketSize;
	
	if (!m_USBInterface)
		printf("usb_bulk_transfer: Called with NULL device" );
	
	if (kIOReturnSuccess != result)
		printf("usb_bulk_transfer: Invalid pipe reference" );
	
	(*m_USBInterface)->GetPipeProperties(m_USBInterface, pipeRef, &direction, &number,
										 &transferType, &maxPacketSize, &interval);
	
	/* Bulk transfer */
	if (transferType == kUSBInterrupt)
		fprintf (stderr, "libusb/darwin.c usb_bulk_transfer: USB pipe is an interrupt pipe. Timeouts will not be used.\n");
	
	if ( transferType != kUSBInterrupt)
		result = (*m_USBInterface)->WritePipeTO(m_USBInterface, 1, data, len, 1000, 1000);
	else
		result = (*m_USBInterface)->WritePipe(m_USBInterface, 1, data, len);
	
	/* Check the return code of both the write and completion functions. */
	if (result != kIOReturnSuccess) {
		printf("%08x -> return code\n", darwin_error_str(result));
	}
	
	return kIOReturnSuccess;
}


int IDVBUSBDevice::USBClearHalt(int pipeRef)
{
	IOReturn result;
	
	if (!m_USBInterface)
		printf("usb_clear_halt: invalid usb_device specified.\n");
	
	result = (*m_USBInterface)->ClearPipeStall(m_USBInterface, pipeRef);
	
	if (result != kIOReturnSuccess)
		printf("usb_clear_halt(ClearPipeStall): %s", darwin_error_str(result));
	
	return kIOReturnSuccess;
}

void IDVBUSBDevice::URBCompleted(void *data, IOReturn result, void *io_size)
{
	struct urb *urb = (struct urb *)data;
	int i;
	UInt8 *b;
	IOReturn ret;
	IOUSBInterfaceInterface300 **intf;
	
	urb->status = result;
	urb->actual_length = (UInt32) io_size;
	
	switch (urb->status) {
		case kIOReturnSuccess:         /* success */
			break;
		case kIOReturnTimeout:    /* NAK */
			printf("urb completition error %d.\n", urb->status);
			break;
		case kIOReturnNotOpen:   /* Connection reset? */
		case kIOReturnNotFound: /* ENOENT */
		case kIOReturnOffline: /* ENOSHUTDOWN */
			printf("urb completition error %d.\n", urb->status);
			return;
		default:        /* error */
			printf("urb completition error %d.\n", urb->status);
			break;
	}
	
	
	i = urb->pipe;
	intf = urb->intf;
	//	ret = ep_to_pipeRef(intf, urb->pipe, &i);
	
	b = (UInt8 *) urb->transfer_buffer;
	//    switch (ptype) {
	/*        case kUSBIsoc:
	 for (i = 0; i < urb->number_of_packets; i++) {
	 
	 if (urb->iso_frame_desc[i].status != 0)
	 printf("iso frame descriptor has an error: %d\n",urb->iso_frame_desc[i].status);
	 else if (urb->iso_frame_desc[i].actual_length > 0)
	 stream->complete(stream, b + urb->iso_frame_desc[i].offset, urb->iso_frame_desc[i].actual_length);
	 
	 urb->iso_frame_desc[i].status = 0;
	 urb->iso_frame_desc[i].actual_length = 0;
	 }
	 debug_dump(b,20,deb_uxfer);
	 break;*/
	//      case PIPE_BULK:
	if (urb->actual_length > 0)
		(urb->adap->*(urb->method))(b, urb->actual_length);
	//                stream->complete(stream, b, urb->actual_length);
	//								 break;
	//    default:
	//        err("unkown endpoint type in completition handler.");
	//         return;
	//  }
	
	
	//	(urb->adap->*(urb->method))((UInt8 *)urb->transfer_buffer, urb->actual_length);
	
	urb->device->USBSubmitURB(urb);
	
	//	urb->complete(urb);
}

int IDVBUSBDevice::USBKillURB(struct urb *urb)
{
	return 0;
}

IOReturn IDVBUSBDevice::USBSubmitURB(struct urb *urb)
{
	urb->status = kIOReturnBusy;
	
	if (!m_ThreadRunLoop)
	{
		Create();
		do {
			usleep(30);
		} while (!m_ThreadRunLoop);
	}
	
	if (urb->intf)
	{
		
		IOReturn ret;
		CFRunLoopSourceRef source;
		
		IOUSBInterfaceInterface300 **intf = urb->intf;
		
		
		source = (*urb->intf)->GetInterfaceAsyncEventSource(urb->intf);
		if (source == NULL)
		{
			ret = (*urb->intf)->CreateInterfaceAsyncEventSource(urb->intf, &source);
			if (ret != kIOReturnSuccess)
			{
				printf("could not create async interface event source.\n");
				return ret;
			}
		}
		
		if (!CFRunLoopContainsSource(m_ThreadRunLoop, source, kCFRunLoopDefaultMode))
		{
			CFRunLoopAddSource(m_ThreadRunLoop, source, kCFRunLoopDefaultMode);
		}
		
		IOReturn result;
		
		u_int8_t  transferType;
		u_int8_t  direction, number, interval;
		u_int16_t maxPacketSize;
		
		
		(*intf)->GetPipeProperties(intf, urb->pipe, &direction, &number,
								   &transferType, &maxPacketSize, &interval);
		
		
		if (direction == 0)
		{
			result = (*intf)->WritePipeAsync(intf, 
											 urb->pipe, 
											 urb->transfer_buffer, 
											 urb->transfer_buffer_length, 
											 URBCompleted,
											 //											 (IOAsyncCallback1)urb_completed,
											 (void *)urb);
			
		}
		else if (direction == 1)
		{
			result = (*intf)->ReadPipeAsync(intf, 
											urb->pipe, 
											urb->transfer_buffer, 
											urb->transfer_buffer_length,
											(IOAsyncCallback1)URBCompleted,
											//											(IOAsyncCallback1)urb_completed,
											(void *)urb);
		}
		else if (direction == 2)
			printf("control urb not handled yet.\n");
		
		//		printf("urb submitted.\n");
		
	}
	
	return kIOReturnSuccess;
}

void IDVBUSBDevice::OnStartup()
{
	m_ThreadRunLoop = CFRunLoopGetCurrent();
};

void IDVBUSBDevice::Process()
{
	do {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10, false);
	}
	while (!m_bStop);
}


