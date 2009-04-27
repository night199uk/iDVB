/*
 *  IDVBDmxPublic.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#pragma once

#define DMX_FILTER_SIZE 16

namespace Video4Mac
{

	
	// dmx_output_t
	typedef enum
	{
		kDVBDmxOutDecoder,		/* Streaming directly to decoder. */
		kDVBDmxOutTap,			/* Output going to a memory buffer */
								/* (to be retrieved via the read command).*/
		kDVBDmxOutTSTap,		/* Output multiplexed into a new TS  */
								/* (to be retrieved by reading from the */
								/* logical DVR device).                 */
		kDVBDmxOutTSDemuxTap	/* Like TS_TAP but retrieved from the DMX device */
	} kDVBDmxOutput;
	

	typedef enum
	{
		kDVBDmxPESAudio0,
		kDVBDmxPESVideo0,
		kDVBDmxPESTeletext0,
		kDVBDmxPESSubtitle0,
		kDVBDmxPESPCR0,
		
		kDVBDmxPESAudio1,
		kDVBDmxPESVideo1,
		kDVBDmxPESTeletext1,
		kDVBDmxPESSubtitle1,
		kDVBDmxPESPCR1,
		
		kDVBDmxPESAudio2,
		kDVBDmxPESVideo2,
		kDVBDmxPESTeletext2,
		kDVBDmxPESSubtitle2,
		kDVBDmxPESPCR2,
		
		kDVBDmxPESAudio3,
		kDVBDmxPESVideo3,
		kDVBDmxPESTeletext3,
		kDVBDmxPESSubtitle3,
		kDVBDmxPESPCR3,
		
		kDVBDmxPESOther
	} kDVBDmxPESType;
	
	
	// dmx_input_t
	typedef enum
	{
		kDVBDmxInFrontend, /* Input from a front-end device.  */
		kDVBDmxInDVR       /* Input from the logical DVR device.  */
	} kDVBDmxInput;
	
	typedef struct IDVBDmxFilter
	{
		UInt8  Filter[DMX_FILTER_SIZE];
		UInt8  Mask[DMX_FILTER_SIZE];
		UInt8  Mode[DMX_FILTER_SIZE];
	} IDVBDmxFilter;
	
	typedef struct 
	{
		UInt16				PID;
		IDVBDmxFilter		Filter;
		UInt32				Timeout;
		UInt32				Flags;
		
#define DMX_CHECK_CRC       1
#define DMX_ONESHOT         2
#define DMX_IMMEDIATE_START 4
#define DMX_KERNEL_CLIENT   0x8000
	} IDVBDmxSctFilterParams;
	
	typedef struct 
	{
		UInt16				PID;
		kDVBDmxInput		Input;
		kDVBDmxOutput		Output;
		kDVBDmxPESType		PESType;
		UInt32				Flags;
	} IDVBDmxPESFilterParams;
	
}