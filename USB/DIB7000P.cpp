/*
 *  DIB7000P.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 09/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "DIB7000P.h"
//#include "Firmware.h"
#include <unistd.h>
#include <IDVBTuner.h>
#include <IDVBFrontend.h>
#include <CDVBLog.h>

using namespace Video4Mac;
using namespace Dibcom;

static struct dibx000_agc_config dib7070_agc_config = {
BAND_UHF | BAND_VHF | BAND_LBAND | BAND_SBAND,
/* P_agc_use_sd_mod1=0, P_agc_use_sd_mod2=0, P_agc_freq_pwm_div=5, P_agc_inv_pwm1=0, P_agc_inv_pwm2=0,
 * P_agc_inh_dc_rv_est=0, P_agc_time_est=3, P_agc_freeze=0, P_agc_nb_est=5, P_agc_write=0 */
(0 << 15) | (0 << 14) | (5 << 11) | (0 << 10) | (0 << 9) | (0 << 8) | (3 << 5) | (0 << 4) | (5 << 1) | (0 << 0), // setup

600, // inv_gain
10,  // time_stabiliz

0,  // alpha_level
118,  // thlock

0,     // wbd_inv
3530,  // wbd_ref
1,     // wbd_sel
5,     // wbd_alpha

65535,  // agc1_max
0,  // agc1_min

65535,  // agc2_max
0,      // agc2_min

0,      // agc1_pt1
40,     // agc1_pt2
183,    // agc1_pt3
206,    // agc1_slope1
255,    // agc1_slope2
72,     // agc2_pt1
152,    // agc2_pt2
88,     // agc2_slope1
90,     // agc2_slope2

17,  // alpha_mant
27,  // alpha_exp
23,  // beta_mant
51,  // beta_exp

0,  // perform_agc_softsplit
};

static struct dibx000_bandwidth_config dib7070_bw_config_12_mhz = {
60000, 15000, // internal, sampling
1, 20, 3, 1, 0, // pll_cfg: prediv, ratio, range, reset, bypass
0, 0, 1, 1, 2, // misc: refdiv, bypclk_div, IO_CLK_en_core, ADClkSrc, modulo
(3 << 14) | (1 << 12) | (524 << 0), // sad_cfg: refsel, sel, freq_15k
(0 << 25) | 0, // ifreq = 0.000000 MHz
20452225, // timf
12000000, // xtal_hz
};

static UInt16 dib7000p_defaults[] =

{
// auto search configuration
3, 2,
0x0004,
0x1000,
0x0814, /* Equal Lock */

12, 6,
0x001b,
0x7740,
0x005b,
0x8d80,
0x01c9,
0xc380,
0x0000,
0x0080,
0x0000,
0x0090,
0x0001,
0xd4c0,

1, 26,
0x6680, // P_timf_alpha=6, P_corm_alpha=6, P_corm_thres=128 default: 6,4,26

/* set ADC level to -16 */
11, 79,
(1 << 13) - 825 - 117,
(1 << 13) - 837 - 117,
(1 << 13) - 811 - 117,
(1 << 13) - 766 - 117,
(1 << 13) - 737 - 117,
(1 << 13) - 693 - 117,
(1 << 13) - 648 - 117,
(1 << 13) - 619 - 117,
(1 << 13) - 575 - 117,
(1 << 13) - 531 - 117,
(1 << 13) - 501 - 117,

1, 142,
0x0410, // P_palf_filter_on=1, P_palf_filter_freeze=0, P_palf_alpha_regul=16

/* disable power smoothing */
8, 145,
0,
0,
0,
0,
0,
0,
0,
0,

1, 154,
1 << 13, // P_fft_freq_dir=1, P_fft_nb_to_cut=0

1, 168,
0x0ccd, // P_pha3_thres, default 0x3000

//	1, 169,
//		0x0010, // P_cti_use_cpe=0, P_cti_use_prog=0, P_cti_win_len=16, default: 0x0010

1, 183,
0x200f, // P_cspu_regul=512, P_cspu_win_cut=15, default: 0x2005

5, 187,
0x023d, // P_adp_regul_cnt=573, default: 410
0x00a4, // P_adp_noise_cnt=
0x00a4, // P_adp_regul_ext
0x7ff0, // P_adp_noise_ext
0x3ccc, // P_adp_fil

1, 198,
0x800, // P_equal_thres_wgn

1, 222,
0x0010, // P_fec_ber_rs_len=2

1, 235,
0x0062, // P_smo_mode, P_smo_rs_discard, P_smo_fifo_flush, P_smo_pid_parse, P_smo_error_discard

2, 901,
0x0006, // P_clk_cfg1
(3 << 10) | (1 << 6), // P_divclksel=3 P_divbitsel=1

1, 905,
0x2c8e, // Tuner IO bank: max drive (14mA) + divout pads max drive

0,
};


Dib7000P::Dib7000P()
{
	
	////////////////////////////////////////////////////////////////////
	//
	// DIB7000P Configuration
	//
	////////////////////////////////////////////////////////////////////
	
	cfg.m_OutputMPEG2in188Bytes = 1;
	cfg.m_HostbusDiversity = 1;
	cfg.m_TunerIsBaseband = 1;
	// update_lna = NULL;
	//		int (*update_lna) (struct dvb_frontend *, UInt16 agc_global);
	cfg.m_AGCConfigCount = 1;
	cfg.agc = &dib7070_agc_config;
	cfg.bw = &dib7070_bw_config_12_mhz;
	cfg.m_GPIODir = DIB7000P_GPIO_DEFAULT_DIRECTIONS;
	cfg.m_GPIOVal = DIB7000P_GPIO_DEFAULT_VALUES;
	cfg.m_GPIOPWMPos = DIB7000P_GPIO_DEFAULT_PWM_POS;
	cfg.m_PWMFreqDiv = 0;
	cfg.m_QuartzDirect = 0;
	cfg.m_SpurProtect = 1;

	////////////////////////////////////////////////////////////////////
	//
	// DIB7000P State Variables
	//
	////////////////////////////////////////////////////////////////////
	m_Timf = 0;
	m_WBDRef = 0;
	m_CurrentBand = 0;
	m_CurrentBandwidth = 0;
	m_CurrentAGC = NULL;
	m_DivForceOff = 0;
	m_DivState = 0;
	m_DivSyncWait = 0;
	m_AGCState = 0;
	m_GPIODir = cfg.m_GPIODir;
	m_GPIOVal = cfg.m_GPIOVal;
	m_SFNWorkaroundActive = 0;

}



