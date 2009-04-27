/*
 *  Dib0070.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 10/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include "IDVBDemod.h"
#include "IDVBI2CAdapter.h"
#include "IDVBI2CClient.h"
#include "IDVBTuner.h"
#include "Dib7000P.h"

#define DEFAULT_DIB0070_I2C_ADDRESS 0x60


namespace Dibcom
{
	typedef struct {
		UInt8 i2c_address;
		
		/* tuner pins controlled externally */
//		int (*reset) (Video4Mac::IDVBDemod *, int);
//		int (*sleep) (Video4Mac::IDVBDemod *, int);
		
		/*  offset in kHz */
		int m_FreqOffsetKhzUHF;
		int m_FreqOffsetKhzVHF;
		
		UInt8 m_OscBufferState; /* 0= normal, 1= tri-state */
		UInt32 m_ClockKhz;
		UInt8 m_ClockPadDrive; /* (Drive + 1) * 2mA */
		
		UInt8 m_InvertIQ; /* invert Q - in case I or Q is inverted on the board */
		
		UInt8 m_ForceCrystalMode; /* if == 0 -> decision is made in the driver default: <24 -> 2, >=24 -> 1 */
		
		UInt8 m_FlipChip;
	} Dib0070Config;

	
	class Dib0070 : public Video4Mac::IDVBTuner, public Video4Mac::IDVBI2CClient
	{
	public:
		Dib0070();

		int			Reset();
		int			TunerAttach(Video4Mac::IDVBI2CAdapter *i2c);
		
		// IDVBTuner stuff
		int			SetParams(DVBFrontendParameters *ch);
		
	protected:
		Dib0070Config			m_Dib0070Config;
		UInt16					m_WBDFFOffset;
		UInt8					m_Revision;
		
		/* Must be protected as it can be called from overridden SetParams for Dib7070P */
		UInt16		WBDOffset();

	private:
		// Varies depending on tuner type, subclass this and override
		virtual int TunerReset(int onoff)			{ return 0;	};
		virtual int TunerSleep(int onoff)			{ return 0; };
		
		// Internal functions
		void		WBDCalibration();

		int			SetControlLo5(UInt8 vco_bias_trim, UInt8 hf_div_trim, UInt8 cp_current, UInt8 third_order_filt);
		int			SetBandwidth(DVBFrontendParameters *ch);
		void		CapTrim(UInt16 LO4);
		uint16_t	ReadReg(UInt8 reg);
		int			WriteReg(UInt8 reg, UInt16 val);

		int			Sleep();
		int			Init();
	};
};
