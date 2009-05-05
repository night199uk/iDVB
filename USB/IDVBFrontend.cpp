/*
 *  IDVBFrontend.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 11/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBFrontend.h"
#include "IDVBDemod.h"
#include "IDVBTuner.h"
#include "CDVBLog.h"
#include <iostream>
#include <sstream>
#include <mach/semaphore.h>
#include <mach/mach.h>
#include <fcntl.h>

using namespace Video4Mac;

#define FESTATE_IDLE 1
#define FESTATE_RETUNE 2
#define FESTATE_TUNING_FAST 4
#define FESTATE_TUNING_SLOW 8
#define FESTATE_TUNED 16
#define FESTATE_ZIGZAG_FAST 32
#define FESTATE_ZIGZAG_SLOW 64
#define FESTATE_DISEQC 128
#define FESTATE_WAITFORLOCK (FESTATE_TUNING_FAST | FESTATE_TUNING_SLOW | FESTATE_ZIGZAG_FAST | FESTATE_ZIGZAG_SLOW | FESTATE_DISEQC)
#define FESTATE_SEARCHING_FAST (FESTATE_TUNING_FAST | FESTATE_ZIGZAG_FAST)
#define FESTATE_SEARCHING_SLOW (FESTATE_TUNING_SLOW | FESTATE_ZIGZAG_SLOW)
#define FESTATE_LOSTLOCK (FESTATE_ZIGZAG_FAST | FESTATE_ZIGZAG_SLOW)

IDVBFrontend::IDVBFrontend()
{
	memset(&m_Parameters, 0, sizeof(DVBFrontendParameters));
	m_TuneModeFlags = 0;
	m_Tone = 0;
	m_Voltage = 0;
	m_Status = (kFEStatus) 0;
	m_State = 0;
	m_Delay = 0;	
	m_Quality = 0;
	m_CheckWrapped = 0;
	m_Wakeup = 0;
	m_Reinitialize = 0;
	m_AlgoStatus = (kFESearch) 0;
	m_LNBDrift = 0;
	m_Inversion = 0;
	m_AutoStep = 0;
	m_AutoSubStep = 0;
	m_StartedAutoStep = 0;
	m_MinDelay = 0;
	m_MaxDrift = 0;
	m_StepSize = 0;
	m_EventSize = 8;
	m_Events = new DVBFrontendEvent[8];
	m_EventR = 0;
	m_EventW = 0;
}

////////////////////////////////////////////////////
//
// File Operations
//
////////////////////////////////////////////////////
int	IDVBFrontend::Initialize()
{
	m_Sem.Down();
	m_TuneModeFlags &= ~kFETuneModeOneShot;
	m_Tone = -1;
	m_Voltage = -1;
	
	Create();
	
	
	return 1;
}

bool IDVBFrontend::IsExiting()
{
    if (m_bStop)
        return true;
	
    return false;
}

bool IDVBFrontend::ShouldWakeup()
{
	if (m_Wakeup)
	{
		m_Wakeup = false;
		return true;
	}
	return IsExiting();
}

void IDVBFrontend::Wakeup()
{
    m_Wakeup = true;
	m_Wait.SignalAll();
}


void IDVBFrontend::AddEvent(kFEStatus status)
{
    DVBFrontendEvent* e;
	int wp;
	
	m_EventMutex.Lock();
	
    wp = (m_EventW + 1) % m_EventSize;
	
    if (wp == m_EventR) {
        m_EventOverflow = 1;
        m_EventR = (m_EventR + 1) % m_EventSize;
    }
	
    e = &m_Events[m_EventW];
	
    memcpy (&e->parameters, &m_Parameters, sizeof(DVBFrontendParameters));
	
    if (status & FE_HAS_LOCK)
	{
		m_Demod->GetFrontend(&e->parameters);
	}
	
    m_EventW = wp;
    e->status = status;
	m_EventMutex.Unlock();
	m_EventWait.SignalAll();
}

int IDVBFrontend::GetEvent(DVBFrontendEvent *event, int flags)
{
	if (m_EventOverflow) {
		m_EventOverflow = 0;
		return -EOVERFLOW;
	}
	
	if (m_EventW == m_EventR) {
		int ret;
		
		if (flags & O_NONBLOCK)
			return -EWOULDBLOCK;

		IDVBCondition Cond;
		Poll(&Cond);
		Cond.Wait();
		
		if (ret < 0)
			return ret;
	}
	m_EventMutex.Lock();
	
	memcpy (event, &m_Events[m_EventR],	sizeof(DVBFrontendEvent));
	m_EventR = (m_EventR + 1) % m_EventSize;

	m_EventMutex.Unlock();
	
	return 0;
};

unsigned int IDVBFrontend::Poll(IDVBCondition *Condition)
{
	m_EventWait.Add(Condition);
	
	if (m_EventW != m_EventR)
		return (POLLIN | POLLRDNORM | POLLPRI);
	
	return 0;
}

int IDVBFrontend::CheckParameters(DVBFrontendParameters *parms)
{
    UInt32 freq_min;
    UInt32 freq_max;
	
    /* range check: frequency */
    GetFrequencyLimits(&freq_min, &freq_max);
    if ((freq_min && parms->Frequency < freq_min) ||
        (freq_max && parms->Frequency > freq_max)) {
        CDVBLog::Log(kDVBLogFrontend,  "DVB: frequency %u out of range (%u..%u)\n",
						parms->Frequency, freq_min, freq_max);
        return -EINVAL;
    }
	
    /* range check: symbol rate */
    if (m_FrontendInfo.Type == FE_QPSK) {
        if ((m_FrontendInfo.SymbolRateMin &&
             parms->u.QPSK.SymbolRate < m_FrontendInfo.SymbolRateMin) ||
            (m_FrontendInfo.SymbolRateMax &&
             parms->u.QPSK.SymbolRate > m_FrontendInfo.SymbolRateMax)) {
            CDVBLog::Log(kDVBLogFrontend,  "DVB: symbol rate %u out of range (%u..%u)\n",
                  parms->u.QPSK.SymbolRate,
                   m_FrontendInfo.SymbolRateMin, m_FrontendInfo.SymbolRateMax);
            return -EINVAL;
        }
		
    } else if (m_FrontendInfo.Type == FE_QAM) {
        if ((m_FrontendInfo.SymbolRateMin &&
             parms->u.QAM.SymbolRate < m_FrontendInfo.SymbolRateMin) ||
            (m_FrontendInfo.SymbolRateMax &&
             parms->u.QAM.SymbolRate > m_FrontendInfo.SymbolRateMax)) {
            printf("DVB: symbol rate %u out of range (%u..%u)\n",
                   parms->u.QAM.SymbolRate,
                   m_FrontendInfo.SymbolRateMin, m_FrontendInfo.SymbolRateMax);
            return -EINVAL;
        }
    }
	
    return 0;
}


