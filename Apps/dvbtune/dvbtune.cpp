/*
 *  dvbtune.c
 *  Deva
 *
 *  Created by Chris Roberts on 14/03/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#include <CoreFoundation/CoreFoundation.h>
#include <Video4Mac/DVB.h>
#include "printInterpretedError.h"
#include "dvbtune.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace Video4Mac;


#define SECA_CA_SYSTEM          0x0100
#define VIACCESS_CA_SYSTEM      0x0500
#define IRDETO_CA_SYSTEM        0x0600
#define VIDEOGUARD_CA_SYSTEM    0x0900
#define BETA_CA_SYSTEM          0x1700
#define NAGRA_CA_SYSTEM         0x1800
#define CONAX_CA_SYSTEM         0x0b00


int pnr=-1;
int apid=0;
int vpid=0;


transponder_t* transponders=NULL;
int num_trans=0;

transponder_t transponder;

typedef struct _pat_t {
	int service_id;
	int pmt_pid;
	int scanned;
	struct _pat_t* next;
} pat_t;

pat_t* pats=NULL;

/* Get the first unscanned transponder (or return NULL) */
transponder_t*  get_unscanned() {
	transponder_t* t;
	
	t=transponders;
	
	while (t!=NULL) {
		if (t->scanned==0) { return(t); };
		t=t->next;
	}
	return NULL;
}

char xmlify_result[10];
char* xmlify (char c) {
	switch(c) {
		case '&': strcpy(xmlify_result,"&amp;");
			break;
		case '<': strcpy(xmlify_result,"&lt;");
			break;
		case '>': strcpy(xmlify_result,"&gt;");
			break;
		case '\"': strcpy(xmlify_result,"&quot;");
			break;
		case 0: xmlify_result[0]=0;
			break;
		default: xmlify_result[0]=c;
			xmlify_result[1]=0;
			break;
	}
	return(xmlify_result);
}

void add_transponder(transponder_t transponder) {
	transponder_t* t;
	int found;
	
	if (transponders==NULL) {
		transponders=(transponder_t*)malloc(sizeof(transponder));
		
		transponders->freq=transponder.freq;
		transponders->srate=transponder.srate;
		transponders->pol=transponder.pol;
		transponders->pos=transponder.pos;
		transponders->we_flag=transponder.we_flag;
		transponders->mod=transponder.mod;
		transponders->scanned=0;
		transponders->next=NULL;
	} else {
		t=transponders;
		found=0;
		while ((!found) && (t!=NULL)) {
			/* Some transponders appear with slightly different frequencies -
			 ignore a new transponder if it is within 3MHz of another */
			if ((abs((int)(t->freq-transponder.freq)<=3000) && (t->pol==transponder.pol))) {
				found=1;
			} else {
				t=t->next;
			}
		}
		
		if (!found) {
			t=(transponder_t*)malloc(sizeof(transponder));
			
			t->freq=transponder.freq;
			t->srate=transponder.srate;
			t->pol=transponder.pol;
			t->pos=transponder.pos;
			t->we_flag=transponder.we_flag;
			t->mod=transponder.mod;
			t->scanned=0;
			t->next=transponders;
			
			transponders=t;
		}
	}
}




/*
struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};
*/

static Boolean quitFlag = false;	// Set by the interrupt routine to stop us.

void tuneHandler(int signal)
{
	quitFlag = true;
//	URBIsRunning = 0;
	exit(0);
}

/*
void diseqc_send_msg(struct dvb_device *fd, fe_sec_voltage_t v, struct diseqc_cmd *cmd,
					 fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b)
{
	dvb_frontend_ioctl(fd, FE_SET_TONE, (void *)SEC_TONE_OFF);
	dvb_frontend_ioctl(fd, FE_SET_VOLTAGE, (void *)v);
	usleep(15 * 1000);
	dvb_frontend_ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd);
	usleep(cmd->wait * 1000);
	usleep(15 * 1000);
	dvb_frontend_ioctl(fd, FE_DISEQC_SEND_BURST, (void *)b);
	usleep(15 * 1000);
	dvb_frontend_ioctl(fd, FE_SET_TONE, (void *)t);
}

 static int do_diseqc(struct dvb_device *secfd, int sat_no, int pol, int hi_lo)
{
	struct diseqc_cmd cmd =
	{ {{0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0 };
*/	
	/* param: high nibble: reset bits, low nibble set bits,
	 * bits are: option, position, polarizaion, band
	 */
