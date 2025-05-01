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

#include <util/Utils.h>
#include <util/OS.h>
#include <mercury/NetworkLayer.h>
#include <wan-env/UDPTransport.h>

// int UDPTransport::MAX_UDP_MSGSIZE = OS::GetMaxDatagramSize();
int UDPTransport::MAX_UDP_MSGSIZE = 4 * 1024; 

void UDPTransport::StartListening()
{
    struct sockaddr_in	server_address;
    Socket sock;

    int err = OS::CreateSocket(&sock, (int) PROTO_UDP ); 
    if (err < 0) {
	ASSERT(0);
    }
    m_ListenSocket = sock;

    /// XXX: PORTABILITY 
    int n;
    if ((n = fcntl (m_ListenSocket, F_GETFL)) < 0
	|| fcntl (m_ListenSocket, F_SETFL, n | O_NONBLOCK) < 0) {
	perror ("fcntl");
	Debug::die ("error while setting the socket to nonblocking");
    }

    int on = 1;
    if (OS::SetSockOpt(sock, SOL_SOCKET, SO_TIMESTAMP, 
		       (char *) &on, sizeof(on)) < 0 ) {
	perror ("setsockopt");
	Debug::die ("error while setting SO_TIMESTAMP");
    }

    memset((char *)&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = m_ID.m_IP; // already in network order
    //server_address.sin_addr.s_addr = INADDR_ANY;   // XXX !HACK! - Ashwin [07/11/2005]
    server_address.sin_port = htons(m_ID.m_Port);

    if (bind(sock, (struct sockaddr *)&server_address, 
	     sizeof(server_address)) < 0 ) {
	perror("bind");
	Debug::die ("StartListening: could not bind server socket to port [%d]\n", m_ID.m_Port);
    }

    DB(1) << "Started [PROTO_UDP] server at port " 
	  << m_ID.m_Port << " successfully..." << endl;
    return;
}

void UDPTransport::StopListening()
{
    if (m_ListenSocket > 0)
	OS::CloseSocket(m_ListenSocket);

    _ClearConnections();
}

int UDPTransport::FillReadSet(fd_set *tofill)
{
    FD_SET(m_ListenSocket, tofill);
    return m_ListenSocket;
}

void UDPTransport::DoWork(fd_set *isset, u_long timeout_usecs)
{
    // XXX Some way to garbage collect stale connections


    // periodically clean up closed connections
    TimeVal now = TimeNow ();
    TimeVal rcv;

    PERIODIC2(1000, now, _CleanupConnections() );

    unsigned long long stoptime = CurrentTimeUsec() + (unsigned long long) MAX(timeout_usecs, 10*1000);

    int i = 0;

    while (true) {
	if (i++ >= MAX_PKTS_SERVICE)
	    break;

	if (_ServiceOnce (&rcv) == EAGAIN) 
	    break;

	if (CurrentTimeUsec() > stoptime)
	    break;
	if (!RealNet::IsDataWaiting(m_ListenSocket))
	    break;

	//if (rcv > now) 
	//    break;
    }

    /*
    if (RealNet::IsDataWaiting(m_ListenSocket)) {
	WARN << "more data is waiting; serviced: " << i 
	     << " now=" << now << " rcv=" << rcv << endl;
    }
    */
    // if at this point, it would be good to figure out the 
    // size of the kernel's recv queue, just to see how we 
    // are performing.
}

extern bool doPrint;

int /* errno */
UDPTransport::_ServiceOnce(TimeVal *tv)
{
    *tv = TIME_NONE;
    int error = 0;

    IPEndPoint fromWhom;
    Packet *pkt = new Packet(MAX_UDP_MSGSIZE);

    START(_ServiceOnce::ReadDatagram);
    int len = RealNet::ReadDatagramTime (m_ListenSocket, 
					 &fromWhom, 
					 pkt->GetBuffer (),
					 pkt->GetMaxSize (), 
					 tv);
    error = errno;

    STOP(_ServiceOnce::ReadDatagram);

    pkt->ResetBufPosition ();
    pkt->IncrBufPosition (len);

    if (len < 0) {
	if (error != EAGAIN) 
	    WARN << "UDP Socket on port " << m_ID.m_Port
		 << " error: " << strerror(error) << endl;
	delete pkt;
	return error; 
    }

    Lock();
    UDPConnection *conn = (UDPConnection *)m_AppConnHash.Lookup(&fromWhom);
    Unlock();

    if (conn == NULL) {
	Socket sock = m_ListenSocket;

	/* This will assign a random socket to the peer... don't
	   want that...
	   if ( _Connect_UDP(&sock, &fromWhom) < 0 ) {
	   WARN << "_Connect_UDP Failed! " << strerror(errno)
	   << endl;
	   delete pkt;
	   continue;
	   }
	*/

	// XXX -- currently we assume that the app-level ID is the
	// ipaddr:port that this packet was sent from...
	conn = CreateConnection(sock, &fromWhom);
	conn->SetStatus(CONN_NEWINCOMING);
	AddConnection(conn);

	DBG << "new connection from: " << fromWhom << endl;
    }

    NOTE(UDPTransport::QUEUESIZE, conn->Size());
    if (conn->Size() > APP_QUEUE_SIZE) {
	doPrint = true;

	TimeVal now = TimeNow ();
	PERIODIC2(1000, now, {
	    WARN << "Dropping packet from: " 
		 << conn->GetAppPeerAddress() << endl;
	});

	// Jeff: If we print this so often, we slow the game down so much
	// it can not possibly recover.
	//WARN << "Dropping packet: " << conn->GetAppPeerAddress() << endl;

	switch (QUEUING_DISCIPLINE) {
	case Q_DROPTAIL: {
	    delete pkt;
	    return len;
	}
	case Q_DROPHEAD: {
	    PacketInfo info = conn->Pop();
	    delete info.pkt;
	    break;
	}
	default:
	    WARN << "Unknown queuing discipline: "
		 << QUEUING_DISCIPLINE << endl;
	    ASSERT(0);
	}
    }

    DB(20) << "servicing: " << *conn->GetAppPeerAddress() << endl;

    // *tv = OS::GetSockTimeStamp(m_ListenSocket); -- no longer needed
    conn->Insert(pkt, *tv);

    return 0;
#undef RETURN
}

UDPConnection *UDPTransport::CreateConnection(Socket sock, 
					      IPEndPoint *otherEnd)
{
    return new UDPConnection(this, sock, otherEnd);
}

void UDPTransport::AddConnection(Connection *connection)
{
    Lock();
    m_ConnectionList.push_back(connection);
    // I know the other end point here, since I am actively connecting. 
    // So this is the ONLY address I will ever use to identify "this" 
    // particular connection 
    m_AppConnHash.Insert(connection->GetAppPeerAddress(), connection);
    Unlock();
}

void UDPTransport::RemoveConnection(Connection *connection)
{
    Lock();
    m_AppConnHash.Flush( connection->GetAppPeerAddress() );
    m_ConnectionList.remove(connection);
    Unlock();
}

Connection *UDPTransport::GetConnection(IPEndPoint *toWhom)
{
    START(UDPTransport::GetConnection);

    Lock();
    UDPConnection *connection = (UDPConnection *)m_AppConnHash.Lookup(toWhom);
    Unlock();

    if (!connection || 
	connection->GetStatus() == CONN_CLOSED || 
	connection->GetStatus() == CONN_ERROR) {

	Socket sock = m_ListenSocket;

	/* This changes the send port -- don't want that...
	   int err = _Connect_UDP(&sock, toWhom);
	   if (err < 0) {
	   Unlock();
	   return NULL; // XXX need to report err somehow
	   }
	*/

	// remove old connection from hash and list
	if (connection)
	    RemoveConnection(connection);

	// create new connection and insert into hash and list
	connection = CreateConnection(sock, toWhom);

	DB(20) << "new connection to: " << toWhom << endl;
	AddConnection(connection);
    }

    STOP(UDPTransport::GetConnection);

    return connection;
}

ConnStatusType UDPTransport::GetReadyConnection(Connection **connp)
{
    START(UDPTransport::GetReadyConnection);

    ConnStatusType ret = CONN_NOMSG;

    // When running many nodes in the same machine, the DoWork() thread
    // may not be serviced fast enough for the regression tests. So 
    // if there is data to be read, make sure to make some progress here

#if 0
    if ( RealNet::IsDataWaiting(m_ListenSocket) ) {
	//DB(5) << "Looks like the Worker thread is behind... servicing here"
	//      << endl;
	_ServiceOnce();
    }
#endif
    Lock();

    NOTE(UDP_CONN_LIST_COUNT, m_ConnectionList.size());

    ///// can do some prioritization here... 
    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); iter++) {
	UDPConnection *connection = (UDPConnection *)(*iter);

	if (connection->m_Queue.size() > 0) {
	    *connp = connection;
	    ret = connection->GetStatus();

	    // we serviced this fellow now - let the other guys have a chance!
	    // put this connection at the END of the list.
	    m_ConnectionList.erase(iter);
	    m_ConnectionList.push_back(connection);

	    if (ret == CONN_NEWINCOMING) {
		(*connp)->SetStatus(CONN_OK);
	    }

	    break;
	}
    }

    Unlock();

    STOP(UDPTransport::GetReadyConnection);

    return ret;
}