int	Dib7000P::GetTuneSettings(struct DVBFrontendTuneSettings* settings)
{
	settings->MinDelayms = 1000;
	return 0; 
};



////////////////////////////////////////////////////////////////////
//
// Demodulator functions
//
////////////////////////////////////////////////////////////////////

void Dib7000P::WriteTab(UInt16 *buf)
{
	UInt16 l = 0, r, *n;
		CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	n = buf;
	l = *n++;
	while (l) {
		r = *n++;
		
		do {
			WriteWord(r, *n++);
			r++;
		} while (--l);
		l = *n++;
	}
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}

int Dib7000P::SetOutputMode(int mode)
{
	int    ret = 0;
	UInt16 outreg, fifo_threshold, smo_mode;
	outreg = 0;
	fifo_threshold = 1792;
	smo_mode = (ReadWord(235) & 0x0010) | (1 << 1);
		CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	CDVBLog::Log(kDVBLogDemod, "setting output mode for demod %p to %d\n",
			0, mode);
	
	switch (mode) {
		case OUTMODE_MPEG2_PAR_GATED_CLK:   // STBs with parallel gated clock
			outreg = (1 << 10);  /* 0x0400 */
			break;
		case OUTMODE_MPEG2_PAR_CONT_CLK:    // STBs with parallel continues clock
			outreg = (1 << 10) | (1 << 6); /* 0x0440 */
			break;
		case OUTMODE_MPEG2_SERIAL:          // STBs with serial input
			outreg = (1 << 10) | (2 << 6) | (0 << 1); /* 0x0480 */
			break;
		case OUTMODE_DIVERSITY:
			if (cfg.m_HostbusDiversity)
				outreg = (1 << 10) | (4 << 6); /* 0x0500 */
			else
				outreg = (1 << 11);
			break;
		case OUTMODE_MPEG2_FIFO:            // e.g. USB feeding
			smo_mode |= (3 << 1);
			fifo_threshold = 512;
			outreg = (1 << 10) | (5 << 6);
			break;
		case OUTMODE_ANALOG_ADC:
			outreg = (1 << 10) | (3 << 6);
			break;
		case OUTMODE_HIGH_Z:  // disable
			outreg = 0;
			break;
		default:
			CDVBLog::Log(kDVBLogDemod, "Unhandled output_mode passed to be set for demod %p\n",this);
			break;
	}
	
	if (cfg.m_OutputMPEG2in188Bytes)
		smo_mode |= (1 << 5) ;
	
	ret |= WriteWord(235, smo_mode);
	ret |= WriteWord(236, fifo_threshold); /* synchronous fread */
	ret |= WriteWord(1286, outreg);         /* P_Div_active */
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return ret;
}

int Dib7000P::SetDiversityIn(int onoff)
{
//	struct dib7000p_state *state = (struct dib7000p_state *) demod->demodulator_priv;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);	
	if (m_DivForceOff) {
		CDVBLog::Log(kDVBLogDemod, "diversity combination deactivated - forced by COFDM parameters\n");
		onoff = 0;
	}
	m_DivState = (UInt8)onoff;
	
	if (onoff) {
		WriteWord(204, 6);
		WriteWord(205, 16);
		/* P_dvsy_sync_mode = 0, P_dvsy_sync_enable=1, P_dvcb_comb_mode=2 */
		WriteWord(207, (m_DivSyncWait << 4) | (1 << 2) | (2 << 0));
	} else {
		WriteWord(204, 1);
		WriteWord(205, 0);
		WriteWord(207, 0);
	}
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}


