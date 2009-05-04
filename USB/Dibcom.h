/*
 *  Dibcom.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 01/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include "IDVBUSBDevice.h"
#include "IDVBI2CAdapter.h"
#include "IDVBUSBAdapter.h"
#include "IDVBDemod.h"
#include "IDVBFrontend.h"
#include "IDVBI2CClient.h"
#include "IDVBTuner.h"
#include "DIB7000P.h"
#include "Dib0070.h"

using namespace Video4Mac;

#define REQUEST_I2C_READ     0x2
#define REQUEST_I2C_WRITE    0x3
#define REQUEST_POLL_RC      0x4 /* deprecated in firmware v1.20 */
#define REQUEST_JUMPRAM      0x8
#define REQUEST_SET_CLOCK    0xB
#define REQUEST_SET_GPIO     0xC
#define REQUEST_ENABLE_VIDEO 0xF
// 1 Byte: 4MSB(1 = enable streaming, 0 = disable streaming) 4LSB(Video Mode: 0 = MPEG2 188Bytes, 1 = Analog)
// 2 Byte: MPEG2 mode:  4MSB(1 = Master Mode, 0 = Slave Mode) 4LSB(Channel 1 = bit0, Channel 2 = bit1)
// 2 Byte: Analog mode: 4MSB(0 = 625 lines, 1 = 525 lines)    4LSB(     "                "           )
#define REQUEST_SET_RC       0x11
#define REQUEST_NEW_I2C_READ 0x12
#define REQUEST_NEW_I2C_WRITE 0x13
#define REQUEST_GET_VERSION  0x15

enum Dib07x0_gpios {
	GPIO0  =  0,
	GPIO1  =  2,
	GPIO2  =  3,
	GPIO3  =  4,
	GPIO4  =  5,
	GPIO5  =  6,
	GPIO6  =  8,
	GPIO7  = 10,
	GPIO8  = 11,
	GPIO9  = 14,
	GPIO10 = 15,
};

#define GPIO_IN  0
#define GPIO_OUT 1

namespace Dibcom
{
	class Dib0700 : public IDVBUSBDevice, public IDVBI2CAdapter
	{
		friend class Dib7070P;
		
	public:
		Dib0700(DVBUSB* m_DVBUSB, IOUSBDeviceInterface300 **dev);
		~Dib0700();
		
		// Functions inherited from IDVBUSBDevice
		static bool				MatchDevice(IOUSBDeviceInterface300 **dev);
		bool					MatchInterface(IOUSBInterfaceInterface300 **dev);
		int						IdentifyState(bool *cold);
		bool					DownloadFirmware(Firmware *firmware);
		char *					GetFirmware();
		int						Initialize();

		// Functions inheritied from IDVBI2CAdapter
		int						Transfer(struct i2c_msg *msg, int num);
		int						SetGPIO(enum Dib07x0_gpios gpio, UInt8 gpio_dir, UInt8 gpio_val);
	private:
		

		// Dib0700 Private functions
		int			TransferLegacy(struct i2c_msg *msg, int num);
		int			ControlWrite(UInt8 *tx, UInt8 txlen);
		int			ControlRead(UInt8 *tx, UInt8 txlen, UInt8 *rx, UInt8 rxlen);
		IOReturn	JumpRAM(UInt32 address);
		int			ControlClock(UInt32 clk_MHz, UInt8 clock_out_gp3);
		int			SetClock(UInt8 en_pll,		UInt8 pll_src, 
							 UInt8 pll_range,	UInt8 clock_gpio3, 
							 UInt16 pll_prediv, UInt16 pll_loopdiv, 
							 UInt16 free_div,	UInt16 dsuScaler);
		
		// Dib0700 State variables
		UInt8		m_ChannelState;
//		UInt16		m_MT2060_if1[2];
		UInt8		m_IsDib7000pc;
		UInt8		m_FWUseNewI2CAPI;
		UInt8		m_DisableStreamingMasterMode;
	};
	
	class Dib7070P : public IDVBFrontend, public IDVBUSBAdapter, public Dib0070, public Dib7000P
	{
	public:
		Dib7070P(Dib0700* Device, int Id);
		virtual int	StreamingControl(int onoff);
		virtual int Initialize();
		virtual int TunerSleep(int onoff);
		virtual int TunerReset(int onoff);
		/* override the default Dib0070 SetParams */
		virtual int SetParams(DVBFrontendParameters *fep);
	};
};