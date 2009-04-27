/*
 *  CDVBUSBDemux.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <IDVBUSBadapter.h>
#include <IDVBDemuxFeed.h>
#include <IDVBAdapter.h>
#include <CDVBUSBDemux.h>
#include <CDVBLog.h>

using namespace Video4Mac;

CDVBUSBDemux::CDVBUSBDemux(IDVBAdapter* Adapter, int FilterNum, int FeedNum) : IDVBDemux(Adapter, FilterNum, FeedNum)
{
	fprintf(stderr, "CDVBUSBDemux: %p\n", this);
	CDVBLog::Log(kDVBLogDemux, "CDVBUSBDemux: %p\n", this);
}

CDVBUSBDemux::~CDVBUSBDemux()
{
	CDVBLog::Log(kDVBLogDemux, "CDVBUSBDemux destructor: %p\n", this);	
}

int CDVBUSBDemux::ControlFeed(IDVBDemuxFeed* dvbdmxfeed, int onoff)
{
//    struct dvb_usb_adapter *adap = (struct dvb_usb_adapter *)dvbdmxfeed->demux->priv;
//    int newfeedcount,ret;
	
//    if (adap == NULL)
//        return -ENODEV;
	
//    newfeedcount = adap->feedcount + (onoff ? 1 : -1);
//	
//    /* stop feed before setting a new pid if there will be no pid anymore */
//    if (newfeedcount == 0) {
//        printf("stop feeding\n");
//        usb_urb_kill(&adap->stream);
//		
//        if (adap->props.streaming_ctrl != NULL)
//            if ((ret = adap->props.streaming_ctrl(adap,0)))
//                err("error while stopping stream.");
//    }
	
//    adap->feedcount = newfeedcount;
	
    /* activate the pid on the device specific pid_filter */
//    printf("setting pid (%s): %5d %04x at index %d '%s'\n",adap->pid_filtering ?
//		   "yes" : "no", dvbdmxfeed->pid,dvbdmxfeed->pid,dvbdmxfeed->index,onoff ?
//		   "on" : "off");
//    if (adap->props.caps & DVB_USB_ADAP_HAS_PID_FILTER &&
//        adap->pid_filtering &&
//        adap->props.pid_filter != NULL)
//        adap->props.pid_filter(adap, dvbdmxfeed->index, dvbdmxfeed->pid,onoff);
	
    /* start the feed if this was the first feed and there is still a feed
     * for reception.
     */
	IDVBUSBAdapter* adap = (IDVBUSBAdapter *) GetAdapter();
	adap->USBURBSubmit();
	
//    if (adap->feedcount == onoff && adap->feedcount > 0) {
//        printf("submitting all URBs\n");
//        usb_urb_submit(&adap->stream);
		
//        printf("controlling pid parser\n");
//        if (adap->props.caps & DVB_USB_ADAP_HAS_PID_FILTER &&
//            adap->props.caps & DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF &&
//            adap->props.pid_filter_ctrl != NULL)
//            if (adap->props.pid_filter_ctrl(adap,adap->pid_filtering) < 0)
//                err("could not handle pid_parser");
	adap->StreamingControl(1);
//        if (adap->props.streaming_ctrl != NULL)
//            if (adap->props.streaming_ctrl(adap,1)) {
//                err("error while enabling fifo.");
//                return -ENODEV;
//            }
//		
//    }
//    return 0;
}

int CDVBUSBDemux::StartFeed(IDVBDemuxFeed* dvbdmxfeed)
{
	CDVBLog::Log(kDVBLogDemux, "start pid: 0x%04x, feedtype: %d\n", dvbdmxfeed->GetPID(),dvbdmxfeed->GetType());
    return ControlFeed(dvbdmxfeed, 1);
}

int CDVBUSBDemux::StopFeed(IDVBDemuxFeed* dvbdmxfeed)
{
    CDVBLog::Log(kDVBLogDemux, "stop pid: 0x%04x, feedtype: %d\n", dvbdmxfeed->GetPID(), dvbdmxfeed->GetType());
    return ControlFeed(dvbdmxfeed, 0);
}
