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

#ifndef __DELAYED_CONNECTION__H
#define __DELAYED_CONNECTION__H

#include <wan-env/DelayedTransport.h>

struct DelayedPacketInfo {
    Packet *pkt;
    ConnStatusType status;
    TimeVal timestamp;

    DelayedPacketInfo(ConnStatusType status, Packet *pkt, TimeVal stamp) : 
	status(status), pkt(pkt), timestamp(stamp) {}
};

typedef list<DelayedPacketInfo> DelayedPacketInfoQueue;

class DelayedConnection : public Connection {

    friend class DelayedTransport;

    DelayedPacketInfoQueue m_Queue;

 protected:

    DelayedConnection(Transport *t, Socket sock, IPEndPoint *otherEnd);

    ConnStatusType ReadyStatus(TimeVal& now);
    int  Size();
    void Insert(ConnStatusType status, Packet *pkt, TimeVal& val);
    DelayedPacketInfo Pop();

    int Send(Packet *tosend);
    Packet *GetNextPacket(PacketAuxInfo *aux);
    void FreePacket(Packet *pkt);

 public:

    virtual ~DelayedConnection();
};

#endif // __DELAYED_CONNECTION__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