int Dib7000P::SetPowerMode(enum dib7000p_power_mode mode)
{
	/* by default everything is powered off */
	UInt16 reg_774 = 0xffff, reg_775 = 0xffff, reg_776 = 0x0007, reg_899  = 0x0003,
	reg_1280 = (0xfe00) | (ReadWord(1280) & 0x01ff);
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	/* now, depending on the requested mode, we power on */
	switch (mode) {
			/* power up everything in the demod */
		case DIB7000P_POWER_ALL:
			reg_774 = 0x0000; reg_775 = 0x0000; reg_776 = 0x0; reg_899 = 0x0; reg_1280 &= 0x01ff;
			break;
			
		case DIB7000P_POWER_ANALOG_ADC:
			/* dem, cfg, iqc, sad, agc */
			reg_774 &= ~((1 << 15) | (1 << 14) | (1 << 11) | (1 << 10) | (1 << 9));
			/* nud */
			reg_776 &= ~((1 << 0));
			/* Dout */
			reg_1280 &= ~((1 << 11));
			/* fall through wanted to enable the interfaces */
			
			/* just leave power on the control-interfaces: GPIO and (I2C or SDIO) */
		case DIB7000P_POWER_INTERFACE_ONLY: /* TODO power up either SDIO or I2C */
			reg_1280 &= ~((1 << 14) | (1 << 13) | (1 << 12) | (1 << 10));
			break;
			
			/* TODO following stuff is just converted from the dib7000-driver - check when is used what */
#if 0
		case DIB7000_POWER_LEVEL_INTERF_ANALOG_AGC:
			/* dem, cfg, iqc, sad, agc */
			reg_774  &= ~((1 << 15) | (1 << 14) | (1 << 11) | (1 << 10) | (1 << 9));
			/* sdio, i2c, gpio */
			reg_1280 &= ~((1 << 13) | (1 << 12) | (1 << 10));
			break;
		case DIB7000_POWER_LEVEL_DOWN_COR4_DINTLV_ICIRM_EQUAL_CFROD:
			reg_774   = 0;
			/* power down: cor4 dintlv equal */
			reg_775   = (1 << 15) | (1 << 6) | (1 << 5);
			reg_776   = 0;
			reg_899   = 0;
			reg_1280 &= 0x01ff;
			break;
		case DIB7000_POWER_LEVEL_DOWN_COR4_CRY_ESRAM_MOUT_NUD:
			reg_774   = 0;
			/* power down: cor4 */
			reg_775   = (1 << 15);
			/* nud */
			reg_776   = (1 <<  0);
			reg_899   = 0;
			reg_1280 &= 0x01ff;
			break;
#endif
	}
	
	WriteWord(774,  reg_774);
	WriteWord(775,  reg_775);
	WriteWord(776,  reg_776);
	WriteWord(899,  reg_899);
	WriteWord(1280, reg_1280);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

void Dib7000P::SetADCState(enum dibx000_adc_states no)
{
	UInt16 reg_908 = ReadWord(908),
	reg_909 = ReadWord(909);
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	switch (no) {
		case DIBX000_SLOW_ADC_ON:
			reg_909 |= (1 << 1) | (1 << 0);
			WriteWord(909, reg_909);
			reg_909 &= ~(1 << 1);
			break;
			
		case DIBX000_SLOW_ADC_OFF:
			reg_909 |=  (1 << 1) | (1 << 0);
			break;
			
		case DIBX000_ADC_ON:
			reg_908 &= 0x0fff;
			reg_909 &= 0x0003;
			break;
			
		case DIBX000_ADC_OFF: // leave the VBG voltage on
			reg_908 |= (1 << 14) | (1 << 13) | (1 << 12);
			reg_909 |= (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2);
			break;
			
		case DIBX000_VBG_ENABLE:
			reg_908 &= ~(1 << 15);
			break;
			
		case DIBX000_VBG_DISABLE:
			reg_908 |= (1 << 15);
			break;
			
		default:
			break;
	}
	
	//	dprintk( "908: %x, 909: %x\n", reg_908, reg_909);
	
	WriteWord(908, reg_908);
	WriteWord(909, reg_909);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}

int Dib7000P::SetBandwidth(UInt32 bw)
{
	UInt32 timf;
		CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	// store the current bandwidth for later use
	m_CurrentBandwidth = bw;
	
	if (m_Timf == 0) {
		CDVBLog::Log(kDVBLogDemod, "using default timf\n");
		timf = cfg.bw->timf;
	} else {
		CDVBLog::Log(kDVBLogDemod, "using updated timf\n");
		timf = m_Timf;
	}
	
	timf = timf * (bw / 50) / 160;
	
	WriteWord(23, (UInt16) ((timf >> 16) & 0xffff));
	WriteWord(24, (UInt16) ((timf      ) & 0xffff));
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib7000P::SadCalib()
{
		CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	/* internal */
	//	WriteWord(72, (3 << 14) | (1 << 12) | (524 << 0)); // sampling clock of the SAD is writting in set_bandwidth
	WriteWord(73, (0 << 1) | (0 << 0));
	WriteWord(74, 776); // 0.625*3.3 / 4096
	
	/* do the calibration */
	WriteWord(73, (1 << 0));
	WriteWord(73, (0 << 0));
	
	usleep(1000);
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib7000P::SetWBDRef(UInt16 value)
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	if (value > 4095)
		value = 4095;
	m_WBDRef = value;
	CDVBLog::Log(kDVBLogDemod, "%s done - writing word now:\n", __func__);
	return WriteWord(105, (ReadWord(105) & 0xf000) | value);
}

void Dib7000P::ResetPLL()
{
	struct dibx000_bandwidth_config *bw = &cfg.bw[0];
	UInt16 clk_cfg0;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	/* force PLL bypass */
	clk_cfg0 = (1 << 15) | ((bw->pll_ratio & 0x3f) << 9) |
	(bw->modulo << 7) | (bw->ADClkSrc << 6) | (bw->IO_CLK_en_core << 5) |
	(bw->bypclk_div << 2) | (bw->enable_refdiv << 1) | (0 << 0);
	
	WriteWord(900, clk_cfg0);
	
	/* P_pll_cfg */
	WriteWord(903, (bw->pll_prediv << 5) | (((bw->pll_ratio >> 6) & 0x3) << 3) | (bw->pll_range << 1) | bw->pll_reset);
	clk_cfg0 = (bw->pll_bypass << 15) | (clk_cfg0 & 0x7fff);
	WriteWord(900, clk_cfg0);
	
	WriteWord(18, (UInt16) (((bw->internal*1000) >> 16) & 0xffff));
	WriteWord(19, (UInt16) ( (bw->internal*1000       ) & 0xffff));
	WriteWord(21, (UInt16) ( (bw->ifreq          >> 16) & 0xffff));
	WriteWord(22, (UInt16) ( (bw->ifreq               ) & 0xffff));
	
	WriteWord(72, bw->sad_cfg);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}

int Dib7000P::ResetGPIO()
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	/* reset the GPIOs */
	CDVBLog::Log(kDVBLogDemod, "gpio dir: %x: val: %x, pwm_pos: %x\n",m_GPIODir, m_GPIOVal,cfg.m_GPIOPWMPos);
	
	WriteWord(1029, m_GPIODir);
	WriteWord(1030, m_GPIOVal);
	
	/* TODO 1031 is P_gpio_od */
	
	WriteWord(1032, cfg.m_GPIOPWMPos);
	
	WriteWord(1037, cfg.m_PWMFreqDiv);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib7000P::CfgGPIO(UInt8 num, UInt8 dir, UInt8 val)
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	m_GPIODir = ReadWord(1029);
	m_GPIODir &= ~(1 << num);         /* reset the direction bit */
	m_GPIODir |=  (dir & 0x1) << num; /* set the new direction */
	WriteWord(1029, m_GPIODir);
	
	m_GPIOVal = ReadWord(1030);
	m_GPIOVal &= ~(1 << num);          /* reset the direction bit */
	m_GPIOVal |=  (val & 0x01) << num; /* set the new value */
	WriteWord(1030, m_GPIOVal);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib7000P::SetGPIO(UInt8 num, UInt8 dir, UInt8 val)
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	return CfgGPIO(num, dir, val);
}


int Dib7000P::DemodReset()
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	SetPowerMode(DIB7000P_POWER_ALL);
	
	SetADCState(DIBX000_VBG_ENABLE);
	
	/* restart all parts */
	WriteWord(770, 0xffff);
	WriteWord(771, 0xffff);
	WriteWord(772, 0x001f);
	WriteWord(898, 0x0003);
	/* except i2c, sdio, gpio - control interfaces */
	WriteWord(1280, 0x01fc - ((1 << 7) | (1 << 6) | (1 << 5)) );
	
	WriteWord(770, 0);
	WriteWord(771, 0);
	WriteWord(772, 0);
	WriteWord(898, 0);
	WriteWord(1280, 0);
	
	/* default */
	ResetPLL();
	
	if (ResetGPIO() != 0)
		CDVBLog::Log(kDVBLogDemod, "GPIO reset was not successful.\n");
	
	if (SetOutputMode(OUTMODE_HIGH_Z) != 0)
		CDVBLog::Log(kDVBLogDemod, "OUTPUT_MODE could not be reset.\n");
	
	/* unforce divstr regardless whether i2c enumeration was done or not */
	WriteWord(1285, ReadWord(1285) & ~(1 << 1) );
	
	SetBandwidth(8000);
	
	SetADCState(DIBX000_SLOW_ADC_ON);
	SadCalib();
	SetADCState(DIBX000_SLOW_ADC_OFF);
	
	// P_iqc_alpha_pha, P_iqc_alpha_amp_dcc_alpha, ...
	if(cfg.m_TunerIsBaseband)
		WriteWord(36,0x0755);
	else
		WriteWord(36,0x1f55);
	
	WriteTab(dib7000p_defaults);
	
	SetPowerMode(DIB7000P_POWER_INTERFACE_ONLY);
	
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

UInt16 Dib7000P::ReadWord(UInt16 reg)
{
	UInt8 wb[2] = { reg >> 8, reg & 0xff };
	UInt8 rb[2];
	struct i2c_msg msg[2] = {
		{ addr : m_I2CAddress >> 1, flags : 0,        len : 2, buf : wb },
		{ addr : m_I2CAddress >> 1, flags : I2C_M_RD, len : 2, buf : rb },
	};
	
	if (m_I2CAdapter->Transfer(msg, 2) != 2)
		CDVBLog::Log(kDVBLogDemod, "i2c read error on %d\n",reg);
	return (rb[0] << 8) | rb[1];
}

static int dib7000p_read_word_static(IDVBI2CAdapter *i2c_adap, UInt8 i2c_addr, UInt16 reg)
{
	UInt8 wb[2] = { reg >> 8, reg & 0xff };
	UInt8 rb[2];
	struct i2c_msg msg[2] = {
		{ addr : i2c_addr >> 1, flags : 0,        len : 2, buf : wb },
		{ addr : i2c_addr >> 1, flags : I2C_M_RD, len : 2, buf : rb },
	};
	if (i2c_adap->Transfer(msg, 2) != 2)
		CDVBLog::Log(kDVBLogDemod, "i2c read error on %d\n", 769);
	
	return (rb[0] << 8) | rb[1];
}

int Dib7000P::WriteWord(UInt16 reg, UInt16 val)
{
	UInt8 b[4] = {
		(reg >> 8) & 0xff, reg & 0xff,
		(val >> 8) & 0xff, val & 0xff,
	};
	struct i2c_msg msg = {
		addr : m_I2CAddress >> 1, flags : 0, len : 4, buf : b
	};
	return m_I2CAdapter->Transfer(&msg, 1) != 1 ? kIOReturnIOError : 0;
}

int dib7000p_write_word_static(IDVBI2CAdapter *i2c_adap, UInt8 i2c_addr, UInt16 reg, UInt16 val)
{
	UInt8 b[4] = {
		(reg >> 8) & 0xff, reg & 0xff,
		(val >> 8) & 0xff, val & 0xff,
	};
	struct i2c_msg msg = {
		addr : i2c_addr >> 1, flags : 0, len : 4, buf : b
	};
	return i2c_adap->Transfer(&msg, 1) != 1 ? kIOReturnIOError : 0;
}

void Dib7000P::DemodAttach(IDVBI2CAdapter *i2c_adap, UInt8 i2c_addr)
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	m_I2CAdapter = i2c_adap;
	m_I2CAddress = i2c_addr;

	/* Ensure the output mode remains at the previous default if it's
	 * not specifically set by the caller.
	 */
	if ((cfg.m_OutputMode != OUTMODE_MPEG2_SERIAL) &&
	    (cfg.m_OutputMode != OUTMODE_MPEG2_PAR_GATED_CLK))
		cfg.m_OutputMode = OUTMODE_MPEG2_FIFO;
	
	if (Identify() != 0)
		goto error;

	printf("initializing I2C Master\n");
	
	m_I2CMaster = new DibX000;
	m_I2CMaster->InitI2CMaster(DIB7000P, m_I2CAdapter, m_I2CAddress);

	printf("going to demod reset\n");
	
	DemodReset();
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
error:
	return;
}

