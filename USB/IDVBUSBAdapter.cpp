/*
 *  IDVBUSBAdapter.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <IDVBUSBDevice.h>
#include <IDVBUSBAdapter.h>
#include <CDVBLog.h>
#include <CDVBUSBDemux.h>

using namespace Video4Mac;

IDVBUSBAdapter::IDVBUSBAdapter(IDVBUSBDevice *Device, int Id) : m_USBDevice(Device), m_Id(Id)
{
	CDVBLog::Log(kDVBLogAdapter, "will pass the complete MPEG2 transport stream to the software demuxer.");

	m_PIDFiltering = 0;
	m_MaxFeedCount = 255;
	m_URBsSubmitted = 0;
	m_URBsInitialized = 0;
	m_Id = 0;
	m_Feedcount = 0;
	m_State = 0;
	m_BufNum = 0;
	m_BufSize = 0;
	m_Demux = NULL;
};

IDVBUSBAdapter::~IDVBUSBAdapter()
{
};

int IDVBUSBAdapter::Initialize()
{
	m_Demux = new CDVBUSBDemux(this, m_MaxFeedCount, m_MaxFeedCount);
	m_Demux->Initialize();
	SetFilterNum(m_Demux->GetFilterNum());
	
	m_USBDevice->m_DVBUSB->m_DVB->RegisterAdapter(this, "test", NULL);
	m_State |= DVB_USB_ADAP_STATE_DVB;
}

void IDVBUSBAdapter::DataComplete(UInt8 *buffer, size_t length)
{
    if (m_Feedcount > 0 && m_State & DVB_USB_ADAP_STATE_DVB)
	{
        m_Demux->SWFilter(buffer, length);
	}
}

int IDVBUSBAdapter::AllocateStreamBuffers(int num, unsigned long size)
{
    m_BufNum = 0;
    m_BufSize = size;
	
	CDVBLog::Log(kDVBLogAdapter, "all in all I will use %lu bytes for streaming\n",num*size);

	for (m_BufNum = 0; m_BufNum < num; m_BufNum++) {
		m_BufList[m_BufNum] = (UInt8 *) malloc(size);
		if (m_BufList[m_BufNum] == NULL)
		{
			CDVBLog::Log(kDVBLogAdapter, "%s: could not allocate a stream buffer of size %d\n", size);
            return -ENOMEM;
        }
        memset(m_BufList[m_BufNum],0,size);
        m_State |= 1;
    }

    return 0;
}

	
int IDVBUSBAdapter::USBBulkURBInit()
{
    int i, j;
	
   if ((i = AllocateStreamBuffers(m_StreamProperties.Count, m_StreamProperties.u.Bulk.BufferSize)) < 0)
       return i;
	
    /* allocate the URBs */
	for (i = 0; i < m_StreamProperties.Count; i++) {
        m_URBList[i] = m_USBDevice->USBAllocURB(0);
		
        if (!m_URBList[i]) {
            printf("not enough memory for urb_alloc_urb!.\n");
            return kIOReturnNoMemory;
        }
		m_URBList[i]->pipe = m_StreamProperties.Endpoint;
		m_URBList[i]->transfer_buffer = m_BufList[i];
		m_URBList[i]->transfer_buffer_length = m_StreamProperties.u.Bulk.BufferSize;
		//		m_URBList[i]->complete = complete_fn;
		m_URBList[i]->context = this;
		m_URBList[i]->adap = this;
		m_URBList[i]->method = &Video4Mac::IDVBUSBAdapter::DataComplete;
		//        stream->urb_list[i]->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
		//        stream->urb_list[i]->transfer_dma = stream->dma_addr[i];
		m_URBsInitialized++;
	}
    return 0;
}


IOReturn IDVBUSBAdapter::USBURBInit(IDVBUSBStreamProperties *props)
{
    if (props == NULL)
        return -EINVAL;
	
    memcpy(&m_StreamProperties, props, sizeof(IDVBUSBStreamProperties));
	
	m_USBDevice->USBClearHalt(m_StreamProperties.Endpoint);
	

//    if (stream->complete == NULL) {
//        err("there is no data callback - this doesn't make sense.");
//        return kIOReturnBadArgument;
//    }
	
	
	switch (m_StreamProperties.Type) {
        case USB_BULK:
           return USBBulkURBInit();
//        case USB_ISOC:
			//            return usb_isoc_urb_init(stream);
        default:
			CDVBLog::Log(kDVBLogAdapter, "unkown URB-type for data transfer.");
            return kIOReturnBadArgument;
    }
}

int IDVBUSBAdapter::USBURBKill()
{
    int i;
    for (i = 0; i < m_URBsSubmitted; i++) {
        printf("killing URB no. %d.\n",i);
		
        /* stop the URB */
//        m_USBDevice->USBKillURB(m_URBList[i]);
    }
    m_URBsSubmitted = 0;
    return 0;
}


int IDVBUSBAdapter::USBURBSubmit()
{
    int i,ret;
	printf("going to submit %d urbs\n", m_URBsInitialized); 
    for (i = 0; i < m_URBsInitialized; i++) {
        printf("submitting URB no. %d\n",i);
        if ((ret = m_USBDevice->USBSubmitURB(m_URBList[i]))) {
            printf("could not submit URB no. %d - get them all back",i);
            USBURBKill();
            return ret;
        }
        m_URBsSubmitted++;
    }
    return 0;
}