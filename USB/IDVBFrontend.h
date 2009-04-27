/*
 *  IDVBFrontend.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 11/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <mach/semaphore.h>
#include <Video4Mac/IThread.h>
//#include <Video4Mac/IEventQueue.h>
#include <Video4Mac/IDVBEvent.h>
#include <pthread.h>
#include <algorithm>
#include <poll.h>

namespace Video4Mac
{
	
	typedef enum kFEType {
		FE_QPSK,
		FE_QAM,
		FE_OFDM,
		FE_ATSC
	} kFEType;
	
	
	typedef enum kFECaps {
		FE_IS_STUPID			= 0,
		FE_CAN_INVERSION_AUTO		= 0x1,
		FE_CAN_FEC_1_2			= 0x2,
		FE_CAN_FEC_2_3			= 0x4,
		FE_CAN_FEC_3_4			= 0x8,
		FE_CAN_FEC_4_5			= 0x10,
		FE_CAN_FEC_5_6			= 0x20,
		FE_CAN_FEC_6_7			= 0x40,
		FE_CAN_FEC_7_8			= 0x80,
		FE_CAN_FEC_8_9			= 0x100,
		FE_CAN_FEC_AUTO			= 0x200,
		FE_CAN_QPSK			= 0x400,
		FE_CAN_QAM_16			= 0x800,
		FE_CAN_QAM_32			= 0x1000,
		FE_CAN_QAM_64			= 0x2000,
		FE_CAN_QAM_128			= 0x4000,
		FE_CAN_QAM_256			= 0x8000,
		FE_CAN_QAM_AUTO			= 0x10000,
		FE_CAN_TRANSMISSION_MODE_AUTO	= 0x20000,
		FE_CAN_BANDWIDTH_AUTO		= 0x40000,
		FE_CAN_GUARD_INTERVAL_AUTO	= 0x80000,
		FE_CAN_HIERARCHY_AUTO		= 0x100000,
		FE_CAN_8VSB			= 0x200000,
		FE_CAN_16VSB			= 0x400000,
		FE_HAS_EXTENDED_CAPS		= 0x800000,   /* We need more bitspace for newer APIs, indicate this. */
		FE_CAN_2G_MODULATION		= 0x10000000, /* frontend supports "2nd generation modulation" (DVB-S2) */
		FE_NEEDS_BENDING		= 0x20000000, /* not supported anymore, don't use (frontend requires frequency bending) */
		FE_CAN_RECOVER			= 0x40000000, /* frontend can recover from a cable unplug automatically */
		FE_CAN_MUTE_TS			= 0x80000000  /* frontend can stop spurious TS data output */
	} kFECaps;
	
	typedef struct DVBFrontendInfo {
		char*		Name;
		kFEType		Type;
		UInt32      FrequencyMin;
		UInt32      FrequencyMax;
		UInt32      FrequencyStepSize;
		UInt32      FrequencyTolerance;
		UInt32      SymbolRateMin;
		UInt32      SymbolRateMax;
		UInt32      SymbolRateTolerance;	/* ppm */
		UInt32      NotifierDelay;		/* DEPRECATED */
		kFECaps		Capabilities;
	} DVBFrontendInfo;
	
	
	/**
	 *  Check out the DiSEqC bus spec available on http://www.eutelsat.org/ for
	 *  the meaning of this struct...
	 */
	struct dvb_diseqc_master_cmd {
		__uint8_t msg [6];	/*  { framing, address, command, data [3] } */
		__uint8_t msg_len;	/*  valid values are 3...6  */
	};
	
	
	struct dvb_diseqc_slave_reply {
		__uint8_t msg [4];	/*  { framing, data [3] } */
		__uint8_t msg_len;	/*  valid values are 0...4, 0 means no msg  */
		int  timeout;	/*  return from ioctl after timeout ms with */
	};			/*  errorcode when no message was received  */
	
	
	typedef enum kFESECVoltage {
		SEC_VOLTAGE_13,
		SEC_VOLTAGE_18,
		SEC_VOLTAGE_OFF
	} kFESECVoltage;
	
	
	typedef enum kFESECToneMode {
		SEC_TONE_ON,
		SEC_TONE_OFF
	} kFESECToneMode;
	
	
	typedef enum kFESECMiniCmd {
		SEC_MINI_A,
		SEC_MINI_B
	} kFESECMiniCmd;
	
	
	typedef enum kFEStatus {
		FE_HAS_SIGNAL	= 0x01,   /* found something above the noise level */
		FE_HAS_CARRIER	= 0x02,   /* found a DVB signal  */
		FE_HAS_VITERBI	= 0x04,   /* FEC is stable  */
		FE_HAS_SYNC	= 0x08,   /* found sync bytes  */
		FE_HAS_LOCK	= 0x10,   /* everything's working... */
		FE_TIMEDOUT	= 0x20,   /* no lock within the last ~2 seconds */
		FE_REINIT	= 0x40    /* frontend was reinitialized,  */
	} kFEStatus;			  /* application is recommended to reset */
	/* DiSEqC, tone and parameters */
	
	typedef enum kFESpectralInversion {
		INVERSION_OFF,
		INVERSION_ON,
		INVERSION_AUTO
	} kFESpectralInversion;
	
	
	typedef enum kFECodeRate {
		FEC_NONE = 0,
		FEC_1_2,
		FEC_2_3,
		FEC_3_4,
		FEC_4_5,
		FEC_5_6,
		FEC_6_7,
		FEC_7_8,
		FEC_8_9,
		FEC_AUTO,
		FEC_3_5,
		FEC_9_10,
	} kFECodeRate;
	
	
	typedef enum kFEModulation {
		QPSK,
		QAM_16,
		QAM_32,
		QAM_64,
		QAM_128,
		QAM_256,
		QAM_AUTO,
		VSB_8,
		VSB_16,
		PSK_8,
		APSK_16,
		APSK_32,
		DQPSK,
	} kFEModulation;
	
	typedef enum kFETransmitMode {
		TRANSMISSION_MODE_2K,
		TRANSMISSION_MODE_8K,
		TRANSMISSION_MODE_AUTO
	} kFETransmitMode;
	
	typedef enum kFEBandwidth {
		BANDWIDTH_8_MHZ,
		BANDWIDTH_7_MHZ,
		BANDWIDTH_6_MHZ,
		BANDWIDTH_AUTO
	} kFEBandwidth;
	
	
	typedef enum kFEGuardInterval {
		GUARD_INTERVAL_1_32,
		GUARD_INTERVAL_1_16,
		GUARD_INTERVAL_1_8,
		GUARD_INTERVAL_1_4,
		GUARD_INTERVAL_AUTO
	} kFEGuardInterval;
	
	
	typedef enum kFEHierarchy {
		HIERARCHY_NONE,
		HIERARCHY_1,
		HIERARCHY_2,
		HIERARCHY_4,
		HIERARCHY_AUTO
	} kFEHierarchy;
	
