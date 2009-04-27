/*
 *  DibX000.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 09/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "DibX000.h"

using namespace Video4Mac;
using namespace Dibcom;

int DibX000::WriteWord(UInt16 reg, UInt16 val)
{
	UInt8 b[4] = {
		(reg >> 8) & 0xff, reg & 0xff,
		(val >> 8) & 0xff, val & 0xff,
	};
	struct i2c_msg msg = {
		addr : m_I2CAddress, flags : 0, len : 4,  buf : b
	};
	return m_I2CAdapter->Transfer(&msg, 1) != 1 ? /* kIOReturnError */ -1 : 0;
}

#if 0
UInt16 DibX000::ReadWord(struct dibx000_i2c_master *mst, UInt16 reg)
{
	UInt8 wb[2] = { (reg >> 8) | 0x80, reg & 0xff };
	UInt8 rb[2];
	struct i2c_msg msg[2] = {
		{ .addr = mst->i2c_addr, .flags = 0,        .buf = wb, .len = 2 },
		{ .addr = mst->i2c_addr, .flags = I2C_M_RD, .buf = rb, .len = 2 },
	};
	
	if (i2c_transfer(mst->i2c_adap, msg, 2) != 2)
		printf("i2c read error on %d\\n",reg);
	
	return (rb[0] << 8) | rb[1];
}
#endif

int DibX000::I2CSelectInterface(enum dibx000_i2c_interface intf)
{
	if (m_DeviceRev > DIB3000MC && m_SelectedInterface != intf) {
		printf("selecting interface: %d\n",intf);
		m_SelectedInterface = intf;
		return WriteWord(m_BaseReg + 4, intf);
	}
	return 0;
}

int DibX000::I2CGateControl(UInt8 tx[4], UInt8 addr, int onoff)
{
	UInt16 val;
	
#if 0
	if (onoff)
		printf("opening gate for %p - on i2c_address %x\n", mst, addr);
	else
		printf("closing gate for %p\n", mst);
#endif
	
	if (onoff)
		val = addr << 8; // bit 7 = use master or not, if 0, the gate is open
	else
		val = 1 << 7;
	
	if (m_DeviceRev > DIB7000)
		val <<= 1;
	
	tx[0] = (((m_BaseReg + 1) >> 8) & 0xff);
	tx[1] = ( (m_BaseReg + 1)       & 0xff);
	tx[2] = val >> 8;
	tx[3] = val & 0xff;
	
	return 0;
}

static UInt32 dibx000_i2c_func(struct i2c_adapter *adapter)
{
//	return I2C_FUNC_I2C;
}

int DibX000::Transfer(struct i2c_msg msg[], int num)
{
	struct i2c_msg m[2 + num];
	UInt8 tx_open[4], tx_close[4];
	
	memset(m,0, sizeof(struct i2c_msg) * (2 + num));
	
	I2CSelectInterface(DIBX000_I2C_INTERFACE_TUNER);
	
	I2CGateControl(tx_open,  msg[0].addr, 1);
	m[0].addr = m_I2CAddress;
	m[0].buf  = tx_open;
	m[0].len  = 4;
	
	memcpy(&m[1], msg, sizeof(struct i2c_msg) * num);
	
	I2CGateControl(tx_close, 0, 0);
	m[num+1].addr = m_I2CAddress;
	m[num+1].buf  = tx_close;
	m[num+1].len  = 4;
	
	return m_I2CAdapter->Transfer(m, 2+num) == 2 + num ? num : -EIO;
}
 
IDVBI2CAdapter *DibX000::GetI2CAdapter(enum dibx000_i2c_interface intf, int gating)
{
	IDVBI2CAdapter *i2c = NULL;
	
	switch (intf) {
		case DIBX000_I2C_INTERFACE_TUNER:
			if (gating)
				i2c = this;
			break;
#if 0
			else
				i2c = &mst->tuner_i2c_adap;
			break;
		case DIBX000_I2C_INTERFACE_GPIO_1_2:
			if (gating)
				i2c = &mst->gated_gpio_1_2_i2c_adap;
			else
				i2c = &mst->gpio_1_2_i2c_adap;
			break;
		case DIBX000_I2C_INTERFACE_GPIO_3_4:
			if (gating)
				i2c = &mst->gated_gpio_3_4_i2c_adap;
			else
				i2c = &mst->gpio_3_4_i2c_adap;
			break;
#endif
		default:
			printf("DiBX000: incorrect I2C interface selected\n");
			break;
	}
	
	return i2c;
}

	/*
static int DibX000::I2CAdapterInit(struct i2c_adapter *i2c_adap, struct i2c_algorithm *algo, const char *name, struct dibx000_i2c_master *mst)
{


	strncpy(i2c_adap->name, name, sizeof(i2c_adap->name));
	//	i2c_adap->class     = I2C_CLASS_TV_DIGITAL,
	i2c_adap->algo      = algo;
	i2c_adap->algo_data = NULL;
	i2c_set_adapdata(i2c_adap, mst);
	if (i2c_add_adapter(i2c_adap) < 0)
		return -ENODEV;
	return 0;

 }
*/
 
int DibX000::InitI2CMaster(UInt16 device_rev, IDVBI2CAdapter *i2c_adap, UInt8 i2c_addr)
{
	UInt8 tx[4];
	struct i2c_msg m = { addr : i2c_addr >> 1, flags : 0, len : 4, buf : tx };
	
	m_DeviceRev = device_rev;
	m_I2CAdapter   = i2c_adap;
	m_I2CAddress   = i2c_addr >> 1;
	
	if (device_rev == DIB7000P)
		m_BaseReg = 1024;
	else
		m_BaseReg = 768;
	
//	if (I2CAdapterInit(&mst->gated_tuner_i2c_adap, &dibx000_i2c_gated_tuner_algo, "DiBX000 tuner I2C bus", mst) != 0)
//		printf("DiBX000: could not initialize the tuner i2c_adapter\n");
	
	/* initialize the i2c-master by closing the gate */
	I2CGateControl(tx, 0, 0);
	
	return i2c_adap->Transfer(&m, 1) == 1;
}

void DibX000::ExitI2CMaster()
{
//	i2c_del_adapter(&mst->gated_tuner_i2c_adap);
}
