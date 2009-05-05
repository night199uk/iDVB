/*
 *  CDVBRingBuffer.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "CDVBRingBuffer.h"
#include <CDVBLog.h>

#define PKT_READY 0
#define PKT_DISPOSED 1

using namespace Video4Mac;

#define DVB_RINGBUFFER_PKTHDRSIZE 3

/* peek at byte <offs> in the buffer */
#define DVB_RINGBUFFER_PEEK(offs)	\
m_Data[(m_pRead+(offs))%m_Size]

/* advance read ptr by <num> bytes */
#define DVB_RINGBUFFER_SKIP(num)	\
m_pRead=(m_pRead+(num))%m_Size

/* write routines & macros */
/* ----------------------- */
/* write single byte to ring buffer */
#define DVB_RINGBUFFER_WRITE_BYTE(byte)	\
{ m_Data[m_pWrite]=(byte); \
m_pWrite=(m_pWrite+1)%m_Size; }

CDVBRingBuffer::CDVBRingBuffer()
{
	m_pRead=m_pWrite=m_Size=m_Error=0;
	m_Data = NULL;
}

CDVBRingBuffer::~CDVBRingBuffer()
{
	
}


void CDVBRingBuffer::Init(size_t len)
{
	m_pRead=m_pWrite=0;
	m_Data=(UInt8 *)malloc(len);
	m_Size=len;
	m_Error=0;
	
	//	init_waitqueue_head(&rbuf->queue);
	
	//	spin_lock_init(&(rbuf->lock));
}



int CDVBRingBuffer::Empty()
{
	return (m_pRead==m_pWrite);
}



ssize_t CDVBRingBuffer::Free()
{
	ssize_t free;
	
	free = m_pRead - m_pWrite;
	if (free <= 0)
		free += m_Size;
	return free-1;
}



ssize_t CDVBRingBuffer::Avail()
{
	ssize_t avail;
	
	avail = m_pWrite - m_pRead;
	if (avail < 0)
		avail += m_Size;
	return avail;
}



void CDVBRingBuffer::Flush()
{
	m_pRead = m_pWrite;
	m_Error = 0;
}

void CDVBRingBuffer::Reset()
{
	m_pRead = m_pWrite = 0;
	m_Error = 0;
}

void CDVBRingBuffer::FlushSpinlockWakeup()
{
	unsigned long flags;
	
	//	spin_lock_irqsave(&rbuf->lock, flags);
	Flush();
	//	spin_unlock_irqrestore(&rbuf->lock, flags);
	
	//	wake_up(&rbuf->queue);
}

ssize_t CDVBRingBuffer::ReadUser(UInt8 /* __user */ *buf, size_t len)
{
	size_t todo = len;
	size_t split;
	
	split = (m_pRead + len > m_Size) ? m_Size - m_pRead : 0;
	if (split > 0) {
		memcpy(buf, m_Data+m_pRead, split);
		//			return -EFAULT;
		buf += split;
		todo -= split;
		m_pRead = 0;
	}
	memcpy(buf, m_Data+m_pRead, todo);
	//		return -EFAULT;
	
	m_pRead = (m_pRead + todo) % m_Size;
	
	return len;
}

void CDVBRingBuffer::Read(UInt8 *buf, size_t len)
{
	size_t todo = len;
	size_t split;
	
	split = (m_pRead + len > m_Size) ? m_Size - m_pRead : 0;
	if (split > 0) {
		memcpy(buf, m_Data+m_pRead, split);
		buf += split;
		todo -= split;
		m_pRead = 0;
	}
	memcpy(buf, m_Data+m_pRead, todo);
	
	m_pRead = (m_pRead + todo) % m_Size;
}


ssize_t CDVBRingBuffer::Write(const UInt8 *buf, size_t len)
{
	size_t todo = len;
	size_t split;
	
	split = (m_pWrite + len > m_Size) ? m_Size - m_pWrite : 0;
	
	if (split > 0) {
		memcpy(m_Data+m_pWrite, buf, split);
		buf += split;
		todo -= split;
		m_pWrite = 0;
	}
	memcpy(m_Data+m_pWrite, buf, todo);
	m_pWrite = (m_pWrite + todo) % m_Size;
	
	return len;
}

ssize_t CDVBRingBuffer::PacketWrite(UInt8 *buf, size_t len)
{
	int status;
	ssize_t oldpwrite = m_pWrite;
	
	DVB_RINGBUFFER_WRITE_BYTE(len >> 8);
	DVB_RINGBUFFER_WRITE_BYTE(len & 0xff);
	DVB_RINGBUFFER_WRITE_BYTE(PKT_READY);
	status = Write(buf, len);
	
	if (status < 0) m_pWrite = oldpwrite;
	return status;
}

ssize_t CDVBRingBuffer::PacketReadUser(size_t idx, int offset, UInt8 /* __user */ *buf, size_t len)
{
	size_t todo;
	size_t split;
	size_t pktlen;
	
	pktlen = m_Data[idx] << 8;
	pktlen |= m_Data[(idx + 1) % m_Size];
	if (offset > pktlen) return -EINVAL;
	if ((offset + len) > pktlen) len = pktlen - offset;
	
	idx = (idx + DVB_RINGBUFFER_PKTHDRSIZE + offset) % m_Size;
	todo = len;
	split = ((idx + len) > m_Size) ? m_Size - idx : 0;
	if (split > 0) {
		memcpy(buf, m_Data+idx, split);
		//			return -EFAULT;
		buf += split;
		todo -= split;
		idx = 0;
	}
	memcpy(buf, m_Data+idx, todo);
	//		return -EFAULT;
	
	return len;
}