int IDVBFrontend::SetFrontend(DVBFrontendParameters *params)
{
	struct DVBFrontendTuneSettings FETuneSettings;
	m_Sem.Down();
	
	if (CheckParameters(params) < 0) {
		return -EINVAL;
	}
		
	memcpy (&m_Parameters, params, sizeof (DVBFrontendParameters));
	
	memset(&FETuneSettings, 0, sizeof(DVBFrontendTuneSettings));
	memcpy(&FETuneSettings.parameters, params, sizeof (DVBFrontendParameters));
	
	/* force auto frequency inversion if requested */
	//	if (dvb_force_auto_inversion) {
	m_Parameters.Inversion = INVERSION_AUTO;
	FETuneSettings.parameters.Inversion = INVERSION_AUTO;
	//	}
	if (m_FrontendInfo.Type == FE_OFDM) {
		/* without hierarchical coding code_rate_LP is irrelevant,
		 * so we tolerate the otherwise invalid FEC_NONE setting */
		if (m_Parameters.u.OFDM.HierarchyInformation == HIERARCHY_NONE &&
			m_Parameters.u.OFDM.CodeRateLP == FEC_NONE)
			m_Parameters.u.OFDM.CodeRateLP = FEC_AUTO;
	}
	
	/* get frontend-specific tuning settings */
	if (m_Demod->GetTuneSettings(&FETuneSettings) == 0) {
		m_MinDelay = FETuneSettings.MinDelayms;
		m_MaxDrift = FETuneSettings.MaxDrift;
		m_StepSize = FETuneSettings.StepSize;
	} else {
		
		/* default values */
		switch(m_FrontendInfo.Type) {
			case FE_QPSK:
				m_MinDelay = 50;
				m_StepSize = m_Parameters.u.QPSK.SymbolRate / 16000;
				m_MaxDrift = m_Parameters.u.QPSK.SymbolRate / 2000;
				break;
				
			case FE_QAM:
				m_MinDelay = 50;
				m_StepSize = 0; /* no zigzag */
				m_MaxDrift = 0;
				break;
				
			case FE_OFDM:
				m_MinDelay = 50;
				m_StepSize = m_FrontendInfo.FrequencyStepSize * 2;
				m_MaxDrift = (m_FrontendInfo.FrequencyStepSize * 2) + 1;
				break;
			case FE_ATSC:
				m_MinDelay = 50;
				m_StepSize = 0;
				m_MaxDrift = 0;
				break;
		}
	}
	//	if (dvb_override_tune_delay > 0)
	//		m_min_delay = (dvb_override_tune_delay * HZ) / 1000;
	//	
	
	
	m_State = FESTATE_RETUNE;
	//	dvb_frontend_wakeup(fe);
	AddEvent((kFEStatus)0);
	m_Status = (kFEStatus) 0;
	Wakeup();
	m_Sem.Up();	

	return 0;
}

