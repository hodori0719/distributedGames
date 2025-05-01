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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:nil -*- */

/***************************************************************************
  BufferManager.h

  Manages the buffers between mercury and the application. I don't expect
  this to be a class with much functionality, but it's there just in case
  I want to do some really "cool" buffer management. For example, static
  sized buffers for most cases, but dynamically allocated buffers for
  some infrequent long publications.

begin           : Nov 12, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/


#ifndef __BUFFERMANAGER__H
#define __BUFFERMANAGER__H

#include <vector>
#include <pthread.h>

#include <mercury/common.h>
#include <mercury/PubsubData.h>

    // buffering interface for the Application and network
    //  m_AppBuffer      -> from the app TO the network
    //  m_NetworkBuffer  -> from the network TO the app

    typedef vector<PubsubData *> PubsubVec;
typedef PubsubVec::iterator  PubsubVecIter;
typedef vector<Event *>      EventVec;
typedef EventVec::iterator   EventVecIter;

class BufferManager {
    friend class FloodRouter;
    friend class MercuryNode;

 private:
    byte *           m_ByteBuf;
    PubsubVec        m_AppBuffer;
    pthread_mutex_t  m_AppMutex;
    EventVec         m_NetworkBuffer;
    pthread_mutex_t  m_NetworkMutex;

 public:
    static const int MAX_NWBUF_SIZE = 256; 
    static const int MAX_APPBUF_SIZE = 256;

    BufferManager();
    ~BufferManager();

    byte*       GetByteBuffer( int length );
    void        ReleaseByteBuffer();

    // app -> network
    void        EnqueueAppEvent(Event *pub);
    void        EnqueueAppInterest(Interest *sub);

    // network -> app
    void        EnqueueNetworkEvent(Event *pub);

    Event*      DequeueNetworkEvent();

    // network
    PubsubData*    DequeueAppData();


    void LockAppBuffer() {
	pthread_mutex_lock (&m_AppMutex);
    }
    void UnlockAppBuffer() {
	pthread_mutex_unlock (&m_AppMutex);
    }
    void LockNetworkBuffer() {
	pthread_mutex_lock (&m_NetworkMutex);
    }
    void UnlockNetworkBuffer() {
	pthread_mutex_unlock (&m_NetworkMutex);
    }
};

#endif // __BUFFERMANAGER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
