/*
 *  CDVBLog.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 12/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

namespace Video4Mac
{
	typedef enum kDVBLogTargets
	{
		kDVBLogFrontend = (1 << 0),
		kDVBLogTuner = (1 << 1),
		kDVBLogDemod = (1 << 2),
		kDVBLogUSB = (1 << 3),
		kDVBLogI2C = (1 << 4),
		kDVBLogAdapter = (1 << 5),
		kDVBLogDemux = (1 << 6),
		kDVBLogDevice = (1 << 7),
	} kDVBLogTargets;
		
	class CDVBLog
	{
	public:
		static kDVBLogTargets LogTargets;
		static void Log(kDVBLogTargets LogLevel, const char *format, ... );
		

	private:
		static void Initialize(void);

	};
};