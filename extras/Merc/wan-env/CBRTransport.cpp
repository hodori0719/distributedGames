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

#include <wan-env/CBRTransport.h>

void CBRTransport::_ClearConnections() {
    Lock();
    m_SenderList.clear();
    Unlock();
    // parent will actually delete the connections
    Transport::_ClearConnections();
}

void CBRTransport::_CleanupConnections() {
    Lock();
    for (ConnectionListIter iter = m_SenderList.begin();
	 iter != m_SenderList.end(); /* !!! */ ) {
	Connection *connection = *iter;

	if (connection->GetStatus() == CONN_CLOSED) {
	    ConnectionListIter oiter = iter;
	    oiter++;
	    m_SenderList.erase(iter);
	    iter = oiter;
	}
	else {
	    iter++;
	}
    }
    Unlock();
    // parent will remove conn from m_ConnectionList and m_AppConnHash
    // as well as actually delete the connection
    Transport::_CleanupConnections();
}

UDPConnection *CBRTransport::CreateConnection(Socket sock, 
					      IPEndPoint *otherEnd)
{
    return new CBRConnection(this, sock, otherEnd);
}

void CBRTransport::AddConnection(Connection *conn) {
    UDPTransport::AddConnection(conn);
    Lock();
    m_SenderList.push_back(conn);
    Unlock();
}

void CBRTransport::RemoveConnection(Connection *conn) {
    UDPTransport::AddConnection(conn);
    Lock();
    m_SenderList.remove(conn);
    Unlock();
}

void CBRTransport::DoWork(fd_set *isset, u_long timeout_usecs)
{
    float SELF_FRAC = 0.25;
    UDPTransport::DoWork(isset, (u_long) ((1 - SELF_FRAC) * timeout_usecs));

    TimeVal now = TimeNow ();

    if (now < m_SendTime) 
	return;

    Lock();

    ConnectionList serviced;

    for (ConnectionListIter it = m_SenderList.begin();
	 it != m_SenderList.end(); /* !!! */ ) {

	CBRConnection *conn = (CBRConnection *)(*it);

	// send 1ms worth of packets from us
	if (now + MAX_SERVICE_BURST < m_SendTime) 
	    break;

	conn->SQ_Lock();

	if ( conn->GetStatus() == CONN_CLOSED ||
	     conn->GetStatus() == CONN_ERROR  ||
	     conn->GetSendTime() >  now ||
	     conn->SQ_Size()   == 0 ) {

	    conn->SQ_Unlock();
	    it++;
	    continue;
	}

	// send 1ms worth of packets to this dude
	while (m_SendTime < now + MAX_SERVICE_BURST &&
	       conn->m_SendTime < now + MAX_SERVICE_BURST &&
	       conn->SQ_Size() > 0) {

	    // connection ready to send another packet!
	    Packet *pkt = conn->SQ_Pop();
	    int ret = _Send_UDP(conn->GetAppPeerAddress(),
				pkt->GetBuffer (),
				pkt->GetUsed ());
	    delete pkt;

	    if (ret < 0) {
		// error! what can we do?
		WARN << "CBR send error: " << strerror(errno) << endl;
		break;
	    }

	    // set the next time this flow is allowed to send
	    conn->SetSendTime(now + ((double)ret/MAX_PERFLOW_RATE)*MSEC_IN_SEC);

	    // set the next time this transport is allowed to send
	    m_SendTime = now + ((double)ret/MAX_AGGREGATE_RATE)*MSEC_IN_SEC;
	}

	conn->SQ_Unlock();

	ConnectionListIter oit = it;
	oit++;

	// remove conn and remember to place it in the back to give others
	// a chance to proceed
	m_SenderList.erase(it);
	serviced.push_back(conn);

	it = oit;
    }

    // now put everyone that was serviced back at the end of the list
    for (ConnectionListIter it = serviced.begin();
	 it != serviced.end(); it++ ) {
	m_SenderList.push_back(*it);
    }
    serviced.clear();

    Unlock();
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
