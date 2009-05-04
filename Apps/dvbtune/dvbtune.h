/*
 *  dvbtune.h
 *  Deva
 *
 *  Created by Chris Roberts on 14/03/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <Video4Mac/DVB.h>

using namespace Video4Mac;

extern MacDVB *g_DVB;
extern IDVBAdapter *g_Adapter;
extern IDVBFrontend *g_Frontend;

#define SLOF (11700*1000UL)
#define LOF1 (9750*1000UL)
#define LOF2 (10600*1000UL)

#define DVB_T_LOCATION          "in United Kingdom"
#define BANDWIDTH_DEFAULT           BANDWIDTH_8_MHZ
#define HP_CODERATE_DEFAULT         FEC_3_4
#define CONSTELLATION_DEFAULT       QAM_16
#define TRANSMISSION_MODE_DEFAULT   TRANSMISSION_MODE_2K
#define GUARD_INTERVAL_DEFAULT      GUARD_INTERVAL_1_32
#define HIERARCHY_DEFAULT           HIERARCHY_NONE
#if HIERARCHY_DEFAULT == HIERARCHY_NONE && !defined (LP_CODERATE_DEFAULT)
#define LP_CODERATE_DEFAULT (0) /* unused if HIERARCHY_NONE */
#endif

//#define LP_CODERATE_DEFAULT FEC_3_4


typedef struct _transponder_t {
	int id;
	int onid;
	unsigned int freq;
	int srate;
	int pos;
	int we_flag;
	char pol;
	int mod;
	
	int scanned;
	struct _transponder_t* next;
} transponder_t;

typedef struct _pat_t {
	int service_id;
	int pmt_pid;
	int scanned;
	struct _pat_t* next;
} pat_t;


extern int tune_it(int fd_sec, 
			unsigned int freq, unsigned int srate, 
			char pol, kFESECToneMode tone, kFESpectralInversion specInv,
			unsigned int diseqc, kFEModulation modulation,
			kFECodeRate HP_CodeRate, kFETransmitMode TransmissionMode,
			kFEGuardInterval guardInterval, kFEBandwidth bandwidth);

extern void scan_pmt(int pid,int sid,int change);
extern void scan_pat();
extern int scan_nit(int x);
extern void scan_sdt();
extern transponder_t* transponders;
extern transponder_t transponder;
extern pat_t* pats;

/* Get the first unscanned transponder (or return NULL) */
extern transponder_t*  get_unscanned(transponder_t* t);