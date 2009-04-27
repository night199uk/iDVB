/*
 *  DibX000.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 09/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include "IDVBI2CAdapter.h"
#include "IDVBI2CClient.h"

enum dibx000_i2c_interface {
	DIBX000_I2C_INTERFACE_TUNER    = 0,
	DIBX000_I2C_INTERFACE_GPIO_1_2 = 1,
	DIBX000_I2C_INTERFACE_GPIO_3_4 = 2
};

#define DIB3000MC 1
#define DIB7000   2
#define DIB7000P  11
#define DIB7000MC 12
		
namespace Dibcom
{
	class DibX000 : public Video4Mac::IDVBI2CAdapter, public Video4Mac::IDVBI2CClient
	{
	public:
		/* virtual */ int			Transfer(struct i2c_msg *msg, int num);
		int							InitI2CMaster(UInt16 device_rev, IDVBI2CAdapter *i2c_adap, UInt8 i2c_addr);
		IDVBI2CAdapter*				GetI2CAdapter(enum dibx000_i2c_interface intf, int gating);
		void						ExitI2CMaster();
	
	private:
		// struct i2c_adapter  gated_tuner_i2c_adap;
		int							WriteWord(UInt16 reg, UInt16 val);
		int							I2CGateControl(UInt8 tx[4], UInt8 addr, int onoff);
		int							I2CSelectInterface(enum dibx000_i2c_interface intf);
		Video4Mac::IDVBI2CAdapter*	m_GatedTunerI2CAdapter;
		UInt16						m_BaseReg;
		enum dibx000_i2c_interface	m_SelectedInterface;
		UInt16						m_DeviceRev;
	};
	
}

/*
extern int dibx000_init_i2c_master(struct dibx000_i2c_master *mst, UInt16 device_rev, struct i2c_adapter *i2c_adap, UInt8 i2c_addr);
extern struct i2c_adapter * dibx000_get_i2c_adapter(struct dibx000_i2c_master *mst, enum dibx000_i2c_interface intf, int gating);
//extern void dibx000_exit_i2c_master(struct dibx000_i2c_master *mst);
*/

#define BAND_LBAND 0x01
#define BAND_UHF   0x02
#define BAND_VHF   0x04
#define BAND_SBAND 0x08
#define BAND_FM	   0x10

#define BAND_OF_FREQUENCY(freq_kHz) ( (freq_kHz) <= 115000 ? BAND_FM : \
(freq_kHz) <= 250000 ? BAND_VHF : \
(freq_kHz) <= 863000 ? BAND_UHF : \
(freq_kHz) <= 2000000 ? BAND_LBAND : BAND_SBAND )

struct dibx000_agc_config {
	/* defines the capabilities of this AGC-setting - using the BAND_-defines*/
	UInt8  band_caps;
	
	UInt16 setup;
	
	UInt16 inv_gain;
	UInt16 time_stabiliz;
	
	UInt8  alpha_level;
	UInt16 thlock;
	
	UInt8  wbd_inv;
	UInt16 wbd_ref;
	UInt8 wbd_sel;
	UInt8 wbd_alpha;
	
	UInt16 agc1_max;
	UInt16 agc1_min;
	UInt16 agc2_max;
	UInt16 agc2_min;
	
	UInt8 agc1_pt1;
	UInt8 agc1_pt2;
	UInt8 agc1_pt3;
	
	UInt8 agc1_slope1;
	UInt8 agc1_slope2;
	
	UInt8 agc2_pt1;
	UInt8 agc2_pt2;
	
	UInt8 agc2_slope1;
	UInt8 agc2_slope2;
	
	UInt8 alpha_mant;
	UInt8 alpha_exp;
	
	UInt8 beta_mant;
	UInt8 beta_exp;
	
	UInt8 perform_agc_softsplit;
	
	struct {
		UInt16 min;
		UInt16 max;
		UInt16 min_thres;
		UInt16 max_thres;
	} split;
};

struct dibx000_bandwidth_config {
	UInt32   internal;
	UInt32   sampling;
	
	UInt8 pll_prediv;
	UInt8 pll_ratio;
	UInt8 pll_range;
	UInt8 pll_reset;
	UInt8 pll_bypass;
	
	UInt8 enable_refdiv;
	UInt8 bypclk_div;
	UInt8 IO_CLK_en_core;
	UInt8 ADClkSrc;
	UInt8 modulo;
	
	UInt16 sad_cfg;
	
	UInt32 ifreq;
	UInt32 timf;
	
	UInt32 xtal_hz;
};

enum dibx000_adc_states {
	DIBX000_SLOW_ADC_ON = 0,
	DIBX000_SLOW_ADC_OFF,
	DIBX000_ADC_ON,
	DIBX000_ADC_OFF,
	DIBX000_VBG_ENABLE,
	DIBX000_VBG_DISABLE,
};

#define BANDWIDTH_TO_KHZ(v) ( (v) == BANDWIDTH_8_MHZ  ? 8000 : \
(v) == BANDWIDTH_7_MHZ  ? 7000 : \
(v) == BANDWIDTH_6_MHZ  ? 6000 : 8000 )

#define BANDWIDTH_TO_INDEX(v) ( \
(v) == 8000 ? BANDWIDTH_8_MHZ : \
(v) == 7000 ? BANDWIDTH_7_MHZ : \
(v) == 6000 ? BANDWIDTH_6_MHZ : BANDWIDTH_8_MHZ )

/* Chip output mode. */
#define OUTMODE_HIGH_Z              0
#define OUTMODE_MPEG2_PAR_GATED_CLK 1
#define OUTMODE_MPEG2_PAR_CONT_CLK  2
#define OUTMODE_MPEG2_SERIAL        7
#define OUTMODE_DIVERSITY           4
#define OUTMODE_MPEG2_FIFO          5
#define OUTMODE_ANALOG_ADC          6