/*
	cmd.cmd.msg[3] =
	0xf0 | (((sat_no * 4) & 0x0f) | (hi_lo ? 1 : 0) | (pol ? 0 : 2));
	
	diseqc_send_msg(secfd, (fe_sec_voltage_t) pol,
					&cmd, (fe_sec_tone_mode_t) hi_lo,
					(sat_no / 4) % 2 ? SEC_MINI_B : SEC_MINI_A);
	
	return 1;
}
*/

void parse_descriptors(int info_len,unsigned char *buf) {
	int i=0;
	int descriptor_tag,descriptor_length,j,k,pid,id;
	int service_type;
	char tmp[128];
	unsigned int freq, pol, sr;
	
	while (i < info_len) {
        descriptor_tag=buf[i++];
        descriptor_length=buf[i++];
		//        printf("Found descriptor: 0x%02x - length %02d\n",descriptor_tag,descriptor_length);
        while (descriptor_length > 0) {
			switch(descriptor_tag) {
				case 0x03: // audio_stream_descriptor
					printf("<audio_info tag=\"0x03\" info=\"%02x\" />\n",buf[i]);
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0x06: // data_stream_alignmentdescriptor
					printf("<data_stream_alignment tag=\"0x06\" data=\"%02x\" />\n",buf[i]);
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0x0a: // iso_639_language_descriptor
					for (j=0;j<((descriptor_length)/4);j++) {
						printf("<iso_639 language=\"");
						if (buf[i]!=0) printf("%c",buf[i]);
						if (buf[i+1]!=0) printf("%c",buf[i+1]);
						if (buf[i+2]!=0) printf("%c",buf[i+2]);
						printf("\" type=\"%d\" />\n",buf[i+3]);
						i+=4;
						descriptor_length-=4;
					}
					break;
					
				case 0x0b: // system_clock_descriptor
					printf("<system_clock tag=\"0x0b\" data=\"%02x%02x\" />\n",buf[i],buf[i+1]);
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0x09: // ca_descriptor
					k=((buf[i]<<8)|buf[i+1]);
					switch(k&0xff00) {
						case SECA_CA_SYSTEM:
							for (j=2; j<descriptor_length; j+=15)
							{
								pid = ((buf[i+j] & 0x1f) << 8) | buf[i+j+1];
								id = (buf[i+j+2] << 8) | buf[i+j+3];
								printf("<ca_system_descriptor type=\"seca\" system_id=\"0x%04x\" ecm_pid=\"%d\" ecm_id=\"%06x\">\n",k,pid,id);
							}        
							break;
						case VIACCESS_CA_SYSTEM:
							j = 4;
							while (j < descriptor_length)
							{
								if (buf[i+j]==0x14)
								{
									pid = ((buf[i+2] & 0x1f) << 8) | buf[i+3];
									id = (buf[i+j+2] << 16) | (buf[i+j+3] << 8) | (buf[i+j+4] & 0xf0);
									printf("<ca_system_descriptor type=\"viaccess\" system_id=\"0x%04x\" ecm_pid=\"%d\" ecm_id=\"%06x\">\n",k,pid,id);
								}
								j += 2+buf[i+j+1];
							}
							break;
						case IRDETO_CA_SYSTEM:
						case BETA_CA_SYSTEM:
							pid = ((buf[i+2] & 0x1f) << 8) | buf[i+3];
							printf("<ca_system_descriptor type=\"irdeto\" system_id=\"0x%04x\" ecm_pid=\"%d\">\n",k,pid);
							break;
						case NAGRA_CA_SYSTEM:
							pid = ((buf[i+2] & 0x1f) << 8) | buf[i+3];
							printf("<ca_system_descriptor type=\"nagra\" system_id=\"0x%04x\" ecm_pid=\"%d\">\n",k,pid);
							break;
						case CONAX_CA_SYSTEM:
							pid = ((buf[i+2] & 0x1f) << 8) | buf[i+3];
							printf("<ca_system_descriptor type=\"conax\" system_id=\"0x%04x\" ecm_pid=\"%d\">\n",k,pid);
							break;
						case VIDEOGUARD_CA_SYSTEM:
							pid = ((buf[i+2] & 0x1f) << 8) | buf[i+3];
							printf("<ca_system_descriptor type=\"videoguard\" system_id=\"0x%04x\" ecm_pid=\"%d\">\n",k,pid);
							break;
						default:
							pid = ((buf[i+2] & 0x1f) << 8) | buf[i+3];
							printf("<ca_system_descriptor type=\"unknown\" system_id=\"0x%04x\">\n",k);
							break;
					}
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0x40: // network_name
					//             printf("<network_name tag=\"0x40\">");
					j=descriptor_length;
					while(j > 0) {
						//               printf("%c",buf[i++]);
						j--;
					}
					descriptor_length=0;
					//             printf("</network_name>\n");
					break;
					
				case 0x41: // service_list
					//             printf("<services tag=\"0x41\" n=\"%d\">\n",descriptor_length/3);
					while (descriptor_length > 0) {
						//               printf("<service id=\"%d\" type=\"%d\" />\n",(buf[i]<<8)|buf[i+1],buf[i+2]);
						i+=3;
						descriptor_length-=3;
					}
					//             printf("</services>\n");
					break;
					
				case 0x43: // satellite_delivery_system
					freq=(unsigned int)(buf[i]<<24)|(buf[i+1]<<16)|(buf[i+2]<<8)|buf[i+3];
					sprintf(tmp,"%x",freq);
					transponder.freq=atoi(tmp)*10;
					i+=4;
					transponder.pos=(buf[i]<<8)|buf[i+1];
					i+=2;
					transponder.we_flag=(buf[i]&0x80)>>7;
					pol=(buf[i]&0x60)>>5;
					switch(pol) {
						case 0 : transponder.pol='H'; break;
						case 1 : transponder.pol='V'; break;
						case 2 : transponder.pol='L'; break;
						case 3 : transponder.pol='R'; break;
					}
					transponder.mod=buf[i]&0x1f;
					i++;
					sr=(unsigned int)(buf[i]<<24)|(buf[i+1]<<16)|(buf[i+2]<<8)|(buf[i+3]&0xf0);
					sr=(unsigned int)(sr >> 4);
					sprintf(tmp,"%x",sr);
					transponder.srate=atoi(tmp)*100;
					i+=4;
					descriptor_length=0;
					add_transponder(transponder);
					//             printf("<satellite_delivery tag=\"0x43\" freq=\"%05d\" srate=\"%d\" pos=\"%04x\" we_flag=\"%d\" polarity=\"%c\" modulation=\"%d\" />\n",transponder.freq,transponder.srate,transponder.pos,transponder.we_flag,transponder.pol,transponder.mod);
					break;
					
				case 0x48: // service_description
					service_type=buf[i++];
					printf("<description tag=\"0x48\" type=\"%d\"",service_type);
					descriptor_length--;
					j=buf[i++];
					descriptor_length-=(j+1);
					printf(" provider_name=\"");;
					while(j > 0) {
						printf("%s",xmlify(buf[i++]));
						j--;
					}
					printf("\" service_name=\"");
					j=buf[i++]; 
					descriptor_length-=(j+1);
					while(j > 0) {
						printf("%s",xmlify(buf[i]));
						i++;
						j--;
					}
					printf("\" />\n");
					break;
					
				case 0x49: // country_availability:
					printf("<country_availability tag=\"0x49\" type=\"%d\" countries=\" ",(buf[i]&0x80)>>7);
					i++;
					j=descriptor_length-1;
					while (j > 0) { 
						printf("%c",buf[i++]);
						j--;
					}
					printf("\" />\n");
					descriptor_length=0;
					break;
					
				case 0x4c:
					printf("<time_shifted_copy_of tag=\"0x4c\" service_id=\"%d\" />\n",(buf[i]<<8)|buf[i+1]);
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0x52: // stream_identifier_descriptor
					printf("<stream_id id=\"%d\" />\n",buf[i]);
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0x53:
					printf("<ca_identifier tag=\"0x53\" length=\"%02x\">\n",descriptor_length);
					for (j=0;j<descriptor_length;j+=2) {
						k=(buf[i+j]<<8)|buf[i+j+1];
						printf("<ca_system_id>%04x</ca_system_id>\n",k);
					}
					i+=descriptor_length;
					descriptor_length=0;
					printf("</ca_identifier>\n");
					break;
					
				case 0x56:
					j=0;
					printf("<teletext tag=\"0x56\">\n");
					while (j < descriptor_length) {
						printf("<teletext_info lang=\"");
						printf("%s",xmlify(buf[i]));
						printf("%s",xmlify(buf[i+1]));
						printf("%s",xmlify(buf[i+2]));
						k=(buf[i+3]&0x07);
						printf("\" type=\"%d\" page=\"%d%02x\" />\n",(buf[i+3]&0xf8)>>3,(k==0 ? 8 : k),buf[i+4]);
						i+=5;
						j+=5;
					}
					printf("</teletext>\n");
					descriptor_length=0;
					break;
					
				case 0x59:
					j=0;
					printf("<subtitling_descriptor tag=\"0x59\">\n");
					while (j < descriptor_length) {
						printf("<subtitle_stream lang=\"");
						printf("%s",xmlify(buf[i]));
						printf("%s",xmlify(buf[i+1]));
						printf("%s",xmlify(buf[i+2]));
						printf("\" type=\"%d\" composition_page_id=\"%04x\" ancillary_page_id=\"%04x\" />\n",buf[i+3],(buf[i+4]<<8)|buf[i+5],(buf[i+6]<<8)|buf[i+7]);
						i+=8;
						j+=8;
					}
					printf("</subtitling_descriptor>\n");
					descriptor_length=0;
					break;
					
				case 0x6a:
					printf("<ac3_descriptor tag=\"0x6a\" data=\"");
					for (j=0;j<descriptor_length;j++) printf("%02x",buf[i+j]);
					printf("\" />\n");
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				case 0xc5: // canal_satellite_radio_descriptor
					/* This is guessed from the data */
					printf("<canal_radio tag=\"0x%02x\" id=\"%d\" name=\"",descriptor_tag,buf[i]);
					i++;
					for (j=0;j<descriptor_length;j++) 
						if (buf[i+j]!=0) printf("%c",buf[i+j]);
					printf("\" />\n");
					i+=descriptor_length;
					descriptor_length=0;
					break;
					
				default:
					printf("<descriptor tag=\"0x%02x\" data=\"",descriptor_tag);
					for (j=0;j<descriptor_length;j++) printf("%02x",buf[i+j]);
					printf("\" text=\"");
					for (j=0;j<descriptor_length;j++) printf("%c",(isalnum(buf[i+j]) ? buf[i+j] : '.'));
					printf("\" />\n");
					i+=descriptor_length;
					descriptor_length=0;
					break;
			}
        }
	}
}

