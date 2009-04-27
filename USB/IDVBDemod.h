/*
 *  IDVBFrontend.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <Video4Mac/IDVBFrontend.h>

namespace Video4Mac
{	
	typedef struct DVBFrontendParameters DVBFrontendParameters;
	class IDVBTuner;
	class IDVBAdapter;

	class IDVBDemod 
	{
//		friend class IDVBTuner;

	public:
		
		// Tuner
		virtual void			SetFrontend(IDVBFrontend* Frontend)														{	m_Frontend = Frontend;  };
		virtual IDVBFrontend*	GetFrontend()																			{	return m_Frontend;	};
		
		// Things our frontend can do
		virtual void	Release()																						{};
		virtual void	ReleaseSEC()																					{};
		
		virtual int		Init()																							{ return 0; };
		virtual int		Sleep()																							{ return 0; };
		
		virtual int		Write(UInt8* buf, int len)																		{ return 0; };

		/* if this is set, it overrides the default swzigzag */
		virtual int		Tune(DVBFrontendParameters* params, unsigned int ModeFlags, unsigned int *Delay, kFEStatus *status)	{ return 0; };
		
		/* get frontend tuning algorithm from the module */
		virtual kFEAlgo	GetFrontendAlgo()																				{ return kFEAlgoSW; };
		
		/* these two are only used for the swzigzag code */
		virtual int		SetFrontendParams(DVBFrontendParameters* params) = 0;
		virtual int		GetTuneSettings(DVBFrontendTuneSettings* settings)												{ return -EINVAL; };

		virtual int		GetFrontend(DVBFrontendParameters* params)														{ return 0; };

		virtual int		ReadStatus(kFEStatus* status)																	{ return 0; };
		virtual int		ReadBER(UInt32* ber)																			{ return 0; };
		virtual int		ReadSignalStrength(UInt16* strength)															{ return 0; };
		virtual int		ReadSNR(UInt16* snr)																			{ return 0; };
		virtual int		ReadUNCBlocks(UInt32* ucblocks)																	{ return 0; };
		
		virtual int		DiSEqCResetOverload()																			{ return 0; };
		virtual int		DiSEqCSendMasterCommand(struct dvb_diseqc_master_cmd* cmd)										{ return 0; };
		virtual int		DiSEqCRecvSlaveReply(struct dvb_diseqc_master_cmd* cmd)											{ return 0; };
		virtual int		DiSEqCSendBurst(kFESECMiniCmd minicmd)															{ return 0; };

		virtual int		SetTone(kFESECToneMode tone)																	{ return 0; };
		virtual int		SetVoltage(kFESECVoltage voltage)																{ return 0; };
		virtual int		EnableHighLNBVoltage(long arg)																	{ return 0; };
		virtual int		DishNetworkSendLegacyCommand(unsigned long cmd)													{ return 0; };
		virtual int		I2CGateCtrl(int enable)																			{ return 0; };
		virtual int		TSBusCtrl(int acquire)																			{ return 0; };
		
		/* These callbacks are for devices that implement their own
		 * tuning algorithms, rather than a simple swzigzag
		 */
		virtual kFESearch Search(struct DVBFrontendParameters *p)														{ return kFEAlgoSearchFailed; };
		virtual int		Track(struct DVBFrontendParameters *p)															{ return 0; };
		
//		struct dvb_tuner_ops tuner_ops;
//		struct analog_demod_ops analog_ops;
		
//		int (*set_property)(struct dvb_frontend* fe, struct dtv_property* tvp);
//		int (*get_property)(struct dvb_frontend* fe, struct dtv_property* tvp);
	protected:
		

		
	private:
		IDVBFrontend*	m_Frontend;
	};
};
