/*
 *  MacDVB.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 26/03/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFArray.h>

#include "MacDVB.h"
#include "DVBUSB.h"

//#include "dib0700.h"
#include <unistd.h>

using namespace Video4Mac;

MacDVB::MacDVB()
{
	
}

MacDVB::~MacDVB()
{
	
}



IOReturn MacDVB::Initialize()
{

//	m_Adapters = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);

	m_USB = new DVBUSB(this);
	
	m_USB->Initialize();
	
};	

int MacDVB::RegisterAdapter(IDVBAdapter *adap, const char *name, short *adapter_nums)
{
	fprintf(stderr, "DVB: registering new adapter (%s)\n", name);
	m_Adapters.push_back(adap);
}

std::vector <IDVBAdapter *> MacDVB::GetAdapters()
{
	return m_Adapters;
}


/*	CFBundleRef mainBundle;
	CFStringRef bundleIdentifier;
	CFURLRef pluginsURL;
	CFArrayRef plugins;

	SInt32 err;
	CFIndex files;
	mainBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.video4mac.Video4Mac"));
	bundleIdentifier = CFBundleGetIdentifier(mainBundle);
	pluginsURL = CFBundleCopyBuiltInPlugInsURL(mainBundle);

	plugins = (CFArrayRef) CFURLCreatePropertyFromResource(kCFAllocatorDefault, pluginsURL, kCFURLFileDirectoryContents, &err);
	
	if (!plugins)
	{
		printf("no plugins directory found or not a directory.\n");
	} else if (CFArrayGetCount(plugins) == 0)
	{
		printf("directory is empty.\n");
	} else {
		printf("we have some possible plugins to go lookat.\n");
	}
	// Here we probe for all card chipsets and start the USB Drivers, etc.
	CFShow(mainBundle);
	CFShow(bundleIdentifier);
	CFShow(pluginsURL);*/
