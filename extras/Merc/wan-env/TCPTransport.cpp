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

#include <netinet/tcp.h>
#include <util/Utils.h>
#include <util/OS.h>
#include <mercury/NetworkLayer.h>
#include <wan-env/TCPTransport.h>

void TCPTransport::StartListening()
{
    struct sockaddr_in	server_address;
    Socket sock;

    int err = OS::CreateSocket(&sock, (int) PROTO_TCP);
    if (err < 0) {
	ASSERT(0);
    }
    m_ListenSocket = sock;

    // make re-starting the server easier
    int restart = 1;
    if (OS::SetSockOpt(sock, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &restart, sizeof(int)) < 0 )
	WARN << "StartListening: could not configure server socket properly: " 
	     << strerror(errno) << endl;

    // turn off nagle for real-time sending
    int nonagle = 1;
    if (OS::SetSockOpt(sock, IPPROTO_TCP, TCP_NODELAY, 
		       (char *) &nonagle, sizeof(int)) < 0 )
	WARN << "StartListening: could not configure server socket properly: " 
	     << strerror(errno) << endl;

    memset((char *)&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = m_ID.m_IP; // already in network order
    //server_address.sin_addr.s_addr = INADDR_ANY;   // XXX !HACK! - Ashwin [07/11/2005]

    server_address.sin_port = htons(m_ID.m_Port);

    if (bind(sock, (struct sockaddr *)&server_address, 
	     sizeof(server_address)) < 0 ) {
	perror("bind");
	Debug::die ("StartListening: bind error for reliable port [%d]\n", 
		    m_ID.m_Port);
    }

    // get ready to accept connection requests
    if (listen(sock, MAX_PENDING_REQUESTS) < 0 ) {
	perror("listen");
	WARN << " could not listen at " << m_ID << endl;
	Debug::die ("start_listening: could not listen at port[%d]\n", 
		    m_ID.m_Port);
    }

    DB(1) << "Started [PROTO_TCP] server at port " 
	  << m_ID.m_Port << " successfully..." << endl;
    return;
}

void  TCPTransport::StopListening()
{
    if (m_ListenSocket > 0)
	OS::CloseSocket(m_ListenSocket);

    _ClearConnections();
}

int  TCPTransport::FillReadSet(fd_set *tofill)
{
    FD_SET(m_ListenSocket, tofill);
    int maxfd = m_ListenSocket;

    Lock();

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); iter++) {
	Connection *connection = (*iter);
	ASSERT(connection != 0);

	if (connection->GetStatus() == CONN_ERROR || 
	    connection->GetStatus() == CONN_CLOSED)
	    continue;

	FD_SET(connection->GetSocket(), tofill);
	if (connection->GetSocket() > maxfd) {
	    maxfd = connection->GetSocket();
	}
    }

    Unlock();

    return maxfd;
}

void  TCPTransport::DoWork(fd_set *isset, u_long timeout_usecs)
{    
    // accept new connections
    _RegisterNewTCPConnections(isset);

    // periodically clean up closed connections
    TimeVal now = TimeNow ();
    PERIODIC2(1000, now, _CleanupConnections() );
}

Connection *TCPTransport::GetConnection(IPEndPoint *toWhom)
{
    Lock();
    Connection *connection = m_AppConnHash.Lookup(toWhom);
    Unlock();

    if (!connection || 
	connection->GetStatus() == CONN_CLOSED || 
	connection->GetStatus() == CONN_ERROR) {

	Socket sock = 0;

	int err = _DoConnect(&sock, toWhom);
	if (err < 0) {
	    return NULL; // XXX need to report err somehow
	}

	// remove old connection from hash and list
	m_AppConnHash.Flush(toWhom);
	if (connection)
	    m_ConnectionList.remove(connection);

	// create new connection and insert into hash and list
	connection = new TCPConnection(this, sock, toWhom);

	Lock();
	m_ConnectionList.push_back(connection);
	// I know the other end point here, since I am actively connecting. 
	// So this is the ONLY address I will ever use to identify "this" 
	// particular connection 
	m_AppConnHash.Insert(connection->GetAppPeerAddress(), connection);
	Unlock();

	// added a socket, so must interrupt select
	GetNetwork()->InterruptWorker();
    }

    return connection;
}

