/*
 *  Dib0070.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 10/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <unistd.h>

#include "Dib0070.h"
#include "CDVBLog.h"
using namespace Video4Mac;
using namespace Dibcom;


#define DIB0070_P1D  0x00
#define DIB0070_P1F  0x01
#define DIB0070_P1G  0x03
#define DIB0070S_P1A 0x02

static UInt16 dib0070_p1f_defaults[] =

{
7, 0x02,
0x0008,
0x0000,
0x0000,
0x0000,
0x0000,
0x0002,
0x0100,

3, 0x0d,
0x0d80,
0x0001,
0x0000,

4, 0x11,
0x0000,
0x0103,
0x0000,
0x0000,

3, 0x16,
0x0004 | 0x0040,
0x0030,
0x07ff,

6, 0x1b,
0x4112,
0xff00,
0xc07f,
0x0000,
0x0180,
0x4000 | 0x0800 | 0x0040 | 0x0020 | 0x0010 | 0x0008 | 0x0002 | 0x0001,

0,
};

Dib0070::Dib0070()
{
	m_I2CAddress = DEFAULT_DIB0070_I2C_ADDRESS;

	
	m_Dib0070Config.m_FreqOffsetKhzUHF = 0;
	m_Dib0070Config.m_FreqOffsetKhzVHF = 0;
	m_Dib0070Config.m_OscBufferState = 0;
	m_Dib0070Config.m_ClockKhz = 12000;
	m_Dib0070Config.m_ClockPadDrive = 4; /* only used for adapter id 0 */
	m_Dib0070Config.m_ForceCrystalMode = 0; /* if == 0 -> decision is made in the driver default: <24 -> 2, >=24 -> 1 */
	m_Dib0070Config.m_InvertIQ = 0;
	m_Dib0070Config.m_FlipChip = 0;
	
	m_TunerInfo.Name         = "DiBcom DiB0070";
	m_TunerInfo.FrequencyMin =  45000000;
	m_TunerInfo.FrequencyMax = 860000000;
	m_TunerInfo.FrequencyStep=      1000;
}	

uint16_t Dib0070::ReadReg(UInt8 reg)
{
	
	UInt8 b[2];
	struct i2c_msg msg[2] = {
		{ addr : m_I2CAddress, flags : 0,        len : 1, buf : &reg },
		{ addr : m_I2CAddress, flags : I2C_M_RD, len : 2, buf : b	 },
	};
	if (m_I2CAdapter->Transfer(msg, 2) != 2) {
		CDVBLog::Log(kDVBLogTuner, "DiB0070 I2C read failed\n");
		return 0;
	}
	return (b[0] << 8) | b[1];
}

int Dib0070::WriteReg(UInt8 reg, UInt16 val)
{
	UInt8 b[3] = { reg, val >> 8, val & 0xff };
	struct i2c_msg msg = { addr : m_I2CAddress, flags : 0, len : 3, buf : b  };
	if (m_I2CAdapter->Transfer(&msg, 1) != 1) {
		CDVBLog::Log(kDVBLogTuner, "DiB0070 I2C write failed\n");
		return -1;
	}
	return 0;
}

int Dib0070::SetBandwidth(DVBFrontendParameters *ch)
{
	UInt16 tmp = 0;
	tmp = ReadReg(0x02) & 0x3fff;
	
    switch(BANDWIDTH_TO_KHZ(ch->u.OFDM.Bandwidth)) {
		case  8000:
			tmp |= (0 << 14);
			break;
		case  7000:
			tmp |= (1 << 14);
			break;
		case  6000:
			tmp |= (2 << 14);
			break;
		case 5000:
		default:
			tmp |= (3 << 14);
			break;
	}
	WriteReg(0x02, tmp);
	return 0;
}