int	IDVBFrontend::SetVoltage(kFESECVoltage params)
{
	return 0;
}
int	IDVBFrontend::SetTone(kFESECToneMode params)
{
	return 0;
}
void IDVBFrontend::OnStartup()
{
	CDVBLog::Log(kDVBLogFrontend, "frontend thread starting up.\n");
    m_State = FESTATE_IDLE;
	m_CheckWrapped = 0;
	m_Quality = 0;
	m_Delay = 2000;
	m_Status = (kFEStatus) 0;
	m_Wakeup = 0;
	m_Reinitialize = 0;
	m_Demod->Init();
	m_Demod->I2CGateCtrl(1);
    m_Tuner->Init();
	m_Demod->I2CGateCtrl(0);
	
}

void IDVBFrontend::SWZigZagUpdateDelay(int locked)
{
    int q2;
	
    if (locked)
        (m_Quality) = (m_Quality * 220 + 36*256) / 256;
    else
        (m_Quality) = (m_Quality * 220 + 0) / 256;
	
    q2 = m_Quality - 128;
    q2 *= q2;
	
    m_Delay = m_MinDelay + q2 * 1000 / (128*128);
}

int IDVBFrontend::SWZigZagAutoTune(int check_wrapped)
{
    int autoinversion;
    int ready = 0;
	
    int original_inversion = m_Parameters.Inversion;
    UInt32 original_frequency = m_Parameters.Frequency;
	
    /* are we using autoinversion? */
    autoinversion = ((!(m_FrontendInfo.Capabilities & FE_CAN_INVERSION_AUTO)) &&
					 (m_Parameters.Inversion == INVERSION_AUTO));
	
    /* setup parameters correctly */
    while(!ready) {
        /* calculate the lnb_drift */
        m_LNBDrift = m_AutoStep * m_StepSize;
		
        /* wrap the auto_step if we've exceeded the maximum drift */
        if (m_LNBDrift > m_MaxDrift) {
            m_AutoStep = 0;
            m_AutoSubStep = 0;
            m_LNBDrift = 0;
        }
		
        /* perform inversion and +/- zigzag */
        switch(m_AutoSubStep) {
			case 0:
				/* try with the current inversion and current drift setting */
				ready = 1;
				break;
				
			case 1:
				if (!autoinversion) break;
				
				m_Inversion = (m_Inversion == INVERSION_OFF) ? INVERSION_ON : INVERSION_OFF;
				ready = 1;
				break;
				
			case 2:
				if (m_LNBDrift == 0) break;
				
				m_LNBDrift = -m_LNBDrift;
				ready = 1;
				break;
				
			case 3:
				if (m_LNBDrift == 0) break;
				if (!autoinversion) break;
				
				m_Inversion = (m_Inversion == INVERSION_OFF) ? INVERSION_ON : INVERSION_OFF;
				m_LNBDrift = -m_LNBDrift;
				ready = 1;
				break;
				
			default:
				m_AutoStep++;
				m_AutoSubStep = -1; /* it'll be incremented to 0 in a moment */
				break;
        }
		
        if (!ready) m_AutoSubStep++;
    }
	
    /* if this attempt would hit where we started, indicate a complete
     * iteration has occurred */
    if ((m_AutoStep == m_StartedAutoStep) &&
        (m_AutoSubStep == 0) && check_wrapped) {
        return 1;
    }
	
	/* these are just for debugging the tuner */
//	m_Demod->ReadSignalStrength(&signal);

		/* these are just for debugging the tuner */
//	CDVBLog::Log(kDVBLogFrontend, "%s: drift:%i inversion:%i auto_step:%i auto_sub_step:%i started_auto_step:%i signal:%i\n",
//				 __func__, m_LNBDrift, m_Inversion,
//				 m_AutoStep, m_AutoSubStep, m_StartedAutoStep, signal);
	
/*	std::ostringstream Status;
	Status << "[";
	if (m_Status & FE_HAS_SIGNAL)  Status  << "SIG ";
	if (m_Status & FE_HAS_CARRIER) Status  << "CARR ";
	if (m_Status & FE_HAS_VITERBI) Status  << "VIT ";
	if (m_Status & FE_HAS_SYNC)    Status  << "SYNC ";
	if (m_Status & FE_HAS_LOCK)    Status  << "LOCK ";
	if (m_Status & FE_TIMEDOUT)    Status  << "TIMEOUT ";
	if (m_Status & FE_REINIT)      Status  << "REINIT ";
	Status << "]\n";

	CDVBLog::Log(kDVBLogFrontend, Status.str().c_str());
*/	
	
    /* set the frontend itself */
    m_Parameters.Frequency += m_LNBDrift;
    if (autoinversion)
        m_Parameters.Inversion = (kFESpectralInversion) m_Inversion;
    m_Demod->SetFrontendParams(&m_Parameters);
	
    m_Parameters.Frequency = original_frequency;
    m_Parameters.Inversion = (kFESpectralInversion) original_inversion;
	
    m_AutoSubStep++;
    return 0;
}