int scan_nit(int x) {
	int fd_nit;
	int n,seclen;
	int i;
	unsigned char buf[4096];
	IDVBDmxSctFilterParams sctFilterParams;
	int info_len,network_id;
	DVBPollDescriptor ufd;
	IDVBEventListener listener;
	
	IDVBDmxDevFilter* filter;
	filter = g_Adapter->GetFilter();
	
	sctFilterParams.PID=0x10;
	memset(&sctFilterParams.Filter,0,sizeof(sctFilterParams.Filter));
	sctFilterParams.Timeout = 0;
	sctFilterParams.Flags = DMX_IMMEDIATE_START;
	sctFilterParams.Filter.Filter[0]=x;
	sctFilterParams.Filter.Mask[0]=0xff;
	
	if (filter->IOCtl(DMX_SET_FILTER,&sctFilterParams) < 0) {
		perror("NIT - DMX_SET_FILTER:");
		close(fd_nit);
		return -1;
	}
	
	ufd.Source=filter;
	ufd.Events=POLLPRI;
	if (listener.Poll(&ufd,1,10000) < 0 ) {
		fprintf(stderr,"TIMEOUT on read from fd_nit\n");
		close(fd_nit);
		return -1;
	}
	if (read(fd_nit,buf,3)==3) {
		seclen=((buf[1] & 0x0f) << 8) | (buf[2] & 0xff);
		n = read(fd_nit,buf+3,seclen);
		if (n==seclen) {
			seclen+=3;
			//      dump("nit.dat",seclen,buf);
			//      printf("<nit>\n");
			network_id=(buf[3]<<8)|buf[4];
			//      printf("<network id=\"%d\">\n",network_id);
			
			info_len=((buf[8]&0x0f)<<8)|buf[9];
			i=10;
			parse_descriptors(info_len,&buf[i]);
			i+=info_len;
			i+=2;
			while (i < (seclen-4)) {
				transponder.id=(buf[i]<<8)|buf[i+1];
				i+=2;
				transponder.onid=(buf[i]<<8)|buf[i+1];
				i+=2;
				//        printf("<transponder id=\"%d\" onid=\"%d\">\n",transponder.id,transponder.onid);
				info_len=((buf[i]&0x0f)<<8)|buf[i+1];
				i+=2;
				parse_descriptors(info_len,&buf[i]);
				//        printf("</transponder>\n");
				i+=info_len;
			}
			//      printf("</network>\n");
			//      printf("</nit>\n");
		} else {
			fprintf(stderr,"Under-read bytes for NIT - wanted %d, got %d\n",seclen,n);
		}
	} else {
		fprintf(stderr,"Nothing to read from fd_nit\n");
	}
	close(fd_nit);
	return(0);
}