Video4Mac::IDVBI2CAdapter *Dib7000P::GetI2CMaster(enum dibx000_i2c_interface intf, int gating)
{
		CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	return m_I2CMaster->GetI2CAdapter(intf, gating);
}

int Dib7000P::Identify()
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	CDVBLog::Log(kDVBLogDemod, "checking demod on I2C address: %d (%x)\n", m_I2CAddress);

	int vendor = ReadWord(768);
	if (vendor != 0x01b3) {
		CDVBLog::Log(kDVBLogDemod, "wrong Vendor ID (read=0x%x)\n", vendor);
		return kIOReturnIOError;
	}

	int device = ReadWord(769);
	if (device != 0x4000) {
		CDVBLog::Log(kDVBLogDemod, "wrong Device ID (%x)\n", device);
		return kIOReturnIOError;
	}

	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib7000P::I2CEnumeration(IDVBI2CAdapter *i2c_adap, int no_of_demods, UInt8 default_addr)
{
	UInt8 i2c_addr;
	
	int k = 0;
	UInt8 new_addr = 0;

	m_I2CAdapter = i2c_adap;

	for (k = no_of_demods-1; k >= 0; k--) {
		new_addr		= (0x40 + k) << 1;
		m_I2CAddress	= new_addr;
		
		if (Identify() != 0) {
			m_I2CAddress = default_addr;
			if (Identify() != 0) {
				CDVBLog::Log(kDVBLogDemod, "DiB7000P #%d: not identified\n", k);
				return -EIO;
			}
		}

		SetOutputMode(OUTMODE_DIVERSITY);
		WriteWord(1285, (new_addr << 2) | 0x2);
		
		CDVBLog::Log(kDVBLogDemod, "IC %d initialized (to i2c_address 0x%x)\n", k, new_addr);
	}
	
	for (k = 0; k < no_of_demods; k++) {
		m_I2CAddress = (0x40 + k) << 1;

		WriteWord(1285, m_I2CAddress << 2);
		SetOutputMode(OUTMODE_HIGH_Z);
	}

	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
	
}

////////////////////////////////////////////////////////////////////
//
// Functions that purely support the IDVBDemod interface
//
////////////////////////////////////////////////////////////////////

void Dib7000P::PLLClockConfig()
{
	UInt16 tmp = 0;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	tmp = ReadWord(903);
	WriteWord(903, (tmp | 0x1));   //pwr-up pll
	tmp = ReadWord(900);
	WriteWord(900, (tmp & 0x7fff) | (1 << 6));     //use High freq clock
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}

void Dib7000P::RestartAGC()
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	// P_restart_iqc & P_restart_agc
	WriteWord(770, (1 << 11) | (1 << 9));
	WriteWord(770, 0x0000);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}