void IDVBFrontend::SWZigZag()
{
    kFEStatus s = (kFEStatus) 0;
	
    /* if we've got no parameters, just keep idling */
    if (m_State & FESTATE_IDLE) {
        m_Delay = 3000; /* 3*HZ = 3 seconds */
        m_Quality = 0;
        return;
    }

    /* in SCAN mode, we just set the frontend when asked and leave it alone */
    if (m_TuneModeFlags & kFETuneModeOneShot) {
        if (m_State & FESTATE_RETUNE) {
			m_Demod->SetFrontendParams(&m_Parameters);
            m_State = FESTATE_TUNED;
        }
        m_Delay = 3000; /* 3*HZ = 3 seconds */
        m_Quality = 0;
        return;
    }
	
    /* get the frontend status */
    if (m_State & FESTATE_RETUNE) {
        s = (kFEStatus) 0;
    } else {
		m_Demod->ReadStatus(&s);
        if (s != m_Status) {
            AddEvent(s);
            m_Status = s;
			
        }
    }
	
	
    /* if we're not tuned, and we have a lock, move to the TUNED state */
    if ((m_State & FESTATE_WAITFORLOCK) && (s & FE_HAS_LOCK)) {
        SWZigZagUpdateDelay(s & FE_HAS_LOCK);
        m_State = FESTATE_TUNED;
		
        /* if we're tuned, then we have determined the correct inversion */
        if ((!(m_FrontendInfo.Capabilities & FE_CAN_INVERSION_AUTO)) &&
            (m_Parameters.Inversion == INVERSION_AUTO)) {
            m_Parameters.Inversion = (kFESpectralInversion) m_Inversion;
        }
        return;
    }
	
    /* if we are tuned already, check we're still locked */
    if (m_State & FESTATE_TUNED) {
        SWZigZagUpdateDelay(s & FE_HAS_LOCK);
		
        /* we're tuned, and the lock is still good... */
        if (s & FE_HAS_LOCK) {
            return;
        } else { /* if we _WERE_ tuned, but now don't have a lock */
            m_State = FESTATE_ZIGZAG_FAST;
            m_StartedAutoStep = m_AutoStep;
			m_CheckWrapped = 0;
        }
    }
	
    /* don't actually do anything if we're in the LOSTLOCK state,
     * the frontend is set to FE_CAN_RECOVER, and the max_drift is 0 */
    if ((m_State & FESTATE_LOSTLOCK) &&
        (m_FrontendInfo.Capabilities & FE_CAN_RECOVER) && (m_MaxDrift == 0)) {
        SWZigZagUpdateDelay(s & FE_HAS_LOCK);
        return;
    }
	
    /* don't do anything if we're in the DISEQC state, since this
     * might be someone with a motorized dish controlled by DISEQC.
     * If its actually a re-tune, there will be a SET_FRONTEND soon enough. */
    if (m_State & FESTATE_DISEQC) {
        SWZigZagUpdateDelay(s & FE_HAS_LOCK);
        return;
    }
	
    /* if we're in the RETUNE state, set everything up for a brand
     * new scan, keeping the current inversion setting, as the next
     * tune is _very_ likely to require the same */
    if (m_State & FESTATE_RETUNE) {
        m_LNBDrift = 0;
        m_AutoStep = 0;
        m_AutoSubStep = 0;
        m_StartedAutoStep = 0;
        m_CheckWrapped = 0;
    }
	
    /* fast zigzag. */
    if ((m_State & FESTATE_SEARCHING_FAST) || (m_State & FESTATE_RETUNE)) {
        m_Delay = m_MinDelay;
		
        /* peform a tune */
        if (SWZigZagAutoTune(m_CheckWrapped)) {
            /* OK, if we've run out of trials at the fast speed.
             * Drop back to slow for the _next_ attempt */
            m_State = FESTATE_SEARCHING_SLOW;
            m_StartedAutoStep = m_AutoStep;
            return;
        }
        m_CheckWrapped = 1;
		
        /* if we've just retuned, enter the ZIGZAG_FAST state.
         * This ensures we cannot return from an
         * FE_SET_FRONTEND ioctl before the first frontend tune
         * occurs */
        if (m_State & FESTATE_RETUNE) {
            m_State = FESTATE_TUNING_FAST;
        }
    }
	
    /* slow zigzag */
    if (m_State & FESTATE_SEARCHING_SLOW) {
        SWZigZagUpdateDelay(s & FE_HAS_LOCK);
		
        /* Note: don't bother checking for wrapping; we stay in this
         * state until we get a lock */
        SWZigZagAutoTune(0);
    }
}

