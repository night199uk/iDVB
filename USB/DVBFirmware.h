/*
 *  DVBFirmware.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

namespace Video4Mac
{
	struct hexline {
		UInt8 len;
		UInt32 addr;
		UInt8 type;
		UInt8 data[255];
		UInt8 chk;
	};
	
	class Firmware
	{
	public:
		Firmware(char *file);
		~Firmware();
		char *GetFilename();
		int GetSize();
		int GetHexline(struct hexline *hx, int *pos);
	protected:
		int m_Size;
		char *m_Data;
		char *m_Filename;
		char *m_Location;
	};
};