int Dib7000P::UpdateLNA()
{
//	UInt16 dyn_gain;
	
	// when there is no LNA to program return immediatly
//	if (m_Cfg.update_lna) {
//		// read dyn_gain here (because it is demod-dependent and not fe)
//		dyn_gain = dib7000p_read_word(state, 394);
//		if (state->cfg.update_lna(&state->demod,dyn_gain)) { // LNA has changed
//			dib7000p_restart_agc(state);
//			return 1;
//		}
//	}
	
	return 0;
}

int Dib7000P::SetAGCConfig(UInt8 band)
{
	struct dibx000_agc_config *agc = NULL;
	int i;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	if (m_CurrentBand == band && m_CurrentAGC != NULL)
		return 0;
	m_CurrentBand = band;
	
	for (i = 0; i < cfg.m_AGCConfigCount; i++)
		if (cfg.agc[i].band_caps & band) {
			agc = &cfg.agc[i];
			break;
		}
	
	if (agc == NULL) {
		CDVBLog::Log(kDVBLogDemod, "no valid AGC configuration found for band 0x%02x",band);
		return -EINVAL;
	}
	
	m_CurrentAGC = agc;
	
	/* AGC */
	WriteWord(75 ,  agc->setup );
	WriteWord(76 ,  agc->inv_gain );
	WriteWord(77 ,  agc->time_stabiliz );
	WriteWord(100, (agc->alpha_level << 12) | agc->thlock);
	
	// Demod AGC loop configuration
	WriteWord(101, (agc->alpha_mant << 5) | agc->alpha_exp);
	WriteWord(102, (agc->beta_mant << 6)  | agc->beta_exp);
	
	/* AGC continued */
	CDVBLog::Log(kDVBLogDemod, "WBD: ref: %d, sel: %d, active: %d, alpha: %d\n",
			m_WBDRef != 0 ? m_WBDRef : agc->wbd_ref, agc->wbd_sel, !agc->perform_agc_softsplit, agc->wbd_sel);
	
	if (m_WBDRef != 0)
		WriteWord(105, (agc->wbd_inv << 12) | m_WBDRef);
	else
		WriteWord(105, (agc->wbd_inv << 12) | agc->wbd_ref);
	
	WriteWord(106, (agc->wbd_sel << 13) | (agc->wbd_alpha << 9) | (agc->perform_agc_softsplit << 8));
	
	WriteWord(107,  agc->agc1_max);
	WriteWord(108,  agc->agc1_min);
	WriteWord(109,  agc->agc2_max);
	WriteWord(110,  agc->agc2_min);
	WriteWord(111, (agc->agc1_pt1    << 8) | agc->agc1_pt2);
	WriteWord(112,  agc->agc1_pt3);
	WriteWord(113, (agc->agc1_slope1 << 8) | agc->agc1_slope2);
	WriteWord(114, (agc->agc2_pt1    << 8) | agc->agc2_pt2);
	WriteWord(115, (agc->agc2_slope1 << 8) | agc->agc2_slope2);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}


int Dib7000P::AGCStartup(DVBFrontendParameters *ch)
{
	int ret = -1;
	UInt8 *agc_state = &m_AGCState;
	UInt8 agc_split;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);	
	switch (m_AGCState) {
		case 0:
			// set power-up level: interf+analog+AGC
			SetPowerMode(DIB7000P_POWER_ALL);
			SetADCState(DIBX000_ADC_ON);
			PLLClockConfig();
			
			if (SetAGCConfig(BAND_OF_FREQUENCY(ch->Frequency/1000)) != 0)
				return -1;
			
			ret = 7;
			(*agc_state)++;
			break;
			
		case 1:
			// AGC initialization
//			if (m_Cfg.agc_control)
//				m_Cfg.agc_control(&state->demod, 1);
			
			WriteWord(78, 32768);
			if (!m_CurrentAGC->perform_agc_softsplit) {
				/* we are using the wbd - so slow AGC startup */
				/* force 0 split on WBD and restart AGC */
				WriteWord(106, (m_CurrentAGC->wbd_sel << 13) | (m_CurrentAGC->wbd_alpha << 9) | (1 << 8));
				(*agc_state)++;
				ret = 5;
			} else {
				/* default AGC startup */
				(*agc_state) = 4;
				/* wait AGC rough lock time */
				ret = 7;
			}
			
			RestartAGC();
			break;
			
		case 2: /* fast split search path after 5sec */
			WriteWord(75, m_CurrentAGC->setup | (1 << 4)); /* freeze AGC loop */
			WriteWord(106, (m_CurrentAGC->wbd_sel << 13) | (2 << 9) | (0 << 8)); /* fast split search 0.25kHz */
			(*agc_state)++;
			ret = 14;
			break;
			
		case 3: /* split search ended */
			agc_split = (UInt8)ReadWord(396); /* store the split value for the next time */
			WriteWord(78, ReadWord(394)); /* set AGC gain start value */
			
			WriteWord(75,  m_CurrentAGC->setup);   /* std AGC loop */
			WriteWord(106, (m_CurrentAGC->wbd_sel << 13) | (m_CurrentAGC->wbd_alpha << 9) | agc_split); /* standard split search */
			
			RestartAGC();
			
			CDVBLog::Log(kDVBLogDemod, "SPLIT %p: %hd", this, agc_split);
			
			(*agc_state)++;
			ret = 5;
			break;
			
		case 4: /* LNA startup */
			// wait AGC accurate lock time
			ret = 7;
			
			if (UpdateLNA())
				// wait only AGC rough lock time
				ret = 5;
			else // nothing was done, go to the next state
				(*agc_state)++;
			break;
			
		case 5:
			AGCControl(0);
			(*agc_state)++;
			break;
		default:
			break;
	}
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return ret;
}

void Dib7000P::UpdateTimf()
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	UInt32 timf = (ReadWord(427) << 16) | ReadWord(428);
	m_Timf = timf * 160 / (m_CurrentBandwidth / 50);
	WriteWord(23, (UInt16) (timf >> 16));
	WriteWord(24, (UInt16) (timf & 0xffff));
	CDVBLog::Log(kDVBLogDemod, "updated timf_frequency: %d (default: %d)\n",m_Timf, cfg.bw->timf);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	
}


