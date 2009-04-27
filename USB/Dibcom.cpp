/*
 *  Dibcom.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 01/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Dibcom.h"
#include "Firmware.h"
#include <CDVBLog.h>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <iomanip>

using namespace Video4Mac;
using namespace Dibcom;


IDVBUSBStreamProperties Dib0700Streaming =
{
Type : USB_BULK,
Count : 4,
Endpoint : 0x03, // Use the BULK pipe number 2 == pipeRef 3 
u : {
Bulk :
{
BufferSize : 39480
}
}
};

Dib0700::Dib0700(DVBUSB* DVBUSB, IOUSBDeviceInterface300 **dev) : IDVBUSBDevice(DVBUSB, dev)
{
	m_Firmware = "/Users/croberts/Documents/Coding/EyeTVUSB/dvb-usb-dib0700-1.20.fw";
	m_ChannelState = 0;
	//		UInt16 mt2060_if1[2];
	m_IsDib7000pc = 0;
	m_FWUseNewI2CAPI = 0;
	m_DisableStreamingMasterMode = 0;
}

Dib0700::~Dib0700()
{

}

bool Dib0700::MatchDevice(IOUSBDeviceInterface300 **dev)
{
	return true;
}

bool Dib0700::MatchInterface(IOUSBInterfaceInterface300 **intf)
{
	return true;
}

char *	Dib0700::GetFirmware()
{
	return m_Firmware;
}

int Dib0700::IdentifyState(bool *cold)
{
	IOUSBDevRequest req;
	IOReturn err;
	UInt8 b[16];
	
	req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	req.bRequest = REQUEST_GET_VERSION; // SetIoPortsConfig
	req.wValue = 0;	// 
	req.wIndex = 0; // 
	req.wLength = 16; // 32bit int
	req.pData = b;
	
	err = (*m_USBInterface)->ControlRequest(m_USBInterface, 0, &req);
	if (kIOReturnSuccess == err) 
	{
		if(req.wLenDone != 16)
		{
			CDVBLog::Log(kDVBLogFrontend, "short read error during identify state on device\n");
			err = kIOReturnUnderrun;
		}
	} else {
		CDVBLog::Log(kDVBLogFrontend, "could not identify device status: %08x\n", err);
	}
	*cold = (req.wLenDone <= 0);
	
	return kIOReturnSuccess;
}

bool Dib0700::DownloadFirmware(Firmware *firmware)
{
	
	CDVBLog::Log(kDVBLogAdapter, "downloading firmware from file '%s'\n",firmware->GetFilename());
	struct hexline hx;
	int pos = 0;
	int ret;
	IOReturn err;
	
	UInt8 buf[260];
	
	while ((ret = firmware->GetHexline(&hx, &pos)) > 0) {
		CDVBLog::Log(kDVBLogAdapter, "writing to address 0x%08x (buffer: 0x%02x %02x)\n",hx.addr, hx.len, hx.chk);
		
		buf[0] = hx.len;
		buf[1] = (hx.addr >> 8) & 0xff;
		buf[2] =  hx.addr       & 0xff;
		buf[3] = hx.type;
		memcpy(&buf[4],hx.data,hx.len);
		buf[4+hx.len] = hx.chk;
		
		err = (*m_USBInterface)->WritePipeTO(m_USBInterface, 1, buf, hx.len + 5, 1000, 1000);

		/*ret = usb_bulk_msg(intf,
						   usb_sndbulkpipe(intf, 0x01),
						   buf,
						   hx.len + 5,
						   &act_len,
						   1000);
		*/
		if (err != kIOReturnSuccess) {
			CDVBLog::Log(kDVBLogAdapter, "firmware download failed at %d with %d",pos, err);
			return false;
		}
	}
	
	if (ret == 0) {
		/* start the firmware */
		if (JumpRAM(0x70000000) == kIOReturnSuccess) {
			CDVBLog::Log(kDVBLogAdapter, "firmware started successfully.\n");
			usleep(500000);
			return true;
		}
	}
	
	return false;
};

IOReturn Dib0700::JumpRAM(UInt32 address)
{
	IOReturn err;
	UInt8 buf[8] = { REQUEST_JUMPRAM, 0, 0, 0,
		(address >> 24) & 0xff,
		(address >> 16) & 0xff,
		(address >> 8)  & 0xff,
	address        & 0xff };
	
	err = (*m_USBInterface)->WritePipeTO(m_USBInterface, 0x01, buf, 8, 1000, 1000);

	if (err != kIOReturnSuccess) {
		CDVBLog::Log(kDVBLogAdapter, "jumpram to 0x%x failed\n",address);
		return err;
	}
	
	return kIOReturnSuccess;
}