void IDVBFrontend::Process()
{
	kFEStatus s;
	kFEAlgo algo;
	IDVBCondition cond;
	DVBFrontendParameters *params;

	while (1) {
		m_Sem.Up();		
	restart:	
		cond.Lock();
		m_Wait.Add(&cond);
		if (!ShouldWakeup())
		{
			cond.TimedWait(m_Delay);
		}
		
		cond.Unlock();
		
		if (IsExiting()) {
			/* got signal or quitting */
			break;
		}
		
		
		m_Sem.Down();
		
		if (m_Reinitialize) {
			m_Demod->Init();
			m_Demod->I2CGateCtrl(1);
			m_Tuner->Init();
			m_Demod->I2CGateCtrl(0);
			
			if (m_Tone != -1) {
				m_Demod->SetTone((kFESECToneMode) m_Tone);
			}
			if (m_Voltage != -1) {
				m_Demod->SetVoltage((kFESECVoltage) m_Voltage);
			}
			m_Reinitialize = 0;
		}
		
		/* do an iteration of the tuning loop */
		algo = m_Demod->GetFrontendAlgo();
		switch (algo) {
			case kFEAlgoHW:
				CDVBLog::Log(kDVBLogFrontend, "%s: Frontend ALGO = DVBFE_ALGO_HW\n", __func__);
				params = NULL; /* have we been asked to RETUNE ? */
				
				if (m_State & FESTATE_RETUNE) {
					CDVBLog::Log(kDVBLogFrontend, "%s: Retune requested, FESTATE_RETUNE\n", __func__);
					params = &m_Parameters;
					m_State = FESTATE_TUNED;
				}
				
				m_Demod->Tune(params, m_TuneModeFlags, &m_Delay, &s);
				
				if (s != m_Status && !(m_TuneModeFlags & kFETuneModeOneShot)) {
					CDVBLog::Log(kDVBLogFrontend,  "%s: state changed, adding current state\n", __func__);
					AddEvent(s);
					m_Status = s;
				}
				break;
			case kFEAlgoSW:
				CDVBLog::Log(kDVBLogFrontend, "%s: Frontend ALGO = DVBFE_ALGO_SW\n", __func__);
				SWZigZag();
				break;
			case kFEAlgoCustom:
				params = NULL; /* have we been asked to RETUNE ?    */
				CDVBLog::Log(kDVBLogFrontend,  "%s: Frontend ALGO = DVBFE_ALGO_CUSTOM, state=%d\n", __func__, m_State);
				if (m_State & FESTATE_RETUNE) {
					CDVBLog::Log(kDVBLogFrontend, "%s: Retune requested, FESTAT_RETUNE\n", __func__);
					params = &m_Parameters;
					m_State = FESTATE_TUNED;
				}
				/* Case where we are going to search for a carrier
				 * User asked us to retune again for some reason, possibly
				 * requesting a search with a new set of parameters
				 */
				if (m_AlgoStatus & kFEAlgoSearchAgain) {
					
					// Need to work out how to do an if/else on this, since we can't establish whether we define a search func or not
					
					//					if (fe->ops.search) {
					//						fepriv->algo_status = fe->ops.search(fe, &fepriv->parameters);
					/* We did do a search as was requested, the flags are
					 * now unset as well and has the flags wrt to search.
					 */
					//					} else {
					//						fepriv->algo_status = (dvbfe_search) (fepriv->algo_status & ~DVBFE_ALGO_SEARCH_AGAIN);
					//					}
					// To here					
					
					
				}
				/* Track the carrier if the search was successful */
				if (m_AlgoStatus == kFEAlgoSearchSuccess) {
					m_Demod->Track(&m_Parameters);
					//					if (fe->ops.track)
					//						fe->ops.track(fe, &fepriv->parameters);
				} else {
					m_AlgoStatus = (kFESearch) (m_AlgoStatus | kFEAlgoSearchAgain);
					m_Delay = 500; /* HZ / 2 = 1/2 of a second */
				}
				m_Demod->ReadStatus(&s);
				if (s != m_Status) {
					AddEvent(s); /* update event list */
					m_Status = s;
					if (!(s & FE_HAS_LOCK)) {
						m_Delay = 100; /* HZ / 10 = 1/10 of a second */
						m_AlgoStatus = (kFESearch) (m_AlgoStatus | kFEAlgoSearchAgain);
					} else {
						m_Delay = 60000; /* 60 * HZ = 60 Seconds */
					}
				}
				break;
			default:
				CDVBLog::Log(kDVBLogFrontend, "%s: UNDEFINED ALGO !\n", __func__);
				break;
		}
	}
	
	return;
}