void scan_pmt(int pid,int sid,int change) {
	int fd_pmt;
	int n,seclen;
	int i;
	unsigned char buf[4096];
	IDVBDmxSctFilterParams sctFilterParams;
	int service_id;
	int info_len,es_pid,stream_type;
	DVBPollDescriptor ufd;
	IDVBEventListener listener;
	
	//  printf("Scanning pmt: pid=%d, sid=%d\n",pid,sid);
	
	if (pid==0) { return; }
	
	IDVBDmxDevFilter* filter;
	filter = g_Adapter->GetFilter();
	
	sctFilterParams.PID=pid;
	memset(&sctFilterParams.Filter,0,sizeof(sctFilterParams.Filter));
	sctFilterParams.Timeout = 0;
	sctFilterParams.Flags = DMX_IMMEDIATE_START;
	sctFilterParams.Filter.Filter[0]=0x02;
	sctFilterParams.Filter.Mask[0]=0xff;
	filter->IOCtl(DMX_SET_FILTER, &sctFilterParams);

	ufd.Source=filter;
	ufd.Events=POLLPRI;
	if (listener.Poll(&ufd,1,10000) < 0) {
		fprintf(stderr,"TIMEOUT reading from fd_pmt\n");
		close(fd_pmt);
		return;
	}
	
	if (read(fd_pmt,buf,3)==3) {
		seclen=((buf[1] & 0x0f) << 8) | (buf[2] & 0xff);
		n = read(fd_pmt,buf+3,seclen);
		if (n==seclen) {
			seclen+=3;
			//      printf("<pmt>\n");
			service_id=(buf[3]<<8)|buf[4];
			//      printf("<service id=\"%d\" pmt_pid=\"%d\">\n",service_id,pid);
			
			if (sid != service_id) {
				close(fd_pmt);
				scan_pmt(pid, sid, change);
				return;
			}
			
			info_len=((buf[10]&0x0f)<<8)|buf[11];
			i=12;
			parse_descriptors(info_len,&buf[i]);
			i+=info_len;
			while (i < (seclen-4)) {
				stream_type=buf[i++];
				es_pid=((buf[i]&0x1f)<<8)|buf[i+1];
				printf("<stream type=\"%d\" pid=\"%d\">\n",stream_type,es_pid);
				if (change) {
					if ((vpid==0) && ((stream_type==1) || (stream_type==2))) {
						vpid=es_pid;
					}
					if ((apid==0) && ((stream_type==3) || (stream_type==4))) {
						apid=es_pid;
					}
				}
				
				i+=2;
				info_len=((buf[i]&0x0f)<<8)|buf[i+1];
				i+=2;
				parse_descriptors(info_len,&buf[i]);
				i+=info_len;
				printf("</stream>\n");
			}
			//      printf("</service>\n");
			//      printf("</pmt>\n");
		} else {
			printf("Under-read bytes for PMT - wanted %d, got %d\n",seclen,n);
		}
	} else {
		fprintf(stderr,"Nothing to read from fd_pmt\n");
	}
	
	close(fd_pmt);
}

 
void print_status(FILE* fd,kFEStatus festatus) {
	fprintf(fd,"-> %08x %d", (int) festatus, (int) festatus);
	fprintf(fd,"FE_STATUS:");
	if (festatus & FE_HAS_SIGNAL) fprintf(fd," FE_HAS_SIGNAL");
	if (festatus & FE_TIMEDOUT) fprintf(fd," FE_TIMEDOUT");
	if (festatus & FE_HAS_LOCK) fprintf(fd," FE_HAS_LOCK");
	if (festatus & FE_HAS_CARRIER) fprintf(fd," FE_HAS_CARRIER");
	if (festatus & FE_HAS_VITERBI) fprintf(fd," FE_HAS_VITERBI");
	if (festatus & FE_HAS_SYNC) fprintf(fd," FE_HAS_SYNC");
	fprintf(fd,"\n");
}