ConnStatusType TCPTransport::GetReadyConnection(Connection **connp)
{
    // XXX: more efficient please!

    *connp = 0;
    ConnStatusType ret = CONN_NOMSG;

    Lock();

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); iter++) {
	TCPConnection *connection = (TCPConnection *)(*iter);

	if ( connection->GetStatus() == CONN_CLOSED || 
	     connection->GetStatus() == CONN_ERROR  ||
	     !RealNet::IsDataWaiting(connection->GetSocket()) )
	    continue;

	// Try to read a message from this connection
	switch (connection->PerformRead()) {
	case NetworkLayer::READ_CLOSE:
	    DB(1) << "connection close!" << endl;
	    connection->SetStatus(CONN_CLOSED);
	    *connp = connection;
	    ret = CONN_CLOSED;
	    break;
	case NetworkLayer::READ_ERROR:
	    DB(1) << "connection error!" << endl;
	    connection->SetStatus(CONN_ERROR);
	    *connp = connection;
	    ret = CONN_ERROR;
	    break;
	case NetworkLayer::READ_INCOMPLETE:
	    DBG << "Read Incomplete" << endl;
	    break;
	case NetworkLayer::READ_COMPLETE:
	    *connp = connection;
	    ret = connection->GetStatus();
	    connection->SetStatus(CONN_OK);
	    break;
	}

	if (*connp) {
	    // we serviced this fellow now - let the other guys have a chance!
	    // put this connection at the END of the list.
	    m_ConnectionList.erase(iter);
	    m_ConnectionList.push_back(connection);
	    break;
	}
    }

    Unlock();

    return ret;
}

void TCPTransport::CloseConnection(IPEndPoint *target) {
    Lock();
    Connection *connection = m_AppConnHash.Lookup(target);
    Unlock();
    if (!connection)
	return;

    OS::CloseSocket(connection->GetSocket()); // do it ourselves.
    connection->SetStatus(CONN_CLOSED);
}

int TCPTransport::_DoConnect(Socket *pSock, IPEndPoint *otherEnd, 
			     int maxTrials) {
    DB(20) << "connecting to: " << otherEnd->ToString()
	   << " (TCP:" << m_ID.m_Port << ")..." << endl;

    // XXX TODO: Should not block!

    for (int i = 0; i < maxTrials; i++) {
	DB(4) << i << ".." << endl; 

	int err = _Connect_TCP(pSock, otherEnd);
	if (err < 0) {
	    WARN << "error while TCP connection to (" << otherEnd << ")" << strerror (errno) << endl;
	    OS::SleepMillis(CONNECT_SLEEP_TIME);
	}
	else {
	    DB(4) << "\tConnected!" << endl;
	    return 0;
	}
    }
    WARN << "\tcould not connect to (" << otherEnd << ") giving up..." << endl;
    return -1;
}

int TCPTransport::_Connect_TCP(Socket *pSock, IPEndPoint *otherEnd) {
    struct sockaddr_in address;
    bzero(&address, sizeof(struct sockaddr_in));

    if (OS::CreateSocket(pSock, (int) PROTO_TCP) < 0) {
	DB(1) << "_Connect_TCP(): Could not create socket for [" 
	      << otherEnd->ToString() << "]" << endl;
	return -1;
    }

#if 1
    // ensure that this socket gets binded to our IP address
    // (in case there are multiple legitimate ones)
    struct sockaddr_in srcaddr;
    srcaddr.sin_family = AF_INET;
    srcaddr.sin_addr.s_addr = m_ID.m_IP;
    srcaddr.sin_port = 0; // let kernel choose port

    if (bind(*pSock, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) < 0) {
	WARN << "bind failed in connecting to (" 
	     << *otherEnd << ") giving up..." << endl;
	return -1;
    }
#endif

    otherEnd->ToSockAddr(&address); 

    if (OS::Connect(*pSock, (struct sockaddr *)&address, 
		    sizeof(address)) < 0) {
	DB(1) << "_Connect_TCP(): Could not connect to " 
	      << otherEnd->ToString() << " error:" << strerror (errno) << endl;
	OS::CloseSocket(*pSock);
	*pSock = -1;
	return -1;
    }

    // The first thing to transmit is our AppLevel ID so that the remote
    // host can identify this connection as a target of return packets

    uint32 ip   = m_ID.GetIP();
    uint16 port = htons( m_ID.GetPort() );
    RealNet::WriteBlock(*pSock, (byte *)&ip, sizeof(ip));
    RealNet::WriteBlock(*pSock, (byte *)&port, sizeof(port));

    return 0;
}