void Dib7000P::SetChannel(DVBFrontendParameters *ch, UInt8 seq)
{
	UInt16 value, est[4];
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);	
    SetBandwidth(BANDWIDTH_TO_KHZ(ch->u.OFDM.Bandwidth));
	
	/* nfft, guard, qam, alpha */
	value = 0;
	switch (ch->u.OFDM.TransmissionMode) {
		case TRANSMISSION_MODE_2K: value |= (0 << 7); break;
		case /* 4K MODE */ 255: value |= (2 << 7); break;
		default:
		case TRANSMISSION_MODE_8K: value |= (1 << 7); break;
	}
	switch (ch->u.OFDM.GuardInterval) {
		case GUARD_INTERVAL_1_32: value |= (0 << 5); break;
		case GUARD_INTERVAL_1_16: value |= (1 << 5); break;
		case GUARD_INTERVAL_1_4:  value |= (3 << 5); break;
		default:
		case GUARD_INTERVAL_1_8:  value |= (2 << 5); break;
	}
	switch (ch->u.OFDM.Constellation) {
		case QPSK:  value |= (0 << 3); break;
		case QAM_16: value |= (1 << 3); break;
		default:
		case QAM_64: value |= (2 << 3); break;
	}
	switch (HIERARCHY_1) {
		case HIERARCHY_2: value |= 2; break;
		case HIERARCHY_4: value |= 4; break;
		default:
		case HIERARCHY_1: value |= 1; break;
	}
	WriteWord(0, value);
	WriteWord(5, (seq << 4) | 1); /* do not force tps, search list 0 */
	
	/* P_dintl_native, P_dintlv_inv, P_hrch, P_code_rate, P_select_hp */
	value = 0;
	if (1 != 0)
		value |= (1 << 6);
	if (ch->u.OFDM.HierarchyInformation == 1)
		value |= (1 << 4);
	if (1 == 1)
		value |= 1;
	switch ((ch->u.OFDM.HierarchyInformation == 0 || 1 == 1) ? ch->u.OFDM.CodeRateHP : ch->u.OFDM.CodeRateLP) {
		case FEC_2_3: value |= (2 << 1); break;
		case FEC_3_4: value |= (3 << 1); break;
		case FEC_5_6: value |= (5 << 1); break;
		case FEC_7_8: value |= (7 << 1); break;
		default:
		case FEC_1_2: value |= (1 << 1); break;
	}
	WriteWord(208, value);
	
	/* offset loop parameters */
	WriteWord(26, 0x6680); // timf(6xxx)
	WriteWord(32, 0x0003); // pha_off_max(xxx3)
	WriteWord(29, 0x1273); // isi
	WriteWord(33, 0x0005); // sfreq(xxx5)
	
	/* P_dvsy_sync_wait */
	switch (ch->u.OFDM.TransmissionMode) {
		case TRANSMISSION_MODE_8K: value = 256; break;
		case /* 4K MODE */ 255: value = 128; break;
		case TRANSMISSION_MODE_2K:
		default: value = 64; break;
	}
	switch (ch->u.OFDM.GuardInterval) {
		case GUARD_INTERVAL_1_16: value *= 2; break;
		case GUARD_INTERVAL_1_8:  value *= 4; break;
		case GUARD_INTERVAL_1_4:  value *= 8; break;
		default:
		case GUARD_INTERVAL_1_32: value *= 1; break;
	}
	m_DivSyncWait = (value * 3) / 2 + 32; // add 50% SFN margin + compensate for one DVSY-fifo TODO
	
	/* deactive the possibility of diversity reception if extended interleaver */
	m_DivForceOff = !1 && ch->u.OFDM.TransmissionMode != TRANSMISSION_MODE_8K;
	SetDiversityIn(m_DivState);
	
	/* channel estimation fine configuration */
	switch (ch->u.OFDM.Constellation) {
		case QAM_64:
			est[0] = 0x0148;       /* P_adp_regul_cnt 0.04 */
			est[1] = 0xfff0;       /* P_adp_noise_cnt -0.002 */
			est[2] = 0x00a4;       /* P_adp_regul_ext 0.02 */
			est[3] = 0xfff8;       /* P_adp_noise_ext -0.001 */
			break;
		case QAM_16:
			est[0] = 0x023d;       /* P_adp_regul_cnt 0.07 */
			est[1] = 0xffdf;       /* P_adp_noise_cnt -0.004 */
			est[2] = 0x00a4;       /* P_adp_regul_ext 0.02 */
			est[3] = 0xfff0;       /* P_adp_noise_ext -0.002 */
			break;
		default:
			est[0] = 0x099a;       /* P_adp_regul_cnt 0.3 */
			est[1] = 0xffae;       /* P_adp_noise_cnt -0.01 */
			est[2] = 0x0333;       /* P_adp_regul_ext 0.1 */
			est[3] = 0xfff8;       /* P_adp_noise_ext -0.002 */
			break;
	}
	for (value = 0; value < 4; value++)
		WriteWord(187 + value, est[value]);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}


int Dib7000P::AutoSearchStart(DVBFrontendParameters *ch)
{
	struct DVBFrontendParameters schan;
	UInt32 value, factor;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);	
	schan = *ch;
	schan.u.OFDM.Constellation = QAM_64;
	schan.u.OFDM.GuardInterval         = GUARD_INTERVAL_1_32;
	schan.u.OFDM.TransmissionMode          = TRANSMISSION_MODE_8K;
	schan.u.OFDM.CodeRateHP  = FEC_2_3;
	schan.u.OFDM.CodeRateLP  = FEC_3_4;
	schan.u.OFDM.HierarchyInformation          = (kFEHierarchy) 0;
	
	SetChannel(&schan, 7);
	
	factor = BANDWIDTH_TO_KHZ(ch->u.OFDM.Bandwidth);
	if (factor >= 5000)
		factor = 1;
	else
		factor = 6;
	
	// always use the setting for 8MHz here lock_time for 7,6 MHz are longer
	value = 30 * cfg.bw->internal * factor;
	WriteWord(6,  (UInt16) ((value >> 16) & 0xffff)); // lock0 wait time
	WriteWord(7,  (UInt16)  (value        & 0xffff)); // lock0 wait time
	value = 100 * cfg.bw->internal * factor;
	WriteWord(8,  (UInt16) ((value >> 16) & 0xffff)); // lock1 wait time
	WriteWord(9,  (UInt16)  (value        & 0xffff)); // lock1 wait time
	value = 500 * cfg.bw->internal * factor;
	WriteWord(10, (UInt16) ((value >> 16) & 0xffff)); // lock2 wait time
	WriteWord(11, (UInt16)  (value        & 0xffff)); // lock2 wait time
	
	value = ReadWord(0);
	WriteWord(0, (UInt16) ((1 << 9) | value));
	ReadWord(1284);
	WriteWord(0, (UInt16) value);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);	
	return 0;
}