int check_status(IDVBFrontend *fd_frontend,struct DVBFrontendParameters* feparams,int tone) {
	int err;
	int i, res, status;
	unsigned int original_freq;
	DVBFrontendEvent event;
	IDVBEventListener listener;
	DVBPollDescriptor pollfd[1];
	
	if (fd_frontend->SetFrontend(feparams) < 0) {
		perror("ERROR tuning channel\n");
		return -1;
	}
	
	pollfd[0].Source = fd_frontend;
	pollfd[0].Events = POLLIN;
	event.status = (kFEStatus) 0;
	while (((event.status & FE_TIMEDOUT) == 0) && ((event.status & FE_HAS_LOCK) == 0)) {
		fprintf(stderr, "polling.....\n");
		if (listener.Poll(pollfd, 1, 10000))
		{
			if ((status = fd_frontend->GetEvent(&event, O_NONBLOCK) < 0))
			{
				if (status != -EOVERFLOW)
				{
					perror("FE_GET_EVENT");
					fprintf(stderr, "status = %d\n err = %d\n", status, err);
					return -1;
				} else {
					fprintf(stderr, "Overflow error, trying again (status = %d, errno = %d)\n", status, err);
				}
			}
			print_status(stderr, event.status);
			
		}
	}
	
	
}
	
