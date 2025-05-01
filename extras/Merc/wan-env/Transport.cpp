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

#include <util/OS.h>
#include <wan-env/Transport.h>
#include <wan-env/UDPTransport.h>
#include <wan-env/TCPTransport.h>
#include <wan-env/TCPPassiveTransport.h>
#include <mercury/Timer.h>
#include <wan-env/CBRTransport.h>

class ResetReadPktsTimer : public Timer {
    Transport *m_Transport;
    int timeout;
public:
    ResetReadPktsTimer(Transport *transport, int timeout) : Timer(timeout), m_Transport(transport), timeout(timeout) {}
    void OnTimeout() {	
	m_Transport->m_ReadPackets = 0;  
	_RescheduleTimer(timeout);
    }
};

Transport::Transport() : m_ReadPackets(0) {}

Transport *Transport::Create(RealNet *net, TransportType protocol, IPEndPoint id) {
    Transport *ret = 0;

    switch (protocol) {
    case PROTO_UDP:
	ret = new UDPTransport();
	break;
    case PROTO_TCP:
	ret = new TCPTransport();
	break;
    case PROTO_TCP_PASSIVE:
	ret = new TCPPassiveTransport();
	break;
    case PROTO_CBR:
	ret = new CBRTransport();
	break;

	//
	// Insert Additional Transport constructors here!
	//

    default:
	ASSERT(0);
    }

    ret->m_Network = net;
    ret->m_ID      = id;
    ret->m_Proto   = protocol;

    ref<ResetReadPktsTimer> t = new refcounted<ResetReadPktsTimer>(ret, 5000);
    IPEndPoint addr = net->GetAppID();
    net->GetScheduler()->RaiseEvent(t, addr, 5000);
    return ret;
}

#ifndef streq
#define streq(a,b) (!strcmp((a),(b)))
#endif

TransportType Transport::GetProtoByName(const char *proto_name)
{
    const char *name_part;
    if (!strncmp(proto_name, "PROTO_", strlen("PROTO_"))) {
	name_part = proto_name + strlen("PROTO_");
    } else {
	name_part = proto_name;
    }

    if (streq(name_part, "UDP")) {
	return PROTO_UDP;
    } else if (streq(name_part, "TCP")) {
	return PROTO_TCP;
    } else if (streq(name_part, "TCP_PASSIVE")) {
	return PROTO_TCP_PASSIVE;
    } else if (streq(name_part, "CBR")) {
	return PROTO_CBR;
    } else {
	WARN << "unknown protocol name: " << proto_name << " ("
	     << name_part << ")" << endl;
	ASSERT(0);
    }

    return PROTO_INVALID;
}

TimeVal& Transport::TimeNow () {
    return m_Network->GetScheduler ()->TimeNow ();
}

void Transport::_ClearConnections() {
    Lock();

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); iter++) {
	Connection *connection = (*iter);
	if (connection) {
	    DBG << "deleting connection to: " 
		<< connection->GetAppPeerAddress() << endl;
	    OS::CloseSocket( connection->GetSocket() );
	    delete connection;
	}
    }
    m_ConnectionList.clear();
    m_AppConnHash.clear();

    Unlock();
}

//
// get rid of connections which were closed by us or the other side
//
void Transport::_CleanupConnections() {
    Lock();

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); /* WATCH OUT! */ ) {
	Connection *connection = (*iter);
	if (connection->GetStatus() == CONN_CLOSED ||
	    connection->GetStatus() == CONN_ERROR) {
	    DBG << "Deleting connection: " << connection << endl;

	    IPEndPoint *peer = connection->GetAppPeerAddress();
	    m_AppConnHash.Flush(peer);

	    // Can't rely on 'iter' being a valid "next" iterator 
	    // after the erase operation.
	    ConnectionListIter oiter = iter;
	    oiter++;
	    m_ConnectionList.erase(iter);
	    delete connection;

	    iter = oiter;
	}
	else {
	    iter++;
	}
    }

    Unlock();
}

///////////////////////////////////////////////////////////////////////////////

void Transport::_PrintConnections()
{
    int i = 1;
    for (ConnectionListIter iter = m_ConnectionList.begin();
	 iter != m_ConnectionList.end(); iter++) {
	Connection *connection = (*iter);

	DB(1) << "Connection #" << i++ << ": \tapp="
	      << connection->GetAppPeerAddress()->ToString() << endl;
	DB(1) << " sock:" << connection->GetSocketPeerAddress()->ToString()
	      << " status:" << g_ConnStatusStrings[connection->GetStatus()]
	      << endl;
    }
}

void Transport::_PrintConnHash()
{
    int i = 0;
    DB(1) << "<<<<< HASH >>>>>\n";
    for (ConnectionHashIter iter = m_AppConnHash.begin();
	 iter != m_AppConnHash.end(); iter++) {
	Connection *connection = (*iter).second;

	DB(1) << "Connection key: <" << (*iter).first.ToString() << "> " 
	      << endl;
	DB(1) << "\tapp="
	      << connection->GetAppPeerAddress()->ToString() << endl;
	DB(1) << " sock:" << connection->GetSocketPeerAddress()->ToString()
	      << " status:" << g_ConnStatusStrings[connection->GetStatus()]
	      << endl;
    }
    DB(1) << "-------------------\n";
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