int Dib7000P::AutoSearchIsIRQ()
{
	UInt16 irq_pending = ReadWord(1284);
	CDVBLog::Log(kDVBLogDemod, "%s \n", __func__);	
	if (irq_pending & 0x1) // failed
		return 1;
	
	if (irq_pending & 0x2) // succeeded
		return 2;
	
	return 0; // still pending
}

void Dib7000P::SpurProtect(UInt32 rf_khz, UInt32 bw)
{
	static SInt16 notch[]={16143, 14402, 12238, 9713, 6902, 3888, 759, -2392};
	static UInt8 sine [] ={0, 2, 3, 5, 6, 8, 9, 11, 13, 14, 16, 17, 19, 20, 22,
		24, 25, 27, 28, 30, 31, 33, 34, 36, 38, 39, 41, 42, 44, 45, 47, 48, 50, 51,
		53, 55, 56, 58, 59, 61, 62, 64, 65, 67, 68, 70, 71, 73, 74, 76, 77, 79, 80,
		82, 83, 85, 86, 88, 89, 91, 92, 94, 95, 97, 98, 99, 101, 102, 104, 105,
		107, 108, 109, 111, 112, 114, 115, 117, 118, 119, 121, 122, 123, 125, 126,
		128, 129, 130, 132, 133, 134, 136, 137, 138, 140, 141, 142, 144, 145, 146,
		147, 149, 150, 151, 152, 154, 155, 156, 157, 159, 160, 161, 162, 164, 165,
		166, 167, 168, 170, 171, 172, 173, 174, 175, 177, 178, 179, 180, 181, 182,
		183, 184, 185, 186, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
		199, 200, 201, 202, 203, 204, 205, 206, 207, 207, 208, 209, 210, 211, 212,
		213, 214, 215, 215, 216, 217, 218, 219, 220, 220, 221, 222, 223, 224, 224,
		225, 226, 227, 227, 228, 229, 229, 230, 231, 231, 232, 233, 233, 234, 235,
		235, 236, 237, 237, 238, 238, 239, 239, 240, 241, 241, 242, 242, 243, 243,
		244, 244, 245, 245, 245, 246, 246, 247, 247, 248, 248, 248, 249, 249, 249,
		250, 250, 250, 251, 251, 251, 252, 252, 252, 252, 253, 253, 253, 253, 254,
		254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255};
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);	
	UInt32 xtal = cfg.bw->xtal_hz / 1000;
	int f_rel = ( (rf_khz + xtal/2) / xtal) * xtal - rf_khz;
	int k;
	int coef_re[8],coef_im[8];
	int bw_khz = bw;
	UInt32 pha;
	
	CDVBLog::Log(kDVBLogDemod, "relative position of the Spur: %dk (RF: %dk, XTAL: %dk)\n", f_rel, rf_khz, xtal);
	
	
	if (f_rel < -bw_khz/2 || f_rel > bw_khz/2)
		return;
	
	bw_khz /= 100;
	
	WriteWord(142 ,0x0610);
	
	for (k = 0; k < 8; k++) {
		pha = ((f_rel * (k+1) * 112 * 80/bw_khz) /1000) & 0x3ff;
		
		if (pha==0) {
			coef_re[k] = 256;
			coef_im[k] = 0;
		} else if(pha < 256) {
			coef_re[k] = sine[256-(pha&0xff)];
			coef_im[k] = sine[pha&0xff];
		} else if (pha == 256) {
			coef_re[k] = 0;
			coef_im[k] = 256;
		} else if (pha < 512) {
			coef_re[k] = -sine[pha&0xff];
			coef_im[k] = sine[256 - (pha&0xff)];
		} else if (pha == 512) {
			coef_re[k] = -256;
			coef_im[k] = 0;
		} else if (pha < 768) {
			coef_re[k] = -sine[256-(pha&0xff)];
			coef_im[k] = -sine[pha&0xff];
		} else if (pha == 768) {
			coef_re[k] = 0;
			coef_im[k] = -256;
		} else {
			coef_re[k] = sine[pha&0xff];
			coef_im[k] = -sine[256 - (pha&0xff)];
		}
		
		coef_re[k] *= notch[k];
		coef_re[k] += (1<<14);
		if (coef_re[k] >= (1<<24))
			coef_re[k]  = (1<<24) - 1;
		coef_re[k] /= (1<<15);
		
		coef_im[k] *= notch[k];
		coef_im[k] += (1<<14);
		if (coef_im[k] >= (1<<24))
			coef_im[k]  = (1<<24)-1;
		coef_im[k] /= (1<<15);
		
		CDVBLog::Log(kDVBLogDemod, "PALF COEF: %d re: %d im: %d\n", k, coef_re[k], coef_im[k]);
		
		WriteWord(143, (0 << 14) | (k << 10) | (coef_re[k] & 0x3ff));
		WriteWord(144, coef_im[k] & 0x3ff);
		WriteWord(143, (1 << 14) | (k << 10) | (coef_re[k] & 0x3ff));
	}
	WriteWord(143 ,0);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
}

