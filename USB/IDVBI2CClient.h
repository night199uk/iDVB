/*
 *  IDVBI2CClient.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

namespace Video4Mac
{
	class IDVBI2CAdapter;
	class IDVBI2CClient
	{
	public:
		UInt8			GetI2CAddress()						{ return m_I2CAddress; }
		void			SetI2CAddress(UInt8 Address)		{ m_I2CAddress = Address; }
	protected:
		UInt8			m_I2CAddress;
		IDVBI2CAdapter* m_I2CAdapter;
	private:
	};
};