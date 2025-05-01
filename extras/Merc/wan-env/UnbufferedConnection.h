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

#ifndef __UNBUFFERED_CONNECTION__H
#define __UNBUFFERED_CONNECTION__H

#include <util/debug.h>
#include <wan-env/Connection.h>

class UnbufferedConnection : public Connection {

 protected:

    TimeVal  m_TimeStamp;     // Timestamp on received packet
    Packet*  m_Packet;        // Packet to store incoming bytes

    UnbufferedConnection(Transport *t, Socket sock, 
			 IPEndPoint *otherEnd) :
	Connection(t, sock, otherEnd), 
	m_Packet(0), m_TimeStamp(TIME_NONE) {}
    virtual ~UnbufferedConnection() {
	if (m_Packet)
	    delete m_Packet;
    }

    void _InitPacket(uint32 len) {
	if (m_Packet) {
	    WARN << "Initializing new packet without reseting it!" << endl;
	}
	m_Packet = new Packet(len);
	m_Packet->ResetBufPosition();
    }

    void _ResetPacket() {
	m_Packet = 0;
	m_TimeStamp = TIME_NONE;
    }

    Packet *GetNextPacket(PacketAuxInfo *aux) {
	aux->timestamp = m_TimeStamp;
	Packet *ret = m_Packet;
	_ResetPacket();
	return ret;
    }

    void FreePacket(Packet *pkt) {
	delete pkt;
    }
};

#endif // __UNBUFFERED_CONNECTION__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
