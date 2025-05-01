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
#ifndef BENCHMARK_REQUIRED
#define BENCHMARK_REQUIRED
#endif

#include <mercury/Packet.h>
#include <wan-env/CBRConnection.h>
#include <wan-env/RealNet.h>

uint32 CBRConnection::SQ_Size() {
    return m_SendQueue.size();
}

void CBRConnection::SQ_Insert(Packet *pkt) {
    m_SendQueue.push_back(pkt);
}

Packet *CBRConnection::SQ_Pop() {
    Packet *ret = NULL;
    if (m_SendQueue.size() != 0) {
	ret = m_SendQueue.front();
	m_SendQueue.pop_front();
    }
    return ret;
}

void CBRConnection::SetSendTime(TimeVal time)
{
    m_SendTime = time;
}

TimeVal CBRConnection::GetSendTime()
{
    return m_SendTime;
}

CBRConnection::CBRConnection(Transport *t, Socket sock, IPEndPoint *otherEnd) :
    UDPConnection(t, sock, otherEnd),
    m_SendTime(TIME_NONE)
{
}

CBRConnection::~CBRConnection() {
    SQ_Lock();
    for (PacketQueue::iterator it = m_SendQueue.begin();
	 it != m_SendQueue.end(); it++) {
	delete *it;
    }
    SQ_Unlock();
}

int CBRConnection::Send(Packet *pkt) {

    SQ_Lock();

    if (SQ_Size() >= (uint32)(CBRTransport::SEND_QUEUE_SIZE)) {

	//WARN << "Dropping packet queue_len=" << SQ_Size() << endl;

	START(CBRConnection::DROPPING_STUFF);
	switch (CBRTransport::SEND_QUEUE_DISCIPLINE) {
	case Q_DROPTAIL: {
	    delete pkt;
	    SQ_Unlock();
	    return -1;
	}
	case Q_DROPHEAD: {
	    Packet *head = SQ_Pop();
	    delete head;
	    break;
	}
	default:
	    WARN << "Unknown queuing discipline: "
		 << CBRTransport::SEND_QUEUE_DISCIPLINE 
		 << endl;
	    ASSERT(0);
	}
	STOP(CBRConnection::DROPPING_STUFF);
    }

    START(CBRConnection::SQ_INSERT);
    SQ_Insert(pkt);
    STOP(CBRConnection::SQ_INSERT);

    SQ_Unlock();

    return 0;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