void Dib0070::CapTrim(UInt16 LO4)
{
	int8_t captrim, fcaptrim, step_sign, step;
	UInt16 adc, adc_diff = 3000;
	
	
	
	WriteReg(0x0f, 0xed10);
	WriteReg(0x17,    0x0034);
	
	WriteReg(0x18, 0x0032);
	usleep(2000);
	
	step = captrim = fcaptrim = 64;
	
	do {
		step /= 2;
		WriteReg(0x14, LO4 | captrim);
		usleep(1000);
		adc = ReadReg(0x19);
		
		CDVBLog::Log(kDVBLogTuner, "CAPTRIM=%hd; ADC = %hd (ADC) & %dmV", captrim, adc, (UInt32) adc*(UInt32)1800/(UInt32)1024);
		
		if (adc >= 400) {
			adc -= 400;
			step_sign = -1;
		} else {
			adc = 400 - adc;
			step_sign = 1;
		}
		
		if (adc < adc_diff) {
			CDVBLog::Log(kDVBLogTuner, "CAPTRIM=%hd is closer to target (%hd/%hd)", captrim, adc, adc_diff);
			adc_diff = adc;
			fcaptrim = captrim;
			
			
			
		}
		captrim += (step_sign * step);
	} while (step >= 1);
	
	WriteReg(0x14, LO4 | fcaptrim);
	WriteReg(0x18, 0x07ff);
}