void UDPTransport::CloseConnection(IPEndPoint *target) {
    Lock();
    Connection *connection = m_AppConnHash.Lookup(target);
    Unlock();
    if (!connection)
	return;

    connection->SetStatus(CONN_CLOSED);
}

//
// A UDP connect. This basically allows normal "send" calls instead of 
// using "sendto".
//
int UDPTransport::_Connect_UDP(Socket *pSock, IPEndPoint *otherEnd) {
    struct sockaddr_in address;
    bzero(&address, sizeof(struct sockaddr_in));

    if (OS::CreateSocket(pSock, (int) PROTO_UDP) < 0) {
	WARN << "_Connect_UDP(): not create socket for [" 
	     << otherEnd->ToString() << "]" << endl;
	return -1;
    }
    otherEnd->ToSockAddr(&address); 

    if (OS::Connect(*pSock, (struct sockaddr *)&address, sizeof(address)) < 0) {
	WARN << "_Connect_UDP(): Could not connect to [" 
	     << otherEnd->ToString() << "]" << endl;
	OS::CloseSocket(*pSock);
	*pSock = -1;
	return -1;
    }
    return 0;
}

int UDPTransport::_Send_UDP(IPEndPoint *otherEnd,
			    byte* buffer, uint32 length) {
    if (length > (uint32)MAX_UDP_MSGSIZE) {
	// can't send, too big!
	return -1;
    }

    return RealNet::WriteDatagram(m_ListenSocket, otherEnd, buffer, length);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
