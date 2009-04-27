/*
 *  DVBUSB.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 01/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFArray.h>

#include "MacDVB.h"
#include "DVBUSB.h"
#include "DVBFirmware.h"
#include <unistd.h>
#include "IDVBUSBDevice.h"
#include "Dibcom.h"

#define VERBOSE 1

using namespace Video4Mac;
using namespace Dibcom;

DVBUSB::DVBUSB(MacDVB* DVB) 
: m_DVB(DVB)
{
}

DVBUSB::~DVBUSB()
{	
	m_DVB = NULL;
}



void DVBUSB::IterateUSBDevices(io_iterator_t DeviceIterator)
{
	io_object_t USBDevice;
	IOUSBDeviceInterface300 **dev=NULL;
	IOReturn err;
	IOCFPlugInInterface **plugInInterface=NULL;
	
	SInt32 score;
	HRESULT res;
	
    USBDevice = IOIteratorNext(DeviceIterator);
    if(!USBDevice)
    {
        fprintf(stderr, "Unable to find first matching USB device\n");
		return;
	}
	
	while(USBDevice)
	{

		err = IOCreatePlugInInterfaceForService(USBDevice, 
												kIOUSBDeviceUserClientTypeID, 
												kIOCFPlugInInterfaceID,
												&plugInInterface, 
												&score);
		if ((kIOReturnSuccess != err) || (plugInInterface == nil) )
		{
			fprintf(stderr, "Unable to create plug in interface for USB device - %d\n", err);
			continue;
		}
		
		res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300), (void **)&dev);
		IODestroyPlugInInterface(plugInInterface);			// done with this
		
		if (res || !dev)
		{
			fprintf(stderr, "Unable to create USB device with QueryInterface\n");
			continue;
		}
		
#if VERBOSE
		{
			UInt16 VID, PID, REL;
			err = (*dev)->GetDeviceVendor(dev, &VID);
			err = (*dev)->GetDeviceProduct(dev, &PID);
			err = (*dev)->GetDeviceReleaseNumber(dev, &REL);
			
			printf("Found device VID 0x%04X (%d), PID 0x%04X (%d), release %d\n", VID, VID, PID, PID, REL);
		}
#endif
		if (Dib0700::MatchDevice(dev))
		{
			Dib0700 *dib;
			fprintf(stderr, "Found and creating a new Dib0700 device\n");
			dib = new Dib0700(this, dev);
			dib->Initialize();
			IDVBUSBDevice* card = dib;
			CFArrayAppendValue(m_Devices, (void *) card);
		}
			
		// DeviceClose(dev);
		// Not worth bohering with errors here
		IOObjectRelease(USBDevice);
		USBDevice = IOIteratorNext(DeviceIterator);
	};
}

IOReturn DVBUSB::Initialize()
{
	// http://developer.apple.com/documentation/DeviceDrivers/Conceptual/AccessingHardware/AH_Finding_Devices/chapter_4_section_2.html#//apple_ref/doc/uid/TP30000379-TPXREF108
	
	IOReturn err;
	CFMutableDictionaryRef pDict;
	SInt32 lVendorID;
	SInt32 lProductID;
	io_iterator_t pIterator;

	m_Devices = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);

    lVendorID = atoi("4057"); // Elgato
    lProductID = atoi("17");   // EyeTV Diversity
	
    pDict = IOServiceMatching("IOUSBDevice");
	
    if(pDict == nil)
    {
        fprintf(stderr, "Could not create matching dictionary\n");
        return(-1);
    }
	
	CFDictionarySetValue(pDict, CFSTR(kUSBVendorID), 
						 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &lVendorID));
	CFDictionarySetValue(pDict, CFSTR(kUSBProductID), 
						 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &lProductID));
	
    
	err = IOServiceGetMatchingServices(kIOMasterPortDefault, pDict, &pIterator);
    if(err != kIOReturnSuccess)
    {
        // Do I need to release dict here, the call (if sucessfull??) consumes one, if so how??
        printf("Could not get device iterator: %d\n", err);
        return(-1);
    }
    
	printf("Going to Iterate Devices\n");
    IterateUSBDevices(pIterator);

	
//	CFArrayApplyFunction(m_Devices, CFRangeMake(0, CFArrayGetCount(m_Devices)), SetupDevices, this);
	IOObjectRelease(pIterator);
    return err;
	
};	