#define LPF	100                       // define for the loop filter 100kHz by default 16-07-06
#define LO4_SET_VCO_HFDIV(l, v, h)   l |= ((v) << 11) | ((h) << 7)
#define LO4_SET_SD(l, s)             l |= ((s) << 14) | ((s) << 12)
#define LO4_SET_CTRIM(l, c)          l |=  (c) << 10
int Dib0070::SetParams(/* struct dvb_frontend *fe, */ DVBFrontendParameters *ch)
{
//	struct dib0070_state *st = (struct dib0070_state *)fe->tuner_priv;
		CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	UInt32 freq = ch->Frequency/1000 + (BAND_OF_FREQUENCY(ch->Frequency/1000) == BAND_VHF ? m_Dib0070Config.m_FreqOffsetKhzVHF : m_Dib0070Config.m_FreqOffsetKhzUHF);
	
	UInt8 band = BAND_OF_FREQUENCY(freq), c;
	
	/*******************VCO***********************************/
	UInt16 lo4 = 0;
	
	UInt8 REFDIV, PRESC = 2;
	UInt32 FBDiv, Rest, FREF, VCOF_kHz;
	UInt16 Num, Den;
	/*******************FrontEnd******************************/
	UInt16 value = 0;
	
	CDVBLog::Log(kDVBLogTuner, "Tuning for Band: %hd (%d kHz)", band, freq);
	
	
	WriteReg(0x17, 0x30);
	
	SetBandwidth(ch);	/* c is used as HF */
	switch (m_Revision) {
		case DIB0070S_P1A:
			switch (band) {
				case BAND_LBAND:
					LO4_SET_VCO_HFDIV(lo4, 1, 1);
					c = 2;
					break;
				case BAND_SBAND:
					LO4_SET_VCO_HFDIV(lo4, 0, 0);
					LO4_SET_CTRIM(lo4, 1);;
					c = 1;
					break;
				case BAND_UHF:
				default:
					if (freq < 570000) {
						LO4_SET_VCO_HFDIV(lo4, 1, 3);
						PRESC = 6; c = 6;
					} else if (freq < 680000) {
						LO4_SET_VCO_HFDIV(lo4, 0, 2);
						c = 4;
					} else {
						LO4_SET_VCO_HFDIV(lo4, 1, 2);
						c = 4;
					}
					break;
			} break;
			
		case DIB0070_P1G:
		case DIB0070_P1F:
		default:
			switch (band) {
				case BAND_FM:
					LO4_SET_VCO_HFDIV(lo4, 0, 7);
					c = 24;
					break;
				case BAND_LBAND:
					LO4_SET_VCO_HFDIV(lo4, 1, 0);
					c = 2;
					break;
				case BAND_VHF:
					if (freq < 180000) {
						LO4_SET_VCO_HFDIV(lo4, 0, 3);
						c = 16;
					} else if (freq < 190000) {
						LO4_SET_VCO_HFDIV(lo4, 1, 3);
						c = 16;
					} else {
						LO4_SET_VCO_HFDIV(lo4, 0, 6);
						c = 12;
					}
					break;
					
				case BAND_UHF:
				default:
					if (freq < 570000) {
						LO4_SET_VCO_HFDIV(lo4, 1, 5);
						c = 6;
					} else if (freq < 700000) {
						LO4_SET_VCO_HFDIV(lo4, 0, 1);
						c = 4;
					} else {
						LO4_SET_VCO_HFDIV(lo4, 1, 1);
						c = 4;
					}
					break;
			}
			break;
	}
	
	CDVBLog::Log(kDVBLogTuner, "HFDIV code: %hd\n", (lo4 >> 7) & 0xf);
	CDVBLog::Log(kDVBLogTuner, "VCO = %hd\n", (lo4 >> 11) & 0x3);
	
	
	VCOF_kHz = (c * freq) * 2;
	CDVBLog::Log(kDVBLogTuner, "VCOF in kHz: %d ((%hd*%d) << 1))\n",VCOF_kHz, c, freq);
	
	switch (band) {
		case BAND_VHF:
			REFDIV = (UInt8) ((m_Dib0070Config.m_ClockKhz + 9999) / 10000);
			break;
		case BAND_FM:
			REFDIV = (UInt8) ((m_Dib0070Config.m_ClockKhz) / 1000);
			break;
		default:
			REFDIV = (UInt8) ( m_Dib0070Config.m_ClockKhz  / 10000);
			break;
	}
	FREF = m_Dib0070Config.m_ClockKhz / REFDIV;
	
	CDVBLog::Log(kDVBLogTuner, "REFDIV: %hd, FREF: %d\n", REFDIV, FREF);
	
	
	
	switch (m_Revision) {
		case DIB0070S_P1A:
			FBDiv = (VCOF_kHz / PRESC / FREF);
			Rest  = (VCOF_kHz / PRESC) - FBDiv * FREF;
			break;
			
		case DIB0070_P1G:
		case DIB0070_P1F:
		default:
			FBDiv = (freq / (FREF / 2));
			Rest  = 2 * freq - FBDiv * FREF;
			break;
	}
	
	
	if (Rest < LPF)              Rest = 0;
	else if (Rest < 2 * LPF)          Rest = 2 * LPF;
	else if (Rest > (FREF - LPF))   { Rest = 0 ; FBDiv += 1; }
	else if (Rest > (FREF - 2 * LPF)) Rest = FREF - 2 * LPF;
	Rest = (Rest * 6528) / (FREF / 10);
	CDVBLog::Log(kDVBLogTuner, "FBDIV: %d, Rest: %d\n", FBDiv, Rest);
	
	Num = 0;
	Den = 1;
	
	if (Rest > 0) {
		LO4_SET_SD(lo4, 1);
		Den = 255;
		Num = (UInt16)Rest;
	}
	CDVBLog::Log(kDVBLogTuner, "Num: %hd, Den: %hd, SD: %hd\n",Num, Den, (lo4 >> 12) & 0x1);
	
	
	
	WriteReg(0x11, (UInt16)FBDiv);
	
	
	WriteReg(0x12, (Den << 8) | REFDIV);
	
	
	WriteReg(0x13, Num);
	
	
	value = 0x0040 | 0x0020 | 0x0010 | 0x0008 | 0x0002 | 0x0001;
	
	switch (band) {
		case BAND_UHF:   value |= 0x4000 | 0x0800; break;
		case BAND_LBAND: value |= 0x2000 | 0x0400; break;
		default:         value |= 0x8000 | 0x1000; break;
	}
	WriteReg(0x20, value);
	
	CapTrim(lo4);
	if (m_Revision == DIB0070S_P1A) {
		if (band == BAND_SBAND)
			WriteReg(0x15, 0x16e2);
		else
			WriteReg(0x15, 0x56e5);
	}
	
	
	
	switch (band) {
		case BAND_UHF:   value = 0x7c82; break;
		case BAND_LBAND: value = 0x7c84; break;
		default:         value = 0x7c81; break;
	}
	WriteReg(0x0f, value);
	WriteReg(0x06, 0x3fff);
	
	/* Front End */
	/* c == TUNE, value = SWITCH */
	c = 0;
	value = 0;
	switch (band) {
		case BAND_FM:
			c = 0; value = 1;
			break;
			
		case BAND_VHF:
			if (freq <= 180000) c = 0;
			else if (freq <= 188200) c = 1;
			else if (freq <= 196400) c = 2;
			else c = 3;
			value = 1;
			break;
			
		case BAND_LBAND:
			if (freq <= 1500000) c = 0;
			else if (freq <= 1600000) c = 1;
			else c = 3;
			break;
			
		case BAND_SBAND:
			c = 7;
			WriteReg(0x1d,0xFFFF);
			break;
			
		case BAND_UHF:
		default:
			if (m_Dib0070Config.m_FlipChip) {
				if (freq <= 550000) c = 0;
				else if (freq <= 590000) c = 1;
				else if (freq <= 666000) c = 3;
				else c = 5;
			} else {
				if (freq <= 550000) c = 2;
				else if (freq <= 650000) c = 3;
				else if (freq <= 750000) c = 5;
				else if (freq <= 850000) c = 6;
				else c = 7;
			}
			value = 2;
			break;
	}
	
	/* default: LNA_MATCH=7, BIAS=3 */
	WriteReg(0x07, (value << 11) | (7 << 8) | (c << 3) | (3 << 0));
	WriteReg(0x08, (c << 10) | (3 << 7) | (127));
	WriteReg(0x0d, 0x0d80);
	
	
	WriteReg(0x18,   0x07ff);
	WriteReg(0x17, 0x0033);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib0070::Init()
{
	TunerSleep(0);
	return 0;
}

int Dib0070::Sleep()
{
	TunerSleep(1);
	return 0;
}


void Dib0070::WBDCalibration()
{
	UInt16 wbd_offs;
	CDVBLog::Log(kDVBLogTuner, "%s\n", __func__);
	TunerSleep(0);

	WriteReg(0x0f, 0x6d81);
	WriteReg(0x20, 0x0040 | 0x0020 | 0x0010 | 0x0008 | 0x0002 | 0x0001);
	usleep(9000);
	wbd_offs = ReadReg(0x19);
	WriteReg(0x20, 0);
	m_WBDFFOffset = ((wbd_offs * 8 * 18 / 33 + 1) / 2);
	CDVBLog::Log(kDVBLogTuner, "WBDStart = %d (Vargen) - FF = %hd", (UInt32) wbd_offs * 1800/1024, m_WBDFFOffset);
	CDVBLog::Log(kDVBLogTuner, "%s done\n", __func__);
	TunerSleep(1);
	
}

UInt16 Dib0070::WBDOffset()
{
	return m_WBDFFOffset;
}

int Dib0070::SetControlLo5(UInt8 vco_bias_trim, UInt8 hf_div_trim, UInt8 cp_current, UInt8 third_order_filt)
{
//	struct dib0070_state *state = (struct dib0070_state *)fe->tuner_priv;
	CDVBLog::Log(kDVBLogTuner, "%s\n", __func__);
    UInt16 lo5 = (third_order_filt << 14) | (0 << 13) | (1 << 12) | (3 << 9) | (cp_current << 6) | (hf_div_trim << 3) | (vco_bias_trim << 0);
	CDVBLog::Log(kDVBLogTuner, "CTRL_LO5: 0x%x\n", lo5);
	CDVBLog::Log(kDVBLogTuner, "%s done - one write more\n", __func__);
	return WriteReg(0x15, lo5);
}

#define pgm_read_word(w) (*w)
int Dib0070::Reset()
{
	CDVBLog::Log(kDVBLogTuner, "%s\n", __func__);
	UInt16 l, r, *n;
	
//	HARD_RESET();
	do 
	{ 
		TunerReset(1); 
		usleep(10000); 
		TunerReset(0);
		usleep(10000); 
	} while (0);
	
#ifndef FORCE_SBAND_TUNER
	if ((ReadReg(0x22) >> 9) & 0x1)
		m_Revision = (ReadReg(0x1f) >> 8) & 0xff;
	else
#endif
		m_Revision = DIB0070S_P1A;
	
	/* P1F or not */
	CDVBLog::Log(kDVBLogTuner, "Revision: %x\n", m_Revision);
	
	if (m_Revision == DIB0070_P1D) {
		CDVBLog::Log(kDVBLogTuner, "Error: this driver is not to be used meant for P1D or earlier");
		return -EINVAL;
	}
	
	CDVBLog::Log(kDVBLogTuner, "going to write dib0070 defaults.\n");
	n = (UInt16 *) dib0070_p1f_defaults;
	l = pgm_read_word(n++);
	while (l) {
		r = pgm_read_word(n++);
		do {
			WriteReg((UInt8)r, pgm_read_word(n++));
			usleep(1000);
			r++;
		} while (--l);
		l = pgm_read_word(n++);
	}
	CDVBLog::Log(kDVBLogTuner, "finished writing dib0070 defaults.\n");

	if (m_Dib0070Config.m_ForceCrystalMode != 0)
		r = m_Dib0070Config.m_ForceCrystalMode;
	else if (m_Dib0070Config.m_ClockKhz >= 24000)
		r = 1;
	else
		r = 2;
	
	r |= m_Dib0070Config.m_OscBufferState << 3;
	
	WriteReg(0x10, r);
	WriteReg(0x1f, (1 << 8) | ((m_Dib0070Config.m_ClockPadDrive & 0xf) << 4));
	
	if (m_Dib0070Config.m_InvertIQ) {
		r = ReadReg(0x02) & 0xffdf;
		WriteReg(0x02, r | (1 << 5));
	}
	
	
	if (m_Revision == DIB0070S_P1A)
		SetControlLo5(4, 7, 3, 1);
	else
		SetControlLo5(4, 4, 2, 0);
	
#if 0
	/* BBFilter calib disabled until further notice */
	
	WriteReg(0x1e, 0x0010);
	msleep(10);
	r = ReadReg(0x1e) >> 6;
	
	if (r == 0)
		r = 54;
	else
		r = (UInt16) (149 - (r * 3112) / state->cfg->clock_khz);
	
	WriteReg(0x01, (r << 9) | 0xc8);
#else
	WriteReg(0x01, (54 << 9) | 0xc8);
#endif
	CDVBLog::Log(kDVBLogTuner, "%s\n", __func__);
	return 0;
}

int Dib0070::TunerAttach(IDVBI2CAdapter *i2c)
{
	m_I2CAdapter = i2c;

	CDVBLog::Log(kDVBLogTuner, "%s: going to reset tuner.\n", __func__);
	if (Reset() != 0)
		goto free_mem;

	CDVBLog::Log(kDVBLogTuner, "%s: going to calibrate tuner.\n", __func__);
	WBDCalibration();
	
	CDVBLog::Log(kDVBLogTuner, "%s: DiB0070: successfully identified\n", __func__);
	
	return 1;
	
free_mem:
	return 0;
}


int Dib7000P::ReadStatus(kFEStatus *stat)
{
	UInt16 lock = ReadWord(509);
	
	*stat = (kFEStatus) 0;
	
	if (lock & 0x8000)
		*stat = (kFEStatus) ((int) (*stat) | FE_HAS_SIGNAL);
	if (lock & 0x3000)
		*stat = (kFEStatus) ((int) (*stat) | FE_HAS_CARRIER);
	if (lock & 0x0100)
		*stat = (kFEStatus) ((int) (*stat) | FE_HAS_VITERBI);
	if (lock & 0x0010)
		*stat = (kFEStatus) ((int) (*stat) | FE_HAS_SYNC);
	if (lock & 0x0008)
		*stat = (kFEStatus) ((int) (*stat) | FE_HAS_LOCK);
	
	return 0;
}

int Dib7000P::ReadBER(UInt32 *ber)
{
	*ber = (ReadWord(500) << 16) | ReadWord(501);
	return 0;
}

int Dib7000P::ReadUNCBlocks(UInt32 *unc)
{
	*unc = ReadWord(506);
	return 0;
}

int Dib7000P::ReadSignalStrength(UInt16 *strength)
{
	UInt16 val = ReadWord(394);
	*strength = 65535 - val;
	return 0;
}

int Dib7000P::ReadSNR(UInt16 *snr)
{
	*snr = 0x0000;
	return 0;
}