ssize_t CDVBRingBuffer::PacketRead(size_t idx, int offset, UInt8 *buf, size_t len)
{
	size_t todo;
	size_t split;
	size_t pktlen;
	
	pktlen = m_Data[idx] << 8;
	pktlen |= m_Data[(idx + 1) % m_Size];
	if (offset > pktlen) return -EINVAL;
	if ((offset + len) > pktlen) len = pktlen - offset;
	
	idx = (idx + DVB_RINGBUFFER_PKTHDRSIZE + offset) % m_Size;
	todo = len;
	split = ((idx + len) > m_Size) ? m_Size - idx : 0;
	if (split > 0) {
		memcpy(buf, m_Data+idx, split);
		buf += split;
		todo -= split;
		idx = 0;
	}
	memcpy(buf, m_Data+idx, todo);
	return len;
}

void CDVBRingBuffer::PacketDispose(size_t idx)
{
	size_t pktlen;
	
	m_Data[(idx + 2) % m_Size] = PKT_DISPOSED;
	
	// clean up disposed packets
	while(Avail() > DVB_RINGBUFFER_PKTHDRSIZE) {
		if (DVB_RINGBUFFER_PEEK(2) == PKT_DISPOSED) {
			pktlen = DVB_RINGBUFFER_PEEK(0) << 8;
			pktlen |= DVB_RINGBUFFER_PEEK(1);
			DVB_RINGBUFFER_SKIP(pktlen + DVB_RINGBUFFER_PKTHDRSIZE);
		} else {
			// first packet is not disposed, so we stop cleaning now
			break;
		}
	}
}

ssize_t CDVBRingBuffer::PacketNext(size_t idx, size_t* pktlen)
{
	int consumed;
	int curpktlen;
	int curpktstatus;
	
	if (idx == -1) {
		idx = m_pRead;
	} else {
		curpktlen = m_Data[idx] << 8;
		curpktlen |= m_Data[(idx + 1) % m_Size];
		idx = (idx + curpktlen + DVB_RINGBUFFER_PKTHDRSIZE) % m_Size;
	}
	
	consumed = (idx - m_pRead) % m_Size;
	
	while((Avail() - consumed) > DVB_RINGBUFFER_PKTHDRSIZE) {
		
		curpktlen = m_Data[idx] << 8;
		curpktlen |= m_Data[(idx + 1) % m_Size];
		curpktstatus = m_Data[(idx + 2) % m_Size];
		
		if (curpktstatus == PKT_READY) {
			*pktlen = curpktlen;
			return idx;
		}
		
		consumed += curpktlen + DVB_RINGBUFFER_PKTHDRSIZE;
		idx = (idx + curpktlen + DVB_RINGBUFFER_PKTHDRSIZE) % m_Size;
	}
	
	// no packets available
	return -1;
}

int CDVBRingBuffer::SetBufferSize(unsigned long size)
{
	void *newmem;
	void *oldmem;
	
	if (m_Size == size)
		return 0;
	
	if (!size)
		return -EINVAL;
	
	newmem = malloc(size);
	if (!newmem)
		return -ENOMEM;
	
	oldmem = m_Data;
	
	//	spin_lock_irq(&dmxdevfilter->dev->lock);
	m_Data = (UInt8 *) newmem;
	m_Size = size;
	
	/* reset and not flush in case the buffer shrinks */
	Reset();
	//	spin_unlock_irq(&dmxdevfilter->dev->lock);
	
	free(oldmem);
	
	return 0;
}


// Moved from the dexuxfilter
ssize_t CDVBRingBuffer::BufferRead(int non_blocking, char /* __user */ *buf, size_t count, long long *ppos)
{
	size_t todo;
	ssize_t avail;
	ssize_t ret = 0;
	
	if (!m_Data)
		return 0;
	
	if (m_Error) {
		ret = m_Error;
		Flush();
		return ret;
	}
	
	for (todo = count; todo > 0; todo -= ret) {
		if (non_blocking && Empty()) {
			ret = -EWOULDBLOCK;
			break;
		}
		
		//		ret = wait_event_interruptible(src->queue,
		//					       !dvb_ringbuffer_empty(src) ||
		//					       (src->error != 0));
		if (ret < 0)
			break;
		
		if (m_Error) {
			ret = m_Error;
			Flush();
			break;
		}
		
		avail = Avail();
		if (avail > todo)
			avail = todo;
		
		ret = ReadUser((UInt8 *)buf, avail);
		if (ret < 0)
			break;
		
		buf += ret;
	}
	
	return (count - todo) ? (count - todo) : ret;
}

int CDVBRingBuffer::BufferWrite(const UInt8 *src, size_t len)
{
	ssize_t free;
	
	if (!len)
	{
		return 0;
	}
	
	/*
	 if (!m_Data)
	 {
		 CDVBLog::Log(kDVBLogDemux, "write on a zero buffer.\n");
		 return 0;
	 }
	*/
	free = Free();
	if (len > free) {
		CDVBLog::Log(kDVBLogDemux, "dmxdev: buffer overflow\n");
		return -EOVERFLOW;
	}
	
	return Write(src, len);
}
