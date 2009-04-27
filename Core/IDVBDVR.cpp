/*
 *  IDVBDVR.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 13/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "IDVBDVR.h"
#include "IDVBDmxDev.h"
#include "CDVBLog.h"
#include <pthread.h>

#define DVR_BUFFER_SIZE (10*188*1024)


using namespace Video4Mac;

int IDVBDVR::Open()
{
	//	struct dmxdev *dmxdev = (struct dmxdev *) dvbdev->priv;

	struct dmx_frontend *front;
	
	CDVBLog::Log(kDVBLogDemux, "function : %s\n", __func__);
	if (pthread_mutex_lock(&m_DmxDev->m_Mutex))
		return -EINTR;
	
	if (m_DmxDev->m_Exit) {
		pthread_mutex_unlock(&m_DmxDev->m_Mutex);
		return -ENODEV;
	}
	
	//	if ((file->f_flags & O_ACCMODE) == O_RDWR) {
	//		if (!(dmxdev->capabilities & DMXDEV_CAP_DUPLEX)) {
	//			mutex_unlock(&dmxdev->mutex);
	//			return -EOPNOTSUPP;
	//		}
	//	}
	
	//	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
	void *mem;
	if (!m_Readers) {
		pthread_mutex_unlock(&m_DmxDev->m_Mutex);
		return -EBUSY;
		
	}
	mem = malloc(DVR_BUFFER_SIZE);
	if (!mem) {
		pthread_mutex_unlock(&m_DmxDev->m_Mutex);
		return -ENOMEM;
	}
	printf("in dvr open, going to initialize dvr ringbuffer.\n");
//	dvb_ringbuffer_init(&dmxdev->dvr_buffer, mem, DVR_BUFFER_SIZE);
	m_Readers--;
	
	//	if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
	//		dmxdev->dvr_orig_fe = dmxdev->demux->frontend;
	//
	//		if (!dmxdev->demux->write) {
	//			mutex_unlock(&dmxdev->mutex);
	//			return -EOPNOTSUPP;
	//		}
	
//	front = m_DmxDev->GetDmx()->GetFrontend(DMX_MEMORY_FE);
//	front = get_fe(dmxdev->demux, DMX_MEMORY_FE);
	
	if (!front) {
		pthread_mutex_unlock(&m_DmxDev->m_Mutex);
		CDVBLog::Log(kDVBLogDemux, "out function (err) : %s\n", __func__);
		return -EINVAL;
	}
	
//	m_DmxDev->GetDmx()->DisconnectFrontend();
//	dmxdev->demux->disconnect_frontend(dmxdev->demux);
//	m_DmxDev->GetDmx()->ConnectFrontend();
//	dmxdev->demux->connect_frontend(front);
	//	}
	
	m_Users++;
	pthread_mutex_unlock(&m_DmxDev->m_Mutex);
	CDVBLog::Log(kDVBLogDemux, "out function : %s\n", __func__);
	return 0;
	
}

ssize_t IDVBDVR::Read(char /*__user*/ *buf, size_t count, long long *ppos)
{
	int ret;
	
	if (m_DmxDev->m_Exit) {
		pthread_mutex_unlock(&m_DmxDev->m_Mutex);
		return -ENODEV;
	}
	
	pthread_mutex_lock(&m_DmxDev->m_Mutex);
	ret = m_DmxDev->BufferRead(&m_DmxDev->DVRBuffer, 1, buf, count, ppos);
	pthread_mutex_unlock(&m_DmxDev->m_Mutex);
	return ret;
}

unsigned int dvb_dvr_poll(struct dvb_device *dvbdev /*, poll_table *wait*/)
{
	//	struct dvb_device *dvbdev = file->private_data;
	struct dmxdev *dmxdev = (struct dmxdev *) dvbdev->priv;
	unsigned int mask = 0;
	
	//	dprintk("function : %s\n", __func__);
	
	//	poll_wait(file, &dmxdev->dvr_buffer.queue, wait);
	
	//	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
	//		if (dmxdev->dvr_buffer.error)
	//			mask |= (POLLIN | POLLRDNORM | POLLPRI | POLLERR);
	
	if (!dvb_ringbuffer_empty(&dmxdev->dvr_buffer))
		mask |= (POLLIN | POLLRDNORM | POLLPRI);
	//	} else
	//		mask |= (POLLOUT | POLLWRNORM | POLLPRI);
	
	return mask;
}

