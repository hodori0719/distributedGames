////////////////////////////////////////////////////////////////////////////////
// Mercury and Colyseus Software Distribution 
// 
// Copyright (C) 2004-2005 Ashwin Bharambe (ashu@cs.cmu.edu)
//               2004-2005 Jeffrey Pang    (jeffpang@cs.cmu.edu)
//                    2004 Mukesh Agrawal  (mukesh@cs.cmu.edu)
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2, or (at
// your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************
  BufferManager.cpp

  Manages the buffers between mercury and the application. I don't expect 
  this to be a class with much functionality, but it's there just in case
  I want to do some really "cool" buffer management. For example, static
  sized buffers for most cases, but dynamically allocated buffers for
  some infrequent long publications.


XXX: This class should be used by MercuryNode and Controller 
for managing event-queues and by the Connection/RealNet class for
managing buffers. Perhaps, we should split these functionalities 
properly now. - [08/10/2003]

begin           : Nov 12, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/


#include <mercury/BufferManager.h>
#include <mercury/PubsubData.h>

    const int BufferManager::MAX_NWBUF_SIZE;
const int BufferManager::MAX_APPBUF_SIZE;

BufferManager::BufferManager():m_ByteBuf(0)
{
    pthread_mutex_init (&m_AppMutex, NULL);
    pthread_mutex_init (&m_NetworkMutex, NULL);
}

BufferManager::~BufferManager()
{
    pthread_mutex_destroy (&m_AppMutex);
    pthread_mutex_destroy (&m_NetworkMutex);
}

// this is the naivest implementation. just a wrapper around new/delete for the moment
byte *BufferManager::GetByteBuffer(int length)
{
    m_ByteBuf = new byte[length];
    return m_ByteBuf;
}

void BufferManager::ReleaseByteBuffer()
{
    delete[]m_ByteBuf;
    m_ByteBuf = 0;
}

void BufferManager::EnqueueAppEvent(Event * pub)
{
    LockAppBuffer();

    PubsubData *pdu = new PubsubData(pub);
    m_AppBuffer.push_back(pdu);
    UnlockAppBuffer();
}

void BufferManager::EnqueueAppInterest(Interest * sub)
{
    LockAppBuffer();
    PubsubData *pdu = new PubsubData(sub);
    m_AppBuffer.push_back(pdu);
    UnlockAppBuffer();
}

void BufferManager::EnqueueNetworkEvent(Event * pub)
{
    LockNetworkBuffer();
#if 0
#endif
    m_NetworkBuffer.push_back(pub);
    UnlockNetworkBuffer();
}

Event *BufferManager::DequeueNetworkEvent()
{
    Event *pub = 0;

    LockNetworkBuffer();
    if (m_NetworkBuffer.size() > 0) {
	EventVecIter it = m_NetworkBuffer.begin();
	pub = *it;
	m_NetworkBuffer.erase(it);
    }
    UnlockNetworkBuffer();

    return pub;
}

PubsubData *BufferManager::DequeueAppData()
{
    PubsubData *pdu = 0;

    LockAppBuffer();
    if (m_AppBuffer.size() > 0) {
	PubsubVecIter it = m_AppBuffer.begin();
	pdu = *it;
	m_AppBuffer.erase(it);
    }
    UnlockAppBuffer();

    return pdu;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
