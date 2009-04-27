/*
 *  DVBUSB.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 01/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

/*
 *  MacDVB.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 26/03/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

namespace Video4Mac
{

	class IDVBUSBDevice;
	
	class DVBUSB {
	public:
		DVBUSB(MacDVB* m_DVB);
		virtual ~DVBUSB();
		IOReturn Initialize();
//		void SetupDevices(void *Device, void *DVB);
		

		MacDVB*				m_DVB;

	private:
		// Device functions
		void IterateUSBDevices(io_iterator_t DeviceIterator);

		CFMutableArrayRef m_Devices;
	};
}