void IDVBFrontend::GetFrequencyLimits(UInt32 *freq_min, UInt32 *freq_max)
{
	
	IDVBTuner *tuner = m_Tuner;
    *freq_min = std::max(m_FrontendInfo.FrequencyMin, tuner->m_TunerInfo.FrequencyMin);
	
    if (m_FrontendInfo.FrequencyMax == 0)
        *freq_max = m_Tuner->m_TunerInfo.FrequencyMax;
    else if (m_Tuner->m_TunerInfo.FrequencyMax == 0)
        *freq_max = m_Tuner->m_TunerInfo.FrequencyMax;
    else
        *freq_max = std::min(m_FrontendInfo.FrequencyMax, m_Tuner->m_TunerInfo.FrequencyMax);
	
    if (*freq_min == 0 || *freq_max == 0)
        printf("DVB: adapter %i frontend %u frequency limits undefined - fix the driver\n",
               this, 0);
}

int IDVBFrontend::GetInfo(DVBFrontendInfo *info)
{
	memcpy(info, &m_FrontendInfo, sizeof(struct DVBFrontendInfo));
	GetFrequencyLimits((UInt32 *)&info->FrequencyMin, (UInt32 *)&info->FrequencyMax);
	
	/* Force the CAN_INVERSION_AUTO bit on. If the frontend doesn't
	 * do it, it is done for it. */
	info->Capabilities = (kFECaps) (info->Capabilities | FE_CAN_INVERSION_AUTO);
	
	return 0;
}