static int dvb_dvr_do_ioctl(struct dvb_device *dvbdev,
							unsigned int cmd, void *parg)
{
	//	struct dvb_device *dvbdev = file->private_data;
	struct dmxdev *dmxdev = (struct dmxdev *) dvbdev->priv;
	unsigned long arg = (unsigned long)parg;
	int ret;
	
	if (pthread_mutex_lock(&dmxdev->mutex))
		return kIOReturnInternalError;
	
	switch (cmd) {
		case DMX_SET_BUFFER_SIZE:
			ret = dvb_dvr_set_buffer_size(dmxdev, arg);
			break;
			
		default:
			ret = kIOReturnBadArgument;
			break;
	}
	pthread_mutex_unlock(&dmxdev->mutex);
	return ret;
}

static int dvb_dvr_release(struct dvb_device *dvbdev)
{
	//	struct dvb_device *dvbdev = file->private_data;
	struct dmxdev *dmxdev = (struct dmxdev *) dvbdev->priv;
	
	printf("function : %s\n", __func__);
	pthread_mutex_lock(&dmxdev->mutex);
	
	//	if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
	dmxdev->demux->disconnect_frontend(dmxdev->demux);
	dmxdev->demux->connect_frontend(dmxdev->demux,
									dmxdev->dvr_orig_fe);
	//	}
	//	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
	dvbdev->readers++;
	if (dmxdev->dvr_buffer.data) {
		void *mem = dmxdev->dvr_buffer.data;
		//			mb();
		//			spin_lock_irq(&dmxdev->lock);
		dmxdev->dvr_buffer.data = NULL;
		//			spin_unlock_irq(&dmxdev->lock);
		free(mem);
	}
	//	}
	/* TODO */
	dvbdev->users--;
	if(dvbdev->users==-1 && dmxdev->exit==1) {
		//		fops_put(file->f_op);
		//		file->f_op = NULL;
		pthread_mutex_unlock(&dmxdev->mutex);
		//		wake_up(&dvbdev->wait_queue);
	} else
		pthread_mutex_unlock(&dmxdev->mutex);
	
	return 0;
}

static ssize_t dvb_dvr_write(struct dvb_device *dvbdev, const char /*__user*/ *buf,
							 size_t count, loff_t *ppos)
{
	//	struct dvb_device *dvbdev = file->private_data;
	struct dmxdev *dmxdev = (struct dmxdev *) dvbdev->priv;
	int ret;
	
	if (!dmxdev->demux->write)
		return -EOPNOTSUPP;
	//	if ((file->f_flags & O_ACCMODE) != O_WRONLY)
	return kIOReturnBadArgument;
	if (pthread_mutex_lock(&dmxdev->mutex))
		return kIOReturnInternalError;
	
	if (dmxdev->exit) {
		pthread_mutex_unlock(&dmxdev->mutex);
		return -ENODEV;
	}
	ret = dmxdev->demux->write(dmxdev->demux, buf, count);
	pthread_mutex_unlock(&dmxdev->mutex);
	return ret;
}

ssize_t dvb_dvr_read(struct dvb_device *dvbdev, char /*__user*/ *buf, size_t count,
					 loff_t *ppos)
{
	//	struct dvb_device *dvbdev = file->private_data;
	struct dmxdev *dmxdev = (struct dmxdev *) dvbdev->priv;
	int ret;
	
	if (dmxdev->exit) {
		pthread_mutex_unlock(&dmxdev->mutex);
		return -ENODEV;
	}
	
	//	pthread_mutex_lock(&dmxdev->mutex);
	ret = dvb_dmxdev_buffer_read(&dmxdev->dvr_buffer,
								 1,
								 buf, count, ppos);
	//	pthread_mutex_unlock(&dmxdev->mutex);
	return ret;
}

static int dvb_dvr_set_buffer_size(struct dmxdev *dmxdev,
								   unsigned long size)
{
	struct dvb_ringbuffer *buf = &dmxdev->dvr_buffer;
	void *newmem;
	void *oldmem;
	
	dprintk("function : %s\n", __func__);
	
	if (buf->size == size)
		return 0;
	if (!size)
		return kIOReturnBadArgument;
	
	newmem = malloc(size);
	if (!newmem)
		return -ENOMEM;
	
	oldmem = buf->data;
	
	//	spin_lock_irq(&dmxdev->lock);
	buf->data = (UInt8 *)newmem;
	buf->size = size;
	
	/* reset and not flush in case the buffer shrinks */
	dvb_ringbuffer_reset(buf);
	//	spin_unlock_irq(&dmxdev->lock);
	
	free(oldmem);
	
	return 0;
}

