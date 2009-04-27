/*
 *  IDVBFileOps.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 11/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <Video4Mac/IEventQueue.h>

namespace Video4Mac
{
	template <typename T>
	class IFile : public IEventQueue<T>
	{
	public:
		virtual int		Open() = 0;
		virtual int		Poll();
		virtual int		Read()	{ return 0; };
		virtual int		Close()	{ return 0; };
	};
};
