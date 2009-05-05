/*
 *  CDVBRingBuffer.h
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

namespace Video4Mac
{
	
	class CDVBRingBuffer
	{
		friend class IDVBDmxDev;
	public:
		CDVBRingBuffer();
		~CDVBRingBuffer();
		inline int			Error()				{ return m_Error; };
		void				Init(size_t len);
		int					Empty();
		ssize_t				Free();
		ssize_t				Avail();
		void				Flush();
		void				Reset();
		void				FlushSpinlockWakeup();
		ssize_t				ReadUser(UInt8 /* __user */ *buf, size_t len);
		void				Read(UInt8 *buf, size_t len);
		ssize_t				Write(const UInt8 *buf, size_t len);
		ssize_t				PacketWrite(UInt8 *buf, size_t len);
		ssize_t				PacketReadUser(size_t idx, int offset, UInt8 /* __user */ *buf, size_t len);
		ssize_t				PacketRead(size_t idx, int offset, UInt8 *buf, size_t len);
		void				PacketDispose(size_t idx);
		ssize_t				PacketNext(size_t idx, size_t* pktlen);
		int					SetBufferSize(unsigned long size);
		ssize_t				BufferRead(int non_blocking, char /* __user */ *buf, size_t count, long long *ppos);
		int					BufferWrite(const UInt8 *src, size_t len);
		
	private:
		UInt8*				m_Data;
		ssize_t				m_Size;
		ssize_t				m_pRead;
		ssize_t				m_pWrite;
		int					m_Error;
		
		//	wait_queue_head_t queue;
		//	spinlock_t        lock;
	};	
};