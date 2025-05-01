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

#include <wan-env/UDPConnection.h>
#include <mercury/Message.h>
#include <mercury/Packet.h>
#include <wan-env/RealNet.h>

int UDPConnection::Size() {
    Lock();
    int ret = m_Queue.size();
    Unlock();
    return ret;
}

void UDPConnection::Insert(Packet *pkt, TimeVal& stamp) {
    Lock();
    m_Queue.push_back(PacketInfo(pkt, stamp));
    Unlock();
}

PacketInfo UDPConnection::Pop() {
    PacketInfo ret(NULL, TIME_NONE);
    Lock();
    if (m_Queue.size() != 0) {
	ret = m_Queue.front();
	m_Queue.pop_front();
    }
    Unlock();
    return ret;
}

UDPConnection::UDPConnection(Transport *t, Socket sock, IPEndPoint *otherEnd) :
    Connection(t, sock, otherEnd)
{
    SetSocketPeerAddress(otherEnd);
}

UDPConnection::~UDPConnection() {
    Lock();
    for (PacketInfoQueue::iterator it = m_Queue.begin();
	 it != m_Queue.end(); it++) {
	delete it->pkt;
    }
    Unlock();
}

int UDPConnection::Send(Packet *pkt) {
    int ret = ((UDPTransport *)GetTransport())->_Send_UDP(GetAppPeerAddress(),
							  pkt->GetBuffer (),
							  pkt->GetUsed ());
    delete pkt;

    return ret;
}

Packet *UDPConnection::GetNextPacket(PacketAuxInfo *aux)
{
    PacketInfo info = Pop();
    if (info.pkt == NULL) {
	bzero(aux, sizeof(PacketAuxInfo));
	return NULL;
    } else {
	DB(20) << " getting a packet from UDP connection " << endl;
	aux->timestamp = info.timestamp;

	GetTransport()->IncrReadPackets();
	return info.pkt;
    }
}

void UDPConnection::FreePacket(Packet *pkt)
{
    delete pkt;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