int Dib7000P::Tune(DVBFrontendParameters *ch)
{
	UInt16 tmp = 0;
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);	
	if (ch != NULL)
		SetChannel(ch, 0);
	else
		return -EINVAL;
	
	// restart demod
	WriteWord(770, 0x4000);
	WriteWord(770, 0x0000);
	usleep(45000);
	
	/* P_ctrl_inh_cor=0, P_ctrl_alpha_cor=4, P_ctrl_inh_isi=0, P_ctrl_alpha_isi=3, P_ctrl_inh_cor4=1, P_ctrl_alpha_cor4=3 */
	tmp = (0 << 14) | (4 << 10) | (0 << 9) | (3 << 5) | (1 << 4) | (0x3);
	if (m_SFNWorkaroundActive) {
		CDVBLog::Log(kDVBLogDemod, "SFN workaround is active\n");
		tmp |= (1 << 9);
		WriteWord(166, 0x4000); // P_pha3_force_pha_shift
	} else {
		WriteWord(166, 0x0000); // P_pha3_force_pha_shift
	}
	WriteWord(29, tmp);
	
	// never achieved a lock with that bandwidth so far - wait for osc-freq to update
	if (m_Timf == 0)
		usleep(200000);
	
	/* offset loop parameters */
	
	/* P_timf_alpha, P_corm_alpha=6, P_corm_thres=0x80 */
	tmp = (6 << 8) | 0x80;
	switch (ch->u.OFDM.TransmissionMode) {
		case TRANSMISSION_MODE_2K: tmp |= (7 << 12); break;
		case /* 4K MODE */ 255: tmp |= (8 << 12); break;
		default:
		case TRANSMISSION_MODE_8K: tmp |= (9 << 12); break;
	}
	WriteWord(26, tmp);  /* timf_a(6xxx) */
	
	/* P_ctrl_freeze_pha_shift=0, P_ctrl_pha_off_max */
	tmp = (0 << 4);
	switch (ch->u.OFDM.TransmissionMode) {
		case TRANSMISSION_MODE_2K: tmp |= 0x6; break;
		case /* 4K MODE */ 255: tmp |= 0x7; break;
		default:
		case TRANSMISSION_MODE_8K: tmp |= 0x8; break;
	}
	WriteWord(32,  tmp);
	
	/* P_ctrl_sfreq_inh=0, P_ctrl_sfreq_step */
	tmp = (0 << 4);
	switch (ch->u.OFDM.TransmissionMode) {
		case TRANSMISSION_MODE_2K: tmp |= 0x6; break;
		case /* 4K MODE */ 255: tmp |= 0x7; break;
		default:
		case TRANSMISSION_MODE_8K: tmp |= 0x8; break;
	}
	WriteWord(33,  tmp);
	
	tmp = ReadWord(509);
	if (!((tmp >> 6) & 0x1)) {
		/* restart the fec */
		tmp = ReadWord(771);
		WriteWord(771, tmp | (1 << 1));
		WriteWord(771, tmp);
		usleep(10000);
		tmp = ReadWord(509);
	}
	
	// we achieved a lock - it's time to update the osc freq
	if ((tmp >> 6) & 0x1)
		UpdateTimf();
	
	if (cfg.m_SpurProtect)
		SpurProtect(ch->Frequency/1000, BANDWIDTH_TO_KHZ(ch->u.OFDM.Bandwidth));
	
    SetBandwidth(BANDWIDTH_TO_KHZ(ch->u.OFDM.Bandwidth));
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

////////////////////////////////////////////////////////////////////
//
// Public IDVBDemod functions
//
////////////////////////////////////////////////////////////////////
int Dib7000P::GetFrontendParams(DVBFrontendParameters *fep)
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	UInt16 tps = ReadWord(463);
	
	fep->Inversion = INVERSION_AUTO;
	
	fep->u.OFDM.Bandwidth = BANDWIDTH_TO_INDEX(m_CurrentBandwidth);
	
	switch ((tps >> 8) & 0x3) {
		case 0: fep->u.OFDM.TransmissionMode = TRANSMISSION_MODE_2K; break;
		case 1: fep->u.OFDM.TransmissionMode = TRANSMISSION_MODE_8K; break;
			/* case 2: fep->u.ofdm.transmission_mode = TRANSMISSION_MODE_4K; break; */
	}
	
	switch (tps & 0x3) {
		case 0: fep->u.OFDM.GuardInterval = GUARD_INTERVAL_1_32; break;
		case 1: fep->u.OFDM.GuardInterval = GUARD_INTERVAL_1_16; break;
		case 2: fep->u.OFDM.GuardInterval = GUARD_INTERVAL_1_8; break;
		case 3: fep->u.OFDM.GuardInterval = GUARD_INTERVAL_1_4; break;
	}
	
	switch ((tps >> 14) & 0x3) {
		case 0: fep->u.OFDM.Constellation = QPSK; break;
		case 1: fep->u.OFDM.Constellation = QAM_16; break;
		case 2:
		default: fep->u.OFDM.Constellation = QAM_64; break;
	}
	
	/* as long as the frontend_param structure is fixed for hierarchical transmission I refuse to use it */
	/* (tps >> 13) & 0x1 == hrch is used, (tps >> 10) & 0x7 == alpha */
	
	fep->u.OFDM.HierarchyInformation = HIERARCHY_NONE;
	switch ((tps >> 5) & 0x7) {
		case 1: fep->u.OFDM.CodeRateHP = FEC_1_2; break;
		case 2: fep->u.OFDM.CodeRateHP = FEC_2_3; break;
		case 3: fep->u.OFDM.CodeRateHP = FEC_3_4; break;
		case 5: fep->u.OFDM.CodeRateHP = FEC_5_6; break;
		case 7:
		default: fep->u.OFDM.CodeRateHP = FEC_7_8; break;
			
	}
	
	switch ((tps >> 2) & 0x7) {
		case 1: fep->u.OFDM.CodeRateLP = FEC_1_2; break;
		case 2: fep->u.OFDM.CodeRateLP = FEC_2_3; break;
		case 3: fep->u.OFDM.CodeRateLP = FEC_3_4; break;
		case 5: fep->u.OFDM.CodeRateLP = FEC_5_6; break;
		case 7:
		default: fep->u.OFDM.CodeRateLP = FEC_7_8; break;
	}
	
	/* native interleaver: (dib7000p_read_word(state, 464) >>  5) & 0x1 */
		CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
	return 0;
}

int Dib7000P::SetFrontendParams(DVBFrontendParameters *fep)
{
	CDVBLog::Log(kDVBLogDemod, "%s\n", __func__);
	int time, ret;
	IDVBTuner* tuner;

	
	SetOutputMode(OUTMODE_HIGH_Z);
	
    /* maybe the parameter has been changed */
//	m_SFNWorkaroundActive = buggy_sfn_workaround;
	tuner = GetFrontend()->GetTuner();
	tuner->SetParams(fep);
	
	/* start up the AGC */
	m_AGCState = 0;

	do {
		time = AGCStartup(fep);
//		time = dib7000p_agc_startup(fe, fep);
		if (time != -1)
			usleep(time*1000);
	} while (time != -1);
	
	if (fep->u.OFDM.TransmissionMode	== TRANSMISSION_MODE_AUTO ||
		fep->u.OFDM.GuardInterval		== GUARD_INTERVAL_AUTO ||
		fep->u.OFDM.Constellation		== QAM_AUTO ||
		fep->u.OFDM.CodeRateHP			== FEC_AUTO) {

		int i = 800, found;
		
		AutoSearchStart(fep);
		do {
			usleep(1000);
			found = AutoSearchIsIRQ();
		} while (found == 0 && i--);
		
		CDVBLog::Log(kDVBLogDemod, "autosearch returns: %d",found);
		if (found == 0 || found == 1)
			return 0; // no channel found
		
		GetFrontend(fep);
	}
	
	ret = Tune(fep);
	
	/* make this a config parameter */
	SetOutputMode(cfg.m_OutputMode);
	CDVBLog::Log(kDVBLogDemod, "%s done\n", __func__);
    return ret;
}
