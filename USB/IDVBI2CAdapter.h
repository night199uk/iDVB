/*
 *  IDVBI2CAdapter.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

struct i2c_msg {
	__uint16_t addr;	/* slave address			*/
	__uint16_t flags;
#define I2C_M_TEN	0x10	/* we have a ten bit chip address	*/
#define I2C_M_RD	0x01
#define I2C_M_NOSTART	0x4000
#define I2C_M_REV_DIR_ADDR	0x2000
#define I2C_M_IGNORE_NAK	0x1000
#define I2C_M_NO_RD_ACK		0x0800
	__uint16_t len;		/* msg length				*/
	__uint8_t *buf;		/* pointer to msg data			*/
};

namespace Video4Mac
{
	/*
	 * I2C Message - used for pure i2c transaction, also from /dev interface
	 */

	class IDVBI2CAdapter 
	{
	public:
		virtual int		Transfer(struct i2c_msg *msg, int num) = 0;
		virtual void*	GetAdapterData() { return m_AdapterData; };
		virtual void	SetAdapterData(void *data) { m_AdapterData = data; };
	
	private:
		void *m_AdapterData;
	};
}
