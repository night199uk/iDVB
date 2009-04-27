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
#include "IDVBAdapter.h"
#include <vector>

namespace Video4Mac
{
	class IDVBAdapter;
	class DVBUSB;
	class MacDVB {
	public:
		MacDVB();
		virtual ~MacDVB();
		IOReturn Initialize();
		int							RegisterAdapter(IDVBAdapter *adap, const char *name, short *adapter_nums);
		std::vector <IDVBAdapter *> GetAdapters();
		
		DVBUSB				*m_USB;
	private:
//		CFMutableArrayRef	m_Adapters;
		std::vector<IDVBAdapter *>	m_Adapters;
	};
}

