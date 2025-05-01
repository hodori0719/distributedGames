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

#ifndef __CBR_CONNECTION__H
#define __CBR_CONNECTION__H

#include <wan-env/UDPConnection.h>
#include <wan-env/CBRTransport.h>

typedef list<Packet *> PacketQueue;

class CBRConnection : public UDPConnection {

    friend class CBRTransport;

 protected:

    Mutex       m_SendLock;  // lock on send queue and send time
    PacketQueue m_SendQueue; // pending packets to send
    TimeVal     m_SendTime;  // min time to send next packet

    //
    // These are NOT thread safe -- you must explicitly call SQ_Lock/Unlock
    // before using any of these accessors!
    //

    inline void SQ_Lock() { 
#ifdef ENABLE_REALNET_THREAD 
	m_SendLock.Acquire();
#endif 
    }
    inline void SQ_Unlock() { 
#ifdef ENABLE_REALNET_THREAD 
	m_SendLock.Release();
#endif 
    }

    uint32 SQ_Size();
    Packet *SQ_Pop();
    void SQ_Insert(Packet *pkt);

    void SetSendTime(TimeVal time);
    TimeVal GetSendTime();

 protected:

    CBRConnection(Transport *t, Socket sock, IPEndPoint *otherEnd);
    int Send(Packet *tosend);

 public:

    virtual ~CBRConnection();
};

#endif // __CBR_CONNECTION__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