#define kFETuneModeOneShot 0x01	
	
	typedef enum kFEAlgo {
		kFEAlgoHW           = (1 <<  0),
		kFEAlgoSW           = (1 <<  1),
		kFEAlgoCustom       = (1 <<  2),
		kFEAlgoRecovery     = (1 << 31)
	} kFEAlgo;
	
	typedef enum kFESearch {
		kFEAlgoSearchSuccess   = (1 <<  0),
		kFEAlgoSearchAsleep    = (1 <<  1),
		kFEAlgoSearchFailed    = (1 <<  2),
		kFEAlgoSearchInvalid   = (1 <<  3),
		kFEAlgoSearchAgain     = (1 <<  4),
		kFEAlgoSearchError     = (1 << 31),
	} kFESearch;
	
	struct DVBQPSKParameters {
		UInt32			SymbolRate;  /* symbol rate in Symbols per second */
		kFECodeRate		FECInner;    /* forward error correction (see above) */
	};
	
	struct DVBQAMParameters {
		UInt32			SymbolRate; /* symbol rate in Symbols per second */
		kFECodeRate		FECInner;   /* forward error correction (see above) */
		kFEModulation	Modulation;  /* modulation type (see above) */
	};
	
	struct DVBVSBParameters {
		kFEModulation	modulation;  /* modulation type (see above) */
	};
	
	struct DVBOFDMParameters {
		kFEBandwidth		Bandwidth;
		kFECodeRate			CodeRateHP;		/* high priority stream code rate */
		kFECodeRate			CodeRateLP;		/* low priority stream code rate */
		kFEModulation		Constellation;	/* modulation type (see above) */
		kFETransmitMode		TransmissionMode;
		kFEGuardInterval	GuardInterval;
		kFEHierarchy		HierarchyInformation;
	};
	
	typedef struct DVBFrontendParameters {
		UInt32					Frequency;		/* (absolute) frequency in Hz for QAM/OFDM/ATSC */
		/* intermediate frequency in kHz for QPSK */
		kFESpectralInversion	Inversion;
		union {
			struct DVBQPSKParameters	QPSK;
			struct DVBQAMParameters		QAM;
			struct DVBOFDMParameters	OFDM;
			struct DVBVSBParameters		VSB;
		} u;
	} DVBFrontendParameters;
	
	typedef struct DVBFrontendEvent {
		kFEStatus status;
		DVBFrontendParameters parameters;
	} DVBFrontendEvent;
	
	typedef struct DVBFrontendTuneSettings {
		int MinDelayms;
		int StepSize;
		int MaxDrift;
		struct DVBFrontendParameters parameters;
	} DVBFrontendTuneSettings;
	
	class IDVBTuner;
	class IDVBDemod;
	
	
	class IDVBFrontend : public IThread, public IDVBEventSource // , public IEventQueue<DVBFrontendEvent>,
	{
	public:
		IDVBFrontend();
		// File Style Operations

		virtual int		Initialize();
		
		unsigned int	Poll(IDVBCondition *Condition);
		virtual int		GetInfo(DVBFrontendInfo *info);
		int				SetFrontend(DVBFrontendParameters *params);
		int				SetVoltage(kFESECVoltage params);
		int				SetTone(kFESECToneMode params);
		int				GetEvent(DVBFrontendEvent *event, int flags);
		
		void			SetTuner(IDVBTuner *tuner)			{ m_Tuner = tuner; };
		IDVBTuner*		GetTuner()							{ return m_Tuner;  };
		
		void			SetDemod(IDVBDemod *Demod)			{ m_Demod = Demod; };
		IDVBDemod*		GetDemod()							{ return m_Demod;  };
		
	protected:
		DVBFrontendInfo				m_FrontendInfo;
	private:
		
		IDVBDemod*		m_Demod;
		IDVBTuner*		m_Tuner;
		
		// IThread funcs
		void			OnStartup();
		void			Process();
		
		
		// IFrontend calls
		bool			IsExiting();
		bool			ShouldWakeup();
		void			AddEvent(kFEStatus status);	
		void			Wakeup();
		
		// Tuning algorithm
		void			SWZigZag();
		void			SWZigZagUpdateDelay(int locked);
		int				SWZigZagAutoTune(int check_wrapped);
		
		// Utilities
		void			GetFrequencyLimits(UInt32 *freq_min, UInt32 *freq_max);
		int				CheckParameters(DVBFrontendParameters *parms);

		// Tuning state variables
		IDVBWaitQueue	m_Wait;
		IDVBSemaphore	m_Sem;
		DVBFrontendParameters m_Parameters;
		unsigned long	m_TuneModeFlags;
		int				m_Tone;
		int				m_Voltage;
		kFEStatus		m_Status;
		unsigned int	m_State;
		unsigned int	m_Delay;	
		int				m_Quality;
		unsigned int	m_CheckWrapped;
		bool			m_Wakeup;
		unsigned int	m_Reinitialize;
		kFESearch		m_AlgoStatus;
		int				m_LNBDrift;
		unsigned int	m_Inversion;
		unsigned int	m_AutoStep;
		unsigned int	m_AutoSubStep;
		unsigned int	m_StartedAutoStep;
		double			m_MinDelay;
		unsigned int	m_MaxDrift;
		unsigned int	m_StepSize;
		
		// Events API
		IDVBMutex		m_EventMutex;
		IDVBWaitQueue	m_EventWait;
		DVBFrontendEvent*m_Events;
		int				m_EventW;
		int				m_EventR;
		int				m_EventSize;
		int				m_EventOverflow;
		
		
//		IEventQueue<DVBFrontendEvent>	m_Events;
	};
};