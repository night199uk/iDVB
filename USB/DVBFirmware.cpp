/*
 *  DVBFirmware.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 06/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "DVBFirmware.h"
#include <fstream>

using namespace std;
using namespace Video4Mac;

Firmware::Firmware(char *filename)
{
	fstream file;
	
	int fd;
	file.open(filename, ios::in|ios::binary|ios::ate);
	if (file.is_open())
	{
		m_Filename = strdup(filename);
		m_Size = file.tellg();
		m_Data = new char[m_Size];
		file.seekg(0, ios::beg);
		file.read(m_Data, m_Size);
		file.close();
	} else {
		fprintf(stderr, "Could not open file %s!\n", filename);
		m_Size == 0;
	}
}

Firmware::~Firmware()
{
	delete[] m_Data;
}

char *Firmware::GetFilename()
{
	return m_Filename;
}

int Firmware::GetSize()
{
	return m_Size;
}

int Firmware::GetHexline(struct hexline *hx,
						 int *pos)
{
	UInt8 *b = (UInt8 *) &m_Data[*pos];
	
    int data_offs = 4;
    if (*pos >= m_Size)
        return 0;
	
    memset(hx,0,sizeof(struct hexline));
	
    hx->len  = b[0];
	
    if ((*pos + hx->len + 4) >= m_Size)
        return -1;
	
    hx->addr = b[1] | (b[2] << 8);
    hx->type = b[3];
	
    if (hx->type == 0x04) {
        /* b[4] and b[5] are the Extended linear address record data field */
        hx->addr |= (b[4] << 24) | (b[5] << 16);
		/*      hx->len -= 2;
		 data_offs += 2; */
    }
    memcpy(hx->data,&b[data_offs],hx->len);
    hx->chk = b[hx->len + data_offs];
	
    *pos += hx->len + 5;
	
    return *pos;
}