int Dib0700::Initialize()
{
	IDVBUSBDevice::Initialize();
	
	m_USBAdapters[0] = new Dib7070P(this, 0);
	m_USBAdapters[0]->Initialize();
	m_USBAdapters[0]->FrontendAttach();
	
	m_USBAdapters[1] = new Dib7070P(this, 1);
	m_USBAdapters[1]->Initialize();	
	m_USBAdapters[1]->FrontendAttach();
	return 0;
}

void debug_dump(UInt8 *buf, UInt8 len, kDVBLogTargets lt, bool in)
{
	std::ostringstream msg;

	if (in == false)
	{
		msg << ">>> ";
	} else {
		msg << "<<< ";
	}
	for (int i = 0; i < len; i++) msg << std::setfill('0') << std::hex << std::setw(2) << (int) buf[i] << ' '; 	
	msg << std::endl;
	CDVBLog::Log(kDVBLogUSB, msg.str().c_str());
}

/* expecting rx buffer: request data[0] data[1] ... data[2] */
int Dib0700::ControlWrite(UInt8 *tx, UInt8 txlen)
{
	IOUSBDevRequest req;
	IOReturn err;

	debug_dump(tx, txlen, kDVBLogUSB, false);
	req.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	req.bRequest = tx[0]; // SetIoPortsConfig
	req.wValue = 0;
	req.wIndex = 0;
	req.wLength = txlen; // 32bit int
	req.pData = tx;
	
	err = (*m_USBInterface)->ControlRequest(m_USBInterface, 0, &req);
	
	if (err != kIOReturnSuccess || req.wLenDone != txlen)
		CDVBLog::Log(kDVBLogAdapter, "ep 0 write error (err = %d, len: %d)\n",err,txlen);
	
	return err == kIOReturnSuccess ? req.wLenDone : 0;
}


/* expecting tx buffer: request data[0] ... data[n] (n <= 4) */
int Dib0700::ControlRead(UInt8 *tx, UInt8 txlen, UInt8 *rx, UInt8 rxlen)
{
	
	IOUSBDevRequest req;
	IOReturn err;

	if (txlen < 2) {
		CDVBLog::Log(kDVBLogAdapter, "tx buffer length is smaller than 2. Makes no sense.");
		return -EINVAL;
	}
	if (txlen > 4) {
		CDVBLog::Log(kDVBLogAdapter, "tx buffer length is larger than 4. Not supported.");
		return -EINVAL;
	}
	
	debug_dump(tx, txlen, kDVBLogUSB, false);
	req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	req.bRequest = tx[0]; // SetIoPortsConfig
	req.wValue = ((txlen - 2) << 8) | tx[1];
	req.wIndex = 0;
	if (txlen > 2)
		req.wIndex |= (tx[2] << 8);
	if (txlen > 3)
		req.wIndex |= tx[3];

	req.wLength = rxlen; // 32bit int
	req.pData = rx;
	
	err = (*m_USBInterface)->ControlRequest(m_USBInterface, 0, &req);

	if (err != kIOReturnSuccess)
		CDVBLog::Log(kDVBLogAdapter, "ep 0 read error (err = %d)\n", err);	
	
	debug_dump(rx, rxlen, kDVBLogUSB, true);
	
	return err == kIOReturnSuccess ? req.wLenDone : 0; /* length in case of success */
}


int Dib0700::SetGPIO(enum Dib07x0_gpios gpio, UInt8 gpio_dir, UInt8 gpio_val)
{
	UInt8 buf[3] = { REQUEST_SET_GPIO, gpio, ((gpio_dir & 0x01) << 7) | ((gpio_val & 0x01) << 6) };
	return ControlWrite(buf,3);
}

int Dib0700::SetClock(UInt8 en_pll,
					 UInt8 pll_src, UInt8 pll_range, UInt8 clock_gpio3, UInt16 pll_prediv,
					 UInt16 pll_loopdiv, UInt16 free_div, UInt16 dsuScaler)
{
	UInt8 b[10];
	b[0] = REQUEST_SET_CLOCK;
	b[1] = (en_pll << 7) | (pll_src << 6) | (pll_range << 5) | (clock_gpio3 << 4);
	b[2] = (pll_prediv >> 8)  & 0xff; // MSB
	b[3] =  pll_prediv        & 0xff; // LSB
	b[4] = (pll_loopdiv >> 8) & 0xff; // MSB
	b[5] =  pll_loopdiv       & 0xff; // LSB
	b[6] = (free_div >> 8)    & 0xff; // MSB
	b[7] =  free_div          & 0xff; // LSB
	b[8] = (dsuScaler >> 8)   & 0xff; // MSB
	b[9] =  dsuScaler         & 0xff; // LSB
	
	return ControlWrite(b, 10);
}

int Dib0700::ControlClock(UInt32 clk_MHz, UInt8 clock_out_gp3)
{
	switch (clk_MHz) {
		case 72: SetClock(1, 0, 1, clock_out_gp3, 2, 24, 0, 0x4c); break;
		default: return -EINVAL;
	}
	return 0;
}

/*
 * I2C master xfer function (pre-1.20 firmware)
 */
int Dib0700::TransferLegacy(struct i2c_msg *msg, int num)
{
	int i,len;
	UInt8 buf[255];
	
//	if (pthread_mutex_lock(&d->i2c_mutex) < 0) /* _interruptable */
//		return -EAGAIN;
	
	for (i = 0; i < num; i++) {
		/* fill in the address */
		buf[1] = (msg[i].addr << 1);
		/* fill the buffer */
		memcpy(&buf[2], msg[i].buf, msg[i].len);
		
		/* write/read request */
		if (i+1 < num && (msg[i+1].flags & I2C_M_RD)) {
			buf[0] = REQUEST_I2C_READ;
			buf[1] |= 1;
			
			/* special thing in the current firmware: when length is zero the read-failed */
			if ((len = ControlRead(buf, msg[i].len + 2, msg[i+1].buf, msg[i+1].len)) <= 0) {
				CDVBLog::Log(kDVBLogI2C, "I2C read failed on address 0x%02x\n",
						 msg[i].addr);
				break;
			}
			
			msg[i+1].len = len;
			
			i++;
		} else {
			buf[0] = REQUEST_I2C_WRITE;
			if (ControlWrite(buf, msg[i].len + 2) < 0)
				break;
		}
	}
	
//	pthread_mutex_unlock(&d->i2c_mutex);
	return i;
}


int Dib0700::Transfer(struct i2c_msg *msg, int num)
{
	if (m_FWUseNewI2CAPI == 1) {
		/* User running at least fw 1.20 */
		return 0; //dib0700_i2c_xfer_new(adap, msg, num);
	} else {
		/* Use legacy calls */
		return TransferLegacy(msg, num);
	}
}



Dib7070P::Dib7070P(Dib0700 *Device, int Id) : IDVBUSBAdapter(Device, Id)
{
	std::ostringstream Name;
	Name << "Dib0700 based adapter " << m_Id;
	m_Name = Name.str();

	////////////////////////////////////////////////////////////////////
	//
	// IDVBDemod Information Structure
	//
	////////////////////////////////////////////////////////////////////
	m_FrontendInfo.Name					= "DiBcom 7000PC";
	m_FrontendInfo.Type					= FE_OFDM;
	m_FrontendInfo.FrequencyMin			= 44250000;
	m_FrontendInfo.FrequencyMax			= 867250000;
	m_FrontendInfo.FrequencyStepSize	= 62500;
	m_FrontendInfo.FrequencyTolerance	= 0;
	m_FrontendInfo.SymbolRateMin		= 0;
	m_FrontendInfo.SymbolRateMax		= 0;
	m_FrontendInfo.SymbolRateTolerance	= 0;
	m_FrontendInfo.NotifierDelay		= 0;
	m_FrontendInfo.Capabilities			= (kFECaps)(FE_CAN_INVERSION_AUTO |
											FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
											FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
											FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
											FE_CAN_TRANSMISSION_MODE_AUTO |
											FE_CAN_GUARD_INTERVAL_AUTO |
											FE_CAN_RECOVER |
											FE_CAN_HIERARCHY_AUTO);
	
	m_TunerInfo.Name         = "DiBcom DiB0070 (Dib7070)";
	m_TunerInfo.FrequencyMin =  45000000;
	m_TunerInfo.FrequencyMax = 860000000;
	m_TunerInfo.FrequencyStep=      1000;
	
	Dib0070::SetI2CAddress(DEFAULT_DIB0070_I2C_ADDRESS);

	m_Dib0070Config.m_FreqOffsetKhzUHF = 0;
	m_Dib0070Config.m_FreqOffsetKhzVHF = 0;
	m_Dib0070Config.m_OscBufferState = 0;
	m_Dib0070Config.m_ClockKhz = 12000;
	if (Id == 0)
		m_Dib0070Config.m_ClockPadDrive = 4; /* only used for adapter id 0 */	
}

