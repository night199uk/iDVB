/*
 *  DIB7000P.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 09/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include "IDVBUSBDevice.h"
#include "IDVBI2CAdapter.h"
#include "IDVBUSBAdapter.h"
#include "IDVBDemod.h"
#include "IDVBI2CClient.h"
#include "DibX000.h"

using namespace Video4Mac;

namespace Dibcom
{
	enum dib7000p_power_mode {
		DIB7000P_POWER_ALL = 0,
		DIB7000P_POWER_ANALOG_ADC,
		DIB7000P_POWER_INTERFACE_ONLY,
	};
	
	class Dib7000P : public Video4Mac::IDVBDemod, private Video4Mac::IDVBI2CClient
	{
	public:
		Dib7000P();

		// Demodulator specific functions
		int					I2CEnumeration(IDVBI2CAdapter *i2c_adap, int no_of_demods, UInt8 default_addr);
		int					Identify();
		void				DemodAttach(IDVBI2CAdapter *i2c_adap, UInt8 i2c_addr);
		IDVBI2CAdapter*		GetI2CMaster(enum dibx000_i2c_interface intf, int gating);	
		int					SetGPIO(UInt8 num, UInt8 dir, UInt8 val);
		
		// IDVBDemod Operations
		/* virtual */ int	SetFrontendParams(DVBFrontendParameters *fep);
		/* virtual */ int	GetFrontendParams(DVBFrontendParameters *fep);
		/* virtual */ int	GetTuneSettings(DVBFrontendTuneSettings* settings);

		/* virtual */ int	ReadStatus(kFEStatus* status);
		/* virtual */ int	ReadBER(UInt32* ber);
		/* virtual */ int	ReadSignalStrength(UInt16* strength);
		/* virtual */ int	ReadSNR(UInt16* snr);
		/* virtual */ int	ReadUNCBlocks(UInt32* ucblocks);
		
	
	protected:
		/* Must be protected since it can be called from overridden Dib7070P::SetParams() */
		int			SetWBDRef(UInt16 value);
	private:

		// Demodulator functions
		UInt16		ReadWord(UInt16 reg);
		int			WriteWord(UInt16 reg, UInt16 val);
		void		WriteTab(UInt16 *buf);
		void		ResetPLL();
		int			ResetGPIO();
		int			CfgGPIO(UInt8 num, UInt8 dir, UInt8 val);


		int			SadCalib();
		int			SetDiversityIn(int onoff);
		int			SetOutputMode(int mode);
		int			SetBandwidth(UInt32 bw);
		void		SetADCState(enum dibx000_adc_states no);
		int			SetPowerMode(enum dib7000p_power_mode mode);
		int			DemodReset();
		virtual int UpdateLNA(UInt16 agc_global)			{ return 0;	};
		virtual int AGCControl(UInt8 before)				{ return 0;	};		
		
		// Functions that only support the IDVBFrontend interfaces
		int			AutoSearchStart(DVBFrontendParameters *ch);
		int			AutoSearchIsIRQ();
		void		UpdateTimf();
		void		SpurProtect(UInt32 rf_khz, UInt32 bw);
		void		SetChannel(DVBFrontendParameters *ch, UInt8 seq);
		void		PLLClockConfig();
		void		RestartAGC();
		int			UpdateLNA();
		int			AGCStartup(DVBFrontendParameters *ch);
		int			SetAGCConfig(UInt8 band);
		int			Tune(DVBFrontendParameters *ch);
		
		
		
		
		// DIB7000P Specific Configuration
		struct {
			UInt8		m_OutputMPEG2in188Bytes;
			UInt8		m_HostbusDiversity;
			UInt8		m_TunerIsBaseband;
			//		int (*update_lna) (struct dvb_frontend *, UInt16 agc_global);
			
			UInt8		m_AGCConfigCount;
			struct		dibx000_agc_config *agc;
			struct		dibx000_bandwidth_config *bw;
			
#define DIB7000P_GPIO_DEFAULT_DIRECTIONS 0xffff
			UInt16		m_GPIODir;
#define DIB7000P_GPIO_DEFAULT_VALUES     0x0000
			UInt16		m_GPIOVal;
#define DIB7000P_GPIO_PWM_POS0(v)        ((v & 0xf) << 12)
#define DIB7000P_GPIO_PWM_POS1(v)        ((v & 0xf) << 8 )
#define DIB7000P_GPIO_PWM_POS2(v)        ((v & 0xf) << 4 )
#define DIB7000P_GPIO_PWM_POS3(v)         (v & 0xf)
#define DIB7000P_GPIO_DEFAULT_PWM_POS    0xffff
			UInt16		m_GPIOPWMPos;
			
			UInt16		m_PWMFreqDiv;
			UInt8		m_QuartzDirect;
			UInt8		m_SpurProtect;
			//		int (*agc_control) (struct dvb_frontend *, UInt8 before);
			UInt8		m_OutputMode;
			
		} cfg;

		
		
		// DIB7000P State variables
		//		struct dvb_frontend demod;
		//		struct dib7000p_config cfg;
		
		DibX000*	m_I2CMaster;
		UInt16		m_WBDRef;
		UInt8		m_CurrentBand;
		UInt32		m_CurrentBandwidth;
		struct dibx000_agc_config *m_CurrentAGC;
		UInt32		m_Timf;
		UInt8		m_DivForceOff : 1;
		UInt8		m_DivState : 1;
		UInt16		m_DivSyncWait;
		UInt8		m_AGCState;
		UInt16		m_GPIODir;
		UInt16		m_GPIOVal;
		UInt8		m_SFNWorkaroundActive :1;
	};
	
}