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

#ifndef __UDP_CONNECTION__H
#define __UDP_CONNECTION__H

#include <wan-env/UDPTransport.h>

struct PacketInfo {
    Packet *pkt;
    TimeVal timestamp;

    PacketInfo(Packet *pkt, TimeVal& stamp) : pkt(pkt), timestamp(stamp) {}
};

typedef list<PacketInfo> PacketInfoQueue;

class UDPConnection : public Connection {

    friend class UDPTransport;

    Mutex           m_Lock;
    PacketInfoQueue m_Queue;

 protected:

    UDPConnection(Transport *t, Socket sock, IPEndPoint *otherEnd);

    int Send(Packet *tosend);
    Packet *GetNextPacket(PacketAuxInfo *aux);
    void FreePacket(Packet *pkt);

    inline void Lock() { 
#ifdef ENABLE_REALNET_THREAD 
	m_Lock.Acquire(); 
#endif
    }
    inline void Unlock() { 
#ifdef ENABLE_REALNET_THREAD 
	m_Lock.Release(); 
#endif
    }

    int  Size();
    void Insert(Packet *pkt, TimeVal& val);
    PacketInfo Pop();

 public:

    virtual ~UDPConnection();
};

#endif // __UDP_CONNECTION__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