int Dib7070P::StreamingControl(int onoff)
{	int ret;
	
	Dib0700* st = (Dib0700*) m_USBDevice;
	UInt8 b[4];
	
	b[0] = REQUEST_ENABLE_VIDEO;
	b[1] = (onoff << 4) | 0x00; /* this bit gives a kind of command, rather than enabling something or not */
	
	if (st->m_DisableStreamingMasterMode == 1)
		b[2] = 0x00;
	else
		b[2] = (0x01 << 4); /* Master mode */
	
	b[3] = 0x00;
	
	fprintf(stderr, "modifying (%d) streaming state for %d\n", onoff, m_Id);
	
	if (onoff)
		st->m_ChannelState |=   1 << m_Id;
	else
		st->m_ChannelState &= ~(1 << m_Id);
	
	b[2] |= st->m_ChannelState;
	
	fprintf(stderr, "data for streaming: %x %x\n",b[1],b[2]);
	ret = st->ControlWrite(b, 4);
	if (ret)
		m_Feedcount++;
	
	return ret;
}

int Dib7070P::Initialize()
{
	Dib0700Streaming.Endpoint = 0x03 + m_Id;
	USBURBInit(&Dib0700Streaming);

	IDVBAdapter::SetFrontend(this);
	IDVBDemod::SetFrontend(this);
	IDVBTuner::SetFrontend(this);
	IDVBFrontend::SetTuner(this);
	IDVBFrontend::SetDemod(this);
	
	Dib0700 *dev = (Dib0700 *) m_USBDevice;
	
	if (m_Id == 0)
	{
		dev->SetGPIO(GPIO6, GPIO_OUT, 1);
		usleep(10000);
		dev->SetGPIO(GPIO9, GPIO_OUT, 1);
		dev->SetGPIO(GPIO4, GPIO_OUT, 1);
		dev->SetGPIO(GPIO7, GPIO_OUT, 1);
		dev->SetGPIO(GPIO10, GPIO_OUT, 0);
		dev->ControlClock(72, 1);
		usleep(10000);
		dev->SetGPIO(GPIO10, GPIO_OUT, 1);
		usleep(10000);
		dev->SetGPIO(GPIO0, GPIO_OUT, 1);
		
		if (I2CEnumeration((IDVBI2CAdapter *)dev, 2, 18) != 0){
			CDVBLog::Log(kDVBLogAdapter, "%s: Dib7000P::I2CEnumeration failed. Cannot continue\n", __func__);
			return -1;
		}
	}
	
	DemodAttach((IDVBI2CAdapter *)dev, (0x40 + m_Id) << 1);
	
	IDVBI2CAdapter *tun_i2c = GetI2CMaster(DIBX000_I2C_INTERFACE_TUNER, 1);
	if (TunerAttach(tun_i2c) == NULL)
	{
		CDVBLog::Log(kDVBLogAdapter, "Could not attach tuner.\n");
		return -1 /* -ENODEV */;
	}
	
	IDVBUSBAdapter::Initialize();
	IDVBFrontend::Initialize();
}
int Dib7070P::TunerReset(int onoff)
{
	return Dib7000P::SetGPIO(8, 0, !onoff);
}

int Dib7070P::TunerSleep(int onoff)
{
	return Dib7000P::SetGPIO(9, 0, onoff);
}

int Dib7070P::SetParams(DVBFrontendParameters *fep)
{
	UInt16 offset;
	UInt8 band = BAND_OF_FREQUENCY(fep->Frequency/1000);
	switch (band) {
		case BAND_VHF: offset = 950; break;
		case BAND_UHF:
		default: offset = 550; break;
	}
	CDVBLog::Log(kDVBLogAdapter, "WBD for DiB7000P: %d\n", offset + Dib0070::WBDOffset());
	SetWBDRef(offset + WBDOffset());
	return Dib0070::SetParams(fep);
}