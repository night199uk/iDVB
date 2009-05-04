/*
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
* ¬¨¬© Copyright 2002 Apple Computer, Inc. All rights reserved.
*
* IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. (‚Äö√Ñ√∫Apple‚Äö√Ñ√π) in 
* consideration of your agreement to the following terms, and your use, installation, 
* modification or redistribution of this Apple software constitutes acceptance of these
* terms.  If you do not agree with these terms, please do not use, install, modify or 
* redistribute this Apple software.
*
* In consideration of your agreement to abide by the following terms, and subject to these 
* terms, Apple grants you a personal, non exclusive license, under Apple‚Äö√Ñ√¥s copyrights in this 
* original Apple software (the ‚Äö√Ñ√∫Apple Software‚Äö√Ñ√π), to use, reproduce, modify and redistribute 
* the Apple Software, with or without modifications, in source and/or binary forms; provided 
* that if you redistribute the Apple Software in its entirety and without modifications, you 
* must retain this notice and the following text and disclaimers in all such redistributions 
* of the Apple Software.  Neither the name, trademarks, service marks or logos of Apple 
* Computer, Inc. may be used to endorse or promote products derived from the Apple Software 
* without specific prior written permission from Apple. Except as expressly stated in this 
* notice, no other rights or licenses, express or implied, are granted by Apple herein, 
* including but not limited to any patent rights that may be infringed by your derivative 
* works or by other works in which the Apple Software may be incorporated.
* 
* The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES, 
* EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-
* INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE 
* SOFTWARE OR ITS USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS. 
*
* IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, 
* REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND 
* WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR 
* OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

    // This example talks to the DevaSys  <http://www.devasys.com> "USB I2C/IO,¬¨‚Ä† Interface Board"
    // It is mainly an example of how to get to talk to a USB device from userland. The files
    // in this project:
    // 
    //   main.c: All the steps to discover a USBDeviceInterface and USBInterfaceInterface
    // 
    //   printInterpretedError.c: The beginnings of a functions to decode cyptic IOReturns
    // 
    //   deva.c: Some functions to send commands to the I/O port on the Devasys board
    // 
    //   something.c: Code which does something to the USB interface found. In this case
    //                it assumes there are swicthes and indicators attached to the I/O port
    //                It reads the switched and flashes the lights.
    // 
    // This example is designed to work with the Ezloader example. The driver produced can
    // be automaically run by Ezloader. This command is invoked in a manner like:
    // 
    //   0ABF03E9 2751 1001
    // 
    // 0ABF03E9 is the name of the command run when Ezload finds a device with VID 0xABF (2751)
    // and PID 0x3E9 (1001). It also gives these as decimal paramters.
    // 
    // The functions in this file are structured so that everytime a new data object that 
    // will eventually need to be disposed of is generated a new function is called. The
    // object is disposed of immediatly on return from the function. This obviates the
    // need for lots of clean up code in exit clauses. There are a lot of different objects
    // generated in order to get to the USBInterfaceInterface object.
    // 
    // Note: You can define the symbol "VERBOSE" to get more commentry on what this is doing.
    //
    // A version of this Ap is designed to be launched for every interface in the device.
    // If a device has 3 interfaces, you should compile and run 3 copies. Each copy should
    // have a different isThisTheInterfaceYoureLookingFor function to descriminate between
    // the different interfaces.

#include <CoreFoundation/CoreFoundation.h>
#include <Video4Mac/DVB.h>
#include "dvbtune.h"

using namespace Video4Mac;


MacDVB *g_DVB;
IDVBAdapter *g_Adapter;
IDVBFrontend *g_Frontend;

void filter_and_record()
{
	IDVBDmxPESFilterParams pesFilterParams; 
	
	IDVBDmxDevFilter* filter;
	
	pesFilterParams.PID = 600;
	pesFilterParams.Input   = kDVBDmxInFrontend; 
	pesFilterParams.Output  = kDVBDmxOutTSTap; 
	pesFilterParams.PESType = kDVBDmxPESOther;
	pesFilterParams.Flags   = DMX_IMMEDIATE_START;
	filter = g_Adapter->GetFilter();
	filter->IOCtl(DMX_SET_PES_FILTER, &pesFilterParams);
	
	pesFilterParams.PID = 601;
	pesFilterParams.Input   = kDVBDmxInFrontend; 
	pesFilterParams.Output  = kDVBDmxOutTSTap; 
	pesFilterParams.PESType = kDVBDmxPESOther;
	pesFilterParams.Flags   = DMX_IMMEDIATE_START;
	filter = g_Adapter->GetFilter();
	filter->IOCtl(DMX_SET_PES_FILTER, &pesFilterParams);
	
	
	FILE *out;
	
	out = fopen("/Users/croberts/output.ts", "w");
	char buf[752*100];
	while (1)
	{
		size_t ret;
		ret = g_Adapter->Read(buf, (size_t) 752*100, (long long) 0);
		fprintf(stderr, "%d\n", ret);
		if (ret > 0)
		{
			fwrite(buf, ret, 1, out);
		}
	}
	fclose(out);
}

void free_pat_list() {
	pat_t* t=pats;
	
	while (pats!=NULL) {
		t=pats->next;
		free(pats);
		pats=t;
	}
};

int main (int argc, const char * argv[]) 
{
	
//	int vpid = 8000;
//	int apid = 8001;	
	
	/* dvbtune stuff */
	kFESpectralInversion	SpectralInversion	= INVERSION_AUTO;
	kFEModulation			Modulation			= CONSTELLATION_DEFAULT;
	kFETransmitMode			TransmissionMode	= TRANSMISSION_MODE_DEFAULT;
	kFEBandwidth			Bandwidth			= BANDWIDTH_DEFAULT;
	kFEGuardInterval		GuardInterval		= GUARD_INTERVAL_DEFAULT;
	kFECodeRate				HP_CodeRate			= HP_CODERATE_DEFAULT;
	kFESECToneMode			Tone				= (kFESECToneMode) -1;

	// Tuning stuff
	transponder_t * t;

	
	//	dvbtune -f 537833 -qam 16 -cr 3_4 
	//	dvbtune -f 537833 -qam 16 -cr 3_4 -m
	unsigned int Frequency = 578166667;
	char			pol=0;
	unsigned int srate=0;
	unsigned int diseqc = 0;
	
	int dmx_buffer_size = 256*1024;
	
	// Strongest at: 507645830
	Frequency = 506000000;
	Modulation = QAM_16;
	HP_CodeRate = FEC_3_4;
	
	
	g_DVB = new Video4Mac::MacDVB;
	g_DVB->Initialize();
	
	g_Adapter = g_DVB->GetAdapters()[0];
	if (g_Frontend != NULL)
	{
		g_Frontend = g_Adapter->GetFrontend();
		
		//sleep(10);
		//	dvb_device_open(2, &fedbdev);
		//	dvb_frontend_open(fedbdev);
		
		tune_it(0,Frequency,srate,pol,Tone,SpectralInversion,diseqc,Modulation,HP_CodeRate,TransmissionMode,GuardInterval,Bandwidth);
		
		printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<satellite>\n");
		scan_nit(0x40); /* Get initial list of transponders */
		scan_nit(0x41); /* Get initial list of transponders */
		while ((t=get_unscanned(transponders))!=NULL) {
			free_pat_list();
			fprintf(stderr,"Scanning %d%c %d\n",t->freq,t->pol,t->srate);
			tune_it(0,t->freq,t->srate,t->pol,Tone,SpectralInversion,0,Modulation,HP_CodeRate,TransmissionMode,GuardInterval,Bandwidth);
			printf("<transponder id=\"%d\" onid=\"%d\" freq=\"%05d\" srate=\"%d\" pos=\"%04x\" we_flag=\"%d\" polarity=\"%c\" modulation=\"%d\">\n",t->id,t->onid,t->freq,t->srate,t->pos,t->we_flag,t->pol,t->mod);
			t->scanned=1;
			scan_pat();
			scan_sdt();
			printf("</transponder>\n");
			scan_nit(0x40); /* See if there are any new transponders */
			scan_nit(0x41); /* See if there are any new transponders */
		}
		printf("</satellite>\n");
		
		sleep(100);
	} else {
		printf("No DVB adapters found.\n");
	}
	//	IDVBDVR* m_DVR = m_Adapter->GetDVR();
}
