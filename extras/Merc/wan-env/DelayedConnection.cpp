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

#include <mercury/Message.h>
#include <mercury/Packet.h>
#include <wan-env/DelayedConnection.h>
#include <wan-env/RealNet.h>

int DelayedConnection::Size() {
    //Lock();
    int ret = m_Queue.size();
    //Unlock();
    return ret;
}

void DelayedConnection::Insert(ConnStatusType status, Packet *pkt, 
			       TimeVal& stamp) {
    //Lock();
    m_Queue.push_back(DelayedPacketInfo(status, pkt, stamp));
    //Unlock();
}

DelayedPacketInfo DelayedConnection::Pop() {
    DelayedPacketInfo ret(CONN_INVALID, NULL, TIME_NONE);
    //Lock();
    if (m_Queue.size() != 0) {
	ret = m_Queue.front();
	m_Queue.pop_front();
    }
    //Unlock();
    return ret;
}

DelayedConnection::DelayedConnection(Transport *t, Socket sock, 
				     IPEndPoint *otherEnd) :
    Connection(t, sock, otherEnd)
{
    SetSocketPeerAddress(otherEnd);
}

DelayedConnection::~DelayedConnection() {
    //Lock();
    for (DelayedPacketInfoQueue::iterator it = m_Queue.begin();
	 it != m_Queue.end(); it++) {
	delete it->pkt;
    }
    //Unlock();
}

int DelayedConnection::Send(Packet *pkt) {
    // can't send packets via this transport connection!
    ASSERT(false);

    return -1;
}

Packet *DelayedConnection::GetNextPacket(PacketAuxInfo *aux)
{
    DelayedPacketInfo info = Pop();
    if (info.pkt == NULL) {
	bzero(aux, sizeof(PacketAuxInfo));
	return NULL;
    } else {
	DB(20) << " getting a packet from UDP connection " << endl;
	aux->timestamp = info.timestamp;
	return info.pkt;
    }
}

void DelayedConnection::FreePacket(Packet *pkt)
{
    delete pkt;
}

ConnStatusType DelayedConnection::ReadyStatus(TimeVal& now)
{
    // Lock();
    if (m_Queue.size() == 0)
	return CONN_NOMSG;
    DelayedPacketInfo info = m_Queue.front();
    // Unlock();

    // only is ready if the first packet we get should have been received
    // by now. -- assumes that packets are enqueued FIFO
    return info.timestamp <= now ? info.status : CONN_NOMSG;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