int tune_it(int fd_sec, 
			unsigned int freq, unsigned int srate, 
			char pol, kFESECToneMode tone, kFESpectralInversion specInv,
			unsigned int diseqc, kFEModulation modulation,
			kFECodeRate HP_CodeRate, kFETransmitMode TransmissionMode,
			kFEGuardInterval guardInterval, kFEBandwidth bandwidth) {

	int res;
	DVBFrontendParameters feparams;
	DVBFrontendInfo fe_info;
	DVBFrontendEvent event;
	DVBPollDescriptor pollfd;
	IDVBEventListener listener;
	kFESECVoltage voltage;
	
	pollfd.Source = g_Frontend;
	pollfd.Events = POLLIN;
	if (res = listener.Poll(&pollfd, 1, 500))
	{
		while (1)
		{
			if(res = g_Frontend->GetEvent(&event, O_NONBLOCK) < 0) 
			{ 
				break; 
			} else {
				fprintf(stderr, "res: %d, should be: %d\n", res, -EWOULDBLOCK);
			}
		}
	}

	if ((res = g_Frontend->GetInfo(&fe_info) < 0)){
		perror("FE_GET_INFO: \n");
		return -1;
	}
	
	fprintf(stderr,"Using DVB card \"%s\"\n",fe_info.Name);

	switch(fe_info.Type) {
		case FE_OFDM:
			if (freq < 1000000) freq*=1000UL;
			feparams.Frequency=freq;
			feparams.Inversion=INVERSION_AUTO;
			feparams.u.OFDM.Bandwidth=bandwidth;
			feparams.u.OFDM.CodeRateHP=HP_CodeRate;
			feparams.u.OFDM.CodeRateLP=(kFECodeRate) LP_CODERATE_DEFAULT;
			feparams.u.OFDM.Constellation=modulation;
			feparams.u.OFDM.TransmissionMode=TransmissionMode;
			feparams.u.OFDM.GuardInterval=guardInterval;
			feparams.u.OFDM.HierarchyInformation=HIERARCHY_DEFAULT;
			fprintf(stderr,"tuning DVB-T (%s) to %d Hz\n",DVB_T_LOCATION,freq);
			break;
		case FE_QPSK:
			fprintf(stderr,"tuning DVB-S to L-Band:%d, Pol:%c Srate=%d, 22kHz=%s\n",feparams.Frequency,pol,srate,tone == SEC_TONE_ON ? "on" : "off");			
			if ((pol=='h') || (pol=='H')) {
				voltage = SEC_VOLTAGE_18;
			} else {
				voltage = SEC_VOLTAGE_13;
			}
			if (diseqc==0) if (g_Frontend->SetVoltage(voltage) < 0) {
					perror("ERROR setting voltage\n");
				}
				
				if (freq > 2200000) {
					// this must be an absolute frequency
					if (freq < SLOF) {
						feparams.Frequency=(freq-LOF1);
						if (tone < 0) tone = SEC_TONE_OFF;
					} else {
						feparams.Frequency=(freq-LOF2);
						if (tone < 0) tone = SEC_TONE_ON;
					}
				} else {
					// this is an L-Band frequency
					feparams.Frequency=freq;
				}
				
				feparams.Inversion=specInv;
				feparams.u.QPSK.SymbolRate=srate;
				feparams.u.QPSK.FECInner=FEC_AUTO;
				
				if (diseqc==0) {
					if (g_Frontend->SetTone(tone) < 0) {
						perror("ERROR setting tone\n");
					}
				}
			
			if (diseqc > 0) {
//				do_diseqc(fd_frontend, diseqc-1,voltage,tone);
				sleep(1);
			}
			break;
		case FE_QAM:
			fprintf(stderr,"tuning DVB-C to %d, srate=%d\n",freq,srate);
			feparams.Frequency=freq;
			feparams.Inversion=INVERSION_OFF;
			feparams.u.QAM.SymbolRate = srate;
			feparams.u.QAM.FECInner = FEC_AUTO;
			feparams.u.QAM.Modulation = modulation;
			break;
		default:
			fprintf(stderr,"Unknown FE type. Aborting\n");
			exit(-1);
	}
	usleep(100000);
	
	return check_status(g_Frontend,&feparams,tone);
	
}