//
// sends the entire buffer out onto the socket
//
int TCPTransport::_Send_TCP(Socket fd, byte* buffer, uint32 length) {
    uint32 nWritten = 0;
    uint32 totalWritten = 0;

    if (!buffer || length <= 0) {
	WARN << "null buffer or zero buffersize" << endl;
	return -1;
    }

    DBG << "sending TCP " << length << " bytes" << endl;

    ASSERT((uint32)length < 0xFFFFFFFFUL);

    uint32 len = htonl( (uint32)length );

    if (RealNet::WriteBlock(fd, (byte *)&len, sizeof(uint32)) < 0)
	return -1;
    return RealNet::WriteBlock(fd, buffer, length);
}

//
// if the listen socket has data on it, accept the connection and register
// it in our connection hashtable
//
void TCPTransport::_RegisterNewTCPConnections(fd_set *isset) {
    if (m_ListenSocket <= 0)
	return;

    if (FD_ISSET(m_ListenSocket, isset)) {
	struct sockaddr_in address;
	int addrlen = sizeof(sockaddr_in);

	Socket newsock = 0;
	int ret = OS::Accept(&newsock, m_ListenSocket, 
			     (struct sockaddr *) &address, 
			     (socklen_t *) &addrlen);
	if (ret < 0 && errno == EINTR) {
	    // interrupted, just ignore this
	} else if (ret < 0) {
	    WARN << "Error while accepting a connection... " << endl;
	} else {
	    // The first things on the connection will be the app level ID
	    uint32 ip;
	    uint16 port;
	    // XXX FIXME - should not block!
	    RealNet::ReadBlock(newsock, (byte *)&ip, sizeof(ip));
	    RealNet::ReadBlock(newsock, (byte *)&port, sizeof(port));

	    IPEndPoint otherEnd(ip, ntohs(port));

	    DBG << "Registered new connection: " << otherEnd << endl;

	    Connection *connection = 
		new TCPConnection(this, newsock, &otherEnd);
	    connection->SetStatus(CONN_NEWINCOMING);

	    Lock();
	    Connection *old = m_AppConnHash.Lookup(&otherEnd);
	    if (old != NULL) {
		// This can happen if two nodes try to connect to each
		// other simultaneously (or close enough)
		DB(1) << "duplicate connection open for: " << otherEnd 
		      << endl;

		// We CAN NOT close either connection, because then we
		// risk losing data (e.g., the stuff that is in the
		// other guy's buffer or in our kernel buffer).
		//
		// For now, I'm just going to hack this and add the
		// new connection to the connectionlist but not the
		// hash. I hope this means we will just use one
		// connection for sending stuff and the other for
		// receiving, which should be fine.

		m_ConnectionList.push_back(connection);

		/*
		// hm... already had a connection to this guy... to ensure
		// that we both just keep 1 connection open, keep the
		// connection initiated by the guy with lowest SID
		//
		// XXX race condition here -- if there is still data on
		// the old connection, we should read it before closing it!
		static less_SID op;
		if (op(*old->GetAppPeerAddress(), GetAppID())) {
		// the other guy will close his connection -- so ignore
		// this new connection request.
		OS::CloseSocket(connection->GetSocket());
		delete connection;
		Unlock();
		return;
		} else {
		// I am the higher sid, therefore, I should keep the new
		// connection and delete my old one.
		OS::CloseSocket(old->GetSocket());
		old->SetStatus(CONN_CLOSED);

		m_AppConnHash.Flush(&otherEnd);
		m_ConnectionList.remove(old);
		delete old;
		}
		*/
	    } else {
		m_ConnectionList.push_back(connection);
		// Make this connection bidirectional -- i.e., when we have
		// data to send to this guy, reuse this connection. No point
		// to initiate a new TCP connection if they aready connected
		// to us
		m_AppConnHash.Insert(&otherEnd, connection);
	    }

	    Unlock();
	}
    }

}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
