/*
 *  IDVBTuner.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 10/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#pragma once

#include <Video4Mac/IDVBFrontend.h>

typedef struct {
	const char*	Name;
	
	UInt32 FrequencyMin;
	UInt32 FrequencyMax;
	UInt32 FrequencyStep;
	
	UInt32 BandwidthMin;
	UInt32 BandwidthMax;
	UInt32 BandwidthStep;
} IDVBTunerInfo;

namespace Video4Mac
{
//	typedef struct DVBFrontendParameters DVBFrontendParameters;
	class IDVBDemod;
	class IDVBFrontend;
	
	class IDVBTuner
	{
		friend class IDVBFrontend;
	public:
		IDVBTuner();
		// Demod
		virtual void			SetFrontend(IDVBFrontend* Frontend)						{	m_Frontend = Frontend;  };
		virtual IDVBFrontend*	GetFrontend()											{	return m_Frontend;	};
		
		virtual int Release()															{ return 0; };
		virtual int Init()																{ return 0; };
		virtual int Sleep()																{ return 0; };
		
		/** This is for simple PLLs - set all parameters in one go. */
		virtual int SetParams(DVBFrontendParameters *p)									{ return 0;	};
//		virtual int SetAnalogParams(struct analog_parameters *p);
		
		/** This is support for demods like the mt352 - fills out the supplied buffer with what to write. */
		virtual int CalcRegs(DVBFrontendParameters *p, UInt8 *buf, int buf_len)			{ return 0; };
		
		/** This is to allow setting tuner-specific configs */
		virtual int SetConfig(void *priv_cfg)											{ return 0; };
		
		virtual int GetFrequency(UInt32 *frequency)										{ return 0; };
		virtual int GetBandwidth(UInt32 *bandwidth)										{ return 0; };
		
#define TUNER_STATUS_LOCKED 1
#define TUNER_STATUS_STEREO 2
		virtual int GetStatus(UInt32 *status)											{ return 0; };
		virtual int GetRFStrength(UInt16 *strength)										{ return 0; };
		
		/** These are provided seperately from set_params in order to facilitate silicon
		 * tuners which require sophisticated tuning loops, controlling each parameter seperately. */
		virtual int SetFrequency(UInt32 frequency)										{ return 0; };
		virtual int SetBandwidth(UInt32 bandwidth)										{ return 0; };
		
		/*
		 * These are provided seperately from set_params in order to facilitate silicon
		 * tuners which require sophisticated tuning loops, controlling each parameter seperately.
		 */
//		virtual int SetState(enum tuner_param param, struct tuner_state *state);
//		virtual int GetState(enum tuner_param param, struct tuner_state *state);
		
	protected:
		Video4Mac::IDVBFrontend*	m_Frontend;
	
		IDVBTunerInfo m_TunerInfo;
	private:

	};
};
