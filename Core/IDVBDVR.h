/*
 *  IDVBDVR.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

namespace Video4Mac
{
	class IDVBDmxDev;
	
	class IDVBDVR
	{
	public:
		int				Open();
		ssize_t			Read(char /*__user*/ *buf, size_t count, long long *ppos);
	private:
		IDVBDmxDev*			m_DmxDev;
		
		// These should all be generic to DVB_Device
		unsigned int		m_Readers;
		unsigned int		m_Writers;
		unsigned int		m_Users;
	};
};
