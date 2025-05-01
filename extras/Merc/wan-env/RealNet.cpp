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

#include <netinet/tcp.h>
#include <wan-env/RealNet.h>
#include <mercury/IPEndPoint.h>
#include <mercury/NetworkLayer.h>
#include <mercury/Scheduler.h>
#include <mercury/Utils.h>
#include <mercury/MercuryNode.h>
#include <mercury/Event.h>
#include <mercury/RoutingLogs.h>
#include <mercury/Packet.h>
#include <mercury/Message.h>
#include <mercury/Timer.h>
#include <wan-env/DelayedTransport.h>
#include <util/OS.h>
#include <util/debug.h>
#include <mercury/ObjectLogs.h>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

IPEndPoint s_HackyBootstrapIP;

///////////////////////////////////////////////////////////////////////////////

RealNetWorker *RealNet::m_WorkerThread = NULL;
Mutex          RealNet::m_Lock;
TransportMap   RealNet::m_Transports;
fd_set         RealNet::m_ReadFileDescs;
struct pollfd  RealNet::m_PollFileDescs[MAX_FILE_DESC];

void RealNet::InitWorker()
{
    FD_ZERO(&m_ReadFileDescs);
    m_WorkerThread = new RealNetWorker();
    m_WorkerThread->Start();
}

void RealNet::InterruptWorker() {
    if (m_WorkerThread) 
	m_WorkerThread->Interrupt();
}

Socket RealNet::FillReadSet(fd_set *tofill)
{
    Socket maxfd = 0;

    Lock();
    for (TransportMapIter it = m_Transports.begin();
	 it != m_Transports.end(); it++) {
	Transport *t = it->second;
	maxfd = MAX( t->FillReadSet(tofill), maxfd );
    }
    Unlock();

    return maxfd;
}

void RealNet::DoWorkUsec (u_long usecs)
{
    TimeVal selectTimeout = {
	usecs / USEC_IN_SEC,
	usecs % USEC_IN_SEC 
    };

    u_long msecs = (usecs / USEC_IN_MSEC);
    NOTE(REALNET:TIMEOUT, msecs);

    START(RealNet::DoWork);

    START(RealNet::SELECT);

    unsigned long long t1 = CurrentTimeUsec ();
    _DoSelect(selectTimeout);
    u_long consumed = (u_long) (CurrentTimeUsec () - t1);
    STOP(RealNet::SELECT);

    // XXX Jeff: This starves incoming tcp connections!
    // all this timeout stuff is whack. in *all* places it is used we *must*
    // ensure we make progress -- that means we can *only* skip something
    // due to a timeout if it is a repeated, non-essential operation.
    // -- better we should just switch over to the async version of 
    // realnet sooner rather than later. :P
    //if (consumed < usecs) {	
	START(RealNet::Transports);
	Lock();

	int total = 0;
	for (TransportMapIter it = m_Transports.begin(); it != m_Transports.end(); ++it) {
	    total += it->second->GetPriority();
	}

	for (TransportMapIter it = m_Transports.begin(); it != m_Transports.end(); ++it) {
	    Transport *t = it->second;
	    int prio = t->GetPriority();

	    u_long alloc = (u_long) ((float) prio * (usecs - consumed)/(float) total);
	    t->DoWork(&m_ReadFileDescs, alloc);
	}
	Unlock();
	STOP(RealNet::Transports);
	//}

    STOP(RealNet::DoWork);

    //START(RealNet::Yield);
    //Thread::Yield();
    //STOP(RealNet::Yield);
}

void RealNet::DoWork (u_long timeout)
{
    if (timeout == 0) 
	timeout = SELECT_TIMEOUT;

    DoWorkUsec (timeout * USEC_IN_MSEC);
}

void RealNet::_DoSelect(TimeVal timeout) {
    Socket maxfd = 0;

    FD_ZERO(&m_ReadFileDescs);

    Lock();
    START(DoSelect::FillReadSet);
    for (TransportMapIter it = m_Transports.begin();
	 it != m_Transports.end(); it++) {
	Transport *t = it->second;
	maxfd = MAX( t->FillReadSet(&m_ReadFileDescs), maxfd );
    }
    STOP(DoSelect::FillReadSet);
    Unlock();

    if (g_Preferences.use_poll) {
	int nfds = 0;

	struct pollfd pfd;
	for (int i = 0; i <= maxfd; i++) {
	    if (FD_ISSET (i, &m_ReadFileDescs)) {
		pfd.fd = i;
		pfd.events = POLLIN;

		m_PollFileDescs[nfds] = pfd;
		nfds++;
	    }
	}

	u_long millis = timeout.tv_sec * MSEC_IN_SEC + timeout.tv_usec / USEC_IN_MSEC;
	int ret = poll (m_PollFileDescs, nfds, millis);

	// convert poll results back to select-style fd_set results 
	// so the other code can continue to work just fine - Ashwin [03/18/2005]
	FD_ZERO (&m_ReadFileDescs);
	if (ret <= 0) {
	    if (ret < 0 && errno == EINTR) {
		return; // interrupted, just loop again
	    } else if (ret < 0) 
		WARN << "poll error: " << strerror (errno) << endl;
	    return;
	}

	// INFO << "using poll: " << ret << " fds have data " << endl;
	for (int i = 0; i < nfds; i++) {
	    if (m_PollFileDescs[i].revents & POLLIN)
		FD_SET (m_PollFileDescs[i].fd, &m_ReadFileDescs);
	}
    }
    else {
	int ret = select((int) (maxfd + 1), &m_ReadFileDescs, 0, 0, &timeout);

	if (ret < 0 && errno == EINTR) {
	    FD_ZERO(&m_ReadFileDescs); // clear these so we try again
	    return; // interrupted, just loop again
	} else if (ret < 0) {
	    FD_ZERO(&m_ReadFileDescs); // clear these so we try again
	    WARN << "select error: " << strerror(errno) << endl;
	}
    }
}

Transport *RealNet::_LookupTransport(const ProtoID& proto)
{
    Lock();
    TransportMapIter it = m_Transports.find(proto);
    ASSERT(it != m_Transports.end());
    Transport *t = it->second;
    Unlock();
    return t;
}

///////////////////////////////////////////////////////////////////////////////

class AggregateMeasurementTimer : public Timer {
    RealNet *m_RealNet;
public:
    AggregateMeasurementTimer (RealNet *realnet) : Timer (0), m_RealNet (realnet) {}
    void OnTimeout () {
	m_RealNet->DoAggregateLogging ();
	_RescheduleTimer (g_MeasurementParams.aggregateLogInterval);
    }
};

void RealNet::DoAggregateLogging ()
{
    AggregateMessageEntry ent (m_InboundAggregates, m_OutboundAggregates);
    LOG(AggregateMessageLog, ent);

    m_InboundAggregates.clear ();
    m_OutboundAggregates.clear ();
}

class SendRecvNoter : public Timer { 
    RealNet *m_RealNet;
public:
    SendRecvNoter (RealNet *realnet) : Timer (0), m_RealNet (realnet) {}
    void OnTimeout () {
	m_RealNet->UpdateSendRecvStats ();
	_RescheduleTimer (5000);
    }
};

sint64 diff(struct timeval& a, struct timeval& b) {
    return ((sint64)a.tv_sec - (sint64)b.tv_sec)*USEC_IN_SEC + 
	((sint64)a.tv_usec - (sint64)b.tv_usec);
}

void RealNet::UpdateSendRecvStats () 
{
    TimeVal now = m_Scheduler->TimeNow ();

    double tput_pps;

    // xxx Jeff: should really be another option i guess...
    if (!g_Preferences.nosuccdebug) {
    tput_pps = (double)(m_RecvMessages) / ((double)(diff(now, m_StartTime))/1000000.0);
    cerr << " recv tput: " << tput_pps << " pps" << endl;
    tput_pps = (double)(m_SentMessages) / ((double)(diff(now, m_StartTime))/1000000.0);
    cerr << " send tput: " << tput_pps << " pps" << endl;
    }

    m_SentMessages = 0;
    m_RecvMessages = 0;

    m_StartTime = m_Scheduler->TimeNow ();
}
//
// We assume here that socket handles returned by the kernel are never "0".
//
RealNet::RealNet(Scheduler *scheduler, IPEndPoint appID, bool recordBwidthUsage, uint32 windowSize) :
    m_Scheduler (scheduler),
    m_AppID (appID), m_RecordBandwidthUsage (recordBwidthUsage), 
    m_WindowSize (windowSize), m_EnableMessageLog (false), 
    m_SentMessages (0), m_RecvMessages (0)
{
    s_HackyBootstrapIP = IPEndPoint (g_Preferences.bootstrap);

    if (g_MeasurementParams.aggregateLog)
	m_Scheduler->RaiseEvent (new refcounted<AggregateMeasurementTimer> (this), m_AppID, g_MeasurementParams.aggregateLogInterval);

    m_StartTime = m_Scheduler->TimeNow ();
    m_Scheduler->RaiseEvent (new refcounted<SendRecvNoter>(this), m_AppID, 5000);

#ifdef ENABLE_REALNET_THREAD
    if (m_WorkerThread == NULL) {
	InitWorker();
    }
#endif
}

//
// clear the hash table and the connection list.
// delete the connections only once - since the entries in the hash and list are the same!
// 
RealNet::~RealNet() {
    StopListening();
}

//
// create socket depending on what protocol the user wants (tcp/udp)
// bind to a port; and start listening
//
void RealNet::StartListening(TransportType p) {
    ProtoID proto(m_AppID, p);

    Lock();
    TransportMapIter it = m_Transports.find(proto);
    ASSERT(it == m_Transports.end());

    Transport *t = Transport::Create(this, proto.proto, proto.id);

    if (g_Preferences.latency) {
	// enable artificial latency?
	t = new DelayedTransport(t);
    }

    m_Transports[proto] = t;
    Unlock();

    m_Protos.insert(proto);

    t->StartListening();
    InterruptWorker(); // new sockets added, interrupt to add to select
}

void RealNet::StopListening() {
    for (ProtoIDSetIter it = m_Protos.begin();
	 it != m_Protos.end(); it++) {
	Transport *t = _LookupTransport(*it);

	Lock();
	m_Transports.erase(*it);
	Unlock();

	t->StopListening();
	delete t;
    }
    m_Protos.clear();
}

int RealNet::SendMessage(Message *msg, IPEndPoint *toWhom, TransportType p) {
    //START( SendMessage::1 );

    ProtoID proto(m_AppID, p);

    // Jeff 6/7/2004: HACK... This will break non-merc stuff if not set.
    if (msg->IsMercMsg())
	msg->sender = m_AppID;	

    Transport *t = _LookupTransport(proto);

    Connection *connection = t->GetConnection(toWhom);
    if (connection == NULL) {
	// connect failed
	return -1;
    }

    /*
      ASSERT( connection->GetProtocol() == p );
    */
    m_SentMessages++;

    msg->hopCount++;
    DB (7) << "sending msg " << msg << endl;

    //STOP( SendMessage::1 );

    //START( SendMessage::2::GetLength );
    int len = msg->GetLength();

    if (len > 65 * 1000) {
	WARN << " humonguous packet length in RealNet::SendMessage len=" << len << endl;
	WARN << msg << endl;
	ASSERT (len <= 65 * 1000);
    }

    //STOP( SendMessage::2::GetLength );
    int ret;
    ///// Message compression
    if (g_Preferences.msg_compress &&
	msg->sender != *toWhom && 
	len > g_Preferences.msg_compminsz) {
	//START( SendMessage::2::Compress );
	MsgCompressed cmsg = MsgCompressed(msg);
	int clen = cmsg.GetLength();
	//STOP( SendMessage::2::Compress );
	// only send compressed if compressed message is smaller
	if (len > clen)
	    ret = _SendMessage(&cmsg, connection);
	else
	    ret = _SendMessage(msg, connection);
    } else {
	ret = _SendMessage(msg, connection);
    }
    /////

    if (msg->GetType() != MSG_PUB || msg->GetType() != MSG_SUB) {
	// XXX HACK: for message structs sent multiple times
	// msg->hopCount--;
	msg->nonce = (uint32)(drand48()*0xFFFFFFFFUL);
    }

    if (ret < 0) {
	WARN << "failed to send to " << *toWhom << " of " << msg 
	     << " errno=" << errno << "(" << strerror(errno) << ")" << endl;
    }
    return ret;
}

MsgType __GetSubTypeForLogging (Message *msg) {
    Interest *in = ((MsgSubscription *) msg)->GetInterest ();

    if (msg->hopCount == 1) 
	return MSG_SUB_NOTROUTING;
    else if (((MsgSubscription *) msg)->IsRoutingModeLinear ())
	return MSG_SUB_LINEAR;
    else
	return MSG_SUB;
}

MsgType __GetPubTypeForLogging(Message *msg) {
    Event *ev = ((MsgPublication *) msg)->GetEvent();

    // EventType t = ev->GetType();  // cant use this since app can override PointEvent

    // dynamic_cast looks ugly here; but it's okay since we 
    // do this for logging only

    if (dynamic_cast<PointEvent *>(ev)) {
	if (ev->IsMatched())
	    return MSG_MATCHED_PUB;
	else 
	    return MSG_PUB;
    }
    else {
	if (ev->IsMatched())
	    return MSG_RANGE_MATCHED_PUB;
	else {
	    if (msg->hopCount == 1)
		return MSG_RANGE_PUB_NOTROUTING;
	    else if (((MsgPublication *) msg)->IsRoutingModeLinear ()) 
		return MSG_RANGE_PUB_LINEAR;
	    else
		return MSG_RANGE_PUB;
	}
    }

    // Should never come here!
    return MSG_PUB;
}

static byte __GetMsgType (Message *msg)
{
    byte type = msg->GetType ();

    // We want to know the type of the original message for compressed ones
    if (type == MSG_COMPRESSED)
	type = ((MsgCompressed *)msg)->orig->GetType();

    if (type == MSG_PUB)
	{
	    if (msg->GetType() == MSG_COMPRESSED)
		type = __GetPubTypeForLogging(((MsgCompressed *) msg)->orig);
	    else
		type = __GetPubTypeForLogging(msg);
	}
    if (type == MSG_SUB)
	{
	    if (msg->GetType() == MSG_COMPRESSED)
		type = __GetSubTypeForLogging(((MsgCompressed *) msg)->orig);
	    else
		type = __GetSubTypeForLogging(msg);

	}

    return type;
}

static Packet* _MakePacket (Message *msg)
{
    int len = msg->GetLength ();
    Packet *pkt = new Packet (len);

    msg->Serialize (pkt);

    // if this isn't true, some component of the msg
    // allocated too much or too little space!
    ASSERT (pkt->GetBufPosition () == len);
    return pkt;
}

int RealNet::_SendMessage(Message *msg, Connection *connection) {
    int err = 0;
    Packet *pkt = 0;
    //try {
    //START( _SendMessage::Packet );

    pkt = _MakePacket (msg);
    ASSERT ((uint32) pkt->GetUsed () == msg->GetLength ());

    // pkt = new Packet(msg);
    //STOP( _SendMessage::Packet );
    //} catch (...) {
    //    WARN << "RealNetHandler::SendMessage(): could not allocate packet..." << endl;
    //    return -1;
    //}

    //START( _SendMessage::Logging );

    TimeVal& now = m_Scheduler->TimeNow ();

    ///// Logging
    bool logworthy = 		// Ignore messages from the bootstrap server
	*connection->GetAppPeerAddress() != s_HackyBootstrapIP &&
	// Ignore messages we send to ourselves
	*connection->GetAppPeerAddress() != m_AppID;

    if (logworthy) {
	if (m_EnableMessageLog) {
	    if (!g_MeasurementParams.aggregateLog) {
		MessageEntry ent(MessageEntry::OUTBOUND, 
				 connection->GetProtocol() == PROTO_TCP ?  MessageEntry::PROTO_TCP : MessageEntry::PROTO_UDP,
				 msg->nonce, __GetMsgType (msg), pkt->GetUsed (), msg->hopCount);

		LOG(MessageLog, ent);
	    }
	    else {
		byte type = __GetMsgType (msg);

		AggMeasurement *msr = NULL;

		MeasurementMap::iterator it = m_OutboundAggregates.find (type);
		if (it == m_OutboundAggregates.end ()) {
		    m_OutboundAggregates.insert (MeasurementMap::value_type (type, AggMeasurement ()));			
		    msr = &( (m_OutboundAggregates.find (type))->second );
		}
		else {
		    msr = & (it->second);
		}

		msr->hopcount += msg->hopCount;
		msr->size += pkt->GetUsed ();
		msr->nsamples++;
	    }
	}
    }
    /////

    ///// Bandwidth usage tracking
    if (m_RecordBandwidthUsage) {
	RecordOutbound(pkt->GetUsed (), now);
    }

    //STOP( _SendMessage::Logging );

    /// Ideally, we should be really logging this JUST when the packet goes into the
    /// kernel, but since we are not using CBR, we should be happy. -- ASHWIN [10/11/2004]
    if (g_MeasurementParams.enabled && !g_MeasurementParams.aggregateLog) {
	int type = msg->GetType();
	Message *omsg = msg;

	// We want to know the type of the original message for compressed ones
	if (type == MSG_COMPRESSED) {
	    type = ((MsgCompressed *)msg)->orig->GetType();
	    omsg = ((MsgCompressed *)msg)->orig;
	}

	if (type == MSG_PUB)
	    {
		type = __GetPubTypeForLogging(omsg);
		if (type != MSG_MATCHED_PUB && type != MSG_RANGE_MATCHED_PUB)
		    {
			DiscoveryLatEntry ent(DiscoveryLatEntry::PUB_ROUTE_SEND, 
					      omsg->hopCount, ((MsgPublication *) omsg)->GetEvent()->GetNonce());
			LOG(DiscoveryLatLog, ent);
		    }
	    }
	else if (type == MSG_SUB)
	    {
		DiscoveryLatEntry ent(DiscoveryLatEntry::SUB_ROUTE_SEND, 
				      omsg->hopCount, ((MsgSubscription *) omsg)->GetInterest()->GetNonce());
		LOG(DiscoveryLatLog, ent);
	    }
    }
    /// MEASUREMENT
    return connection->SendMessage(pkt);
}

//
// lookup the socket corresponding to the endpoint
// and close the socket.
//
void RealNet::CloseConnection(IPEndPoint *otherEnd, TransportType p) {
    ProtoID proto(m_AppID, p);

    Transport *t = _LookupTransport(proto);
    t->CloseConnection(otherEnd);
}


//
// if tcp, poll for all connections. try to construct message from the
// data present at any of the connections. (possible modifications =
// greater control, timeouts, etc.)  also, if data is there on listening
// socket, accept connection and go into loop
//
// if udp, poll for data, and get datagram.  configurations for
// reliability support
//
// ALL THE ABOVE is transparently managed by the connection class
// beneath the "Connection::ReadMessage()" method. In this method, we go
// through the connections, and schedule their servicing properly.
//
ConnStatusType RealNet::GetNextMessage(IPEndPoint *ref_fromWhom, Message **ref_msg) 
{
    ConnStatusType status = CONN_NOMSG;
    Connection *connection = 0;

    TimeVal now = m_Scheduler->TimeNow ();

    // XXX not sure what could be more efficient :P
    // XXX TODO: be more efficient here!

    // XXX: Jeff: this is borked. if lots of messages arrive on the first
    // transport, the others will never be serviced! for now just
    // hackily randomize the order to prevent starvation...
    vector<ProtoID> perm;
    for (ProtoIDSetIter it = m_Protos.begin(); it != m_Protos.end(); it++) {
	perm.push_back(*it);
    }
    random_shuffle(perm.begin(), perm.end());

    Transport *t = NULL;

    for (vector<ProtoID>::iterator it = perm.begin(); it != perm.end(); it++) {
    //for (ProtoIDSetIter it = m_Protos.begin(); it != m_Protos.end(); it++) {
	t = _LookupTransport(*it);

	status = t->GetReadyConnection(&connection);
	if (status != CONN_NOMSG) {
	    DB(10) << "found a message for transport " << g_TransportProtoStrings[t->GetProtocol()] << endl;
	    break;
	}
    }

    if (!connection) {
	return CONN_NOMSG;
    }

    ConnStatusType ret = CONN_NOMSG;

    ASSERT(connection);
    *ref_fromWhom = *connection->GetAppPeerAddress();
    *ref_msg = 0;

    // Try to read a message from this connection
    switch (status) {
    case CONN_CLOSED:
	ret = CONN_CLOSED;
	t->CloseConnection(connection->GetAppPeerAddress());
	break;
    case CONN_ERROR:
	ret = CONN_ERROR;
	t->CloseConnection(connection->GetAppPeerAddress());
	break;
    case CONN_NEWINCOMING:
    case CONN_OK: {
	*ref_msg = connection->GetLatestMessage();

	if (*ref_msg == NULL) {
	    DB(1) << "Invalid packet read" << endl;
	    ret = CONN_ERROR;
	    break;
	}

	/*
	  ASSERT( t->GetProtocol() == connection->GetProtocol() );
	  ASSERT( *connection->GetAppPeerAddress() == 
	  (*ref_msg)->sender ||
	  !(*ref_msg)->IsMercMsg() && ((MsgApp *)(*ref_msg))->IsForwarded() );
	*/
	m_RecvMessages++;

	uint32 len = (*ref_msg)->GetLength();
	bool logworthy = 			 // Ignore messages from the bootstrap server
	    s_HackyBootstrapIP != (*ref_msg)->sender &&
	    // Mercury Messages (pubs, subs) can be sent to ourselves...
	    m_AppID != (*ref_msg)->sender;

	///// Logging
	if (logworthy) {
	    if (m_EnableMessageLog) {
		Message *msg = *ref_msg;

		if (!g_MeasurementParams.aggregateLog) {
		    MessageEntry ent(MessageEntry::INBOUND, 
				     connection->GetProtocol() == PROTO_TCP ?  MessageEntry::PROTO_TCP : MessageEntry::PROTO_UDP,
				     msg->nonce, __GetMsgType (msg), len, msg->hopCount);

		    LOG(MessageLog, ent);
		}
		else {
		    byte type = __GetMsgType (msg);
		    AggMeasurement *msr = NULL;

		    MeasurementMap::iterator it = m_InboundAggregates.find (type);
		    if (it == m_InboundAggregates.end ()) {
			m_InboundAggregates.insert (MeasurementMap::value_type (type, AggMeasurement ()));			
			msr = &( (m_InboundAggregates.find (type))->second );
		    }
		    else {
			msr = & (it->second);
		    }

		    msr->hopcount += msg->hopCount;
		    msr->size += len;
		    msr->nsamples++;
		}
	    }
	}
	/////

	if (m_RecordBandwidthUsage) {
	    RecordInbound(len, now);
	}

	//// Decompression
	if ((*ref_msg) && (*ref_msg)->GetType() == MSG_COMPRESSED) {
	    MsgCompressed *cmsg = (MsgCompressed *)*ref_msg;
	    *ref_msg = cmsg->orig;
	    (*ref_msg)->recvTime = cmsg->recvTime;
	    delete cmsg;
	}
	/////

	DB(6) << "Read Complete: " << *ref_msg << endl;

	ret = status;
	break;
    }
    default:
	WARN << "BUG: should never be here..." << endl;
	ASSERT(0);
    }

    return ret;
}

//
// One could implement a message buffer pool as well! 
//
void RealNet::FreeMessage(Message *msg) {
    delete msg;
}

///////////////////////////////////////////////////////////////////////////////

bool RealNet::IsDataWaiting(Socket sock) {
    TimeVal t;
    t.tv_sec  = 0;
    t.tv_usec = 0;

    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);

    select((int) sock + 1, &ready, 0, 0, &t);
    if (FD_ISSET(sock, &ready) != 0)
	return true;
    else
	return false;
}

int RealNet::ReadDatagram(Socket sock, IPEndPoint *fromWhom,
			  byte *buffer, int length) {
    int error, junklen = sizeof(struct sockaddr_in);
    struct sockaddr_in junk; // can not be static, otherwise not thread safe!

    //START(REALNET::recvfrom);
    // XXX stick in OS::
    error = recvfrom(sock, (char *) buffer, length, 0, 
		     (struct sockaddr *) &junk, (socklen_t *) &junklen);
    //STOP(REALNET::recvfrom);

    if (error >= 0) {
	fromWhom->m_IP   = junk.sin_addr.s_addr;
	fromWhom->m_Port = ntohs(junk.sin_port);
    }
    return error;
}

int RealNet::ReadDatagramTime (Socket sock, IPEndPoint *fromWhom, 
			       byte *buffer, int length, TimeVal *tv)
{
    int error;
    struct msghdr msg;
    struct iovec iov;
    char ctrl[CMSG_SPACE(sizeof(struct timeval))];
    struct sockaddr_in junk;

    struct cmsghdr *cmsg = (struct cmsghdr *) &ctrl;
    msg.msg_control = (caddr_t) ctrl;
    msg.msg_controllen = sizeof(ctrl);
    msg.msg_name = &junk;
    msg.msg_namelen = sizeof (struct sockaddr_in);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    iov.iov_base = buffer;
    iov.iov_len = length;

    error = recvmsg (sock, &msg, 0);

    if (cmsg->cmsg_level == SOL_SOCKET &&
	cmsg->cmsg_type == SCM_TIMESTAMP &&
	cmsg->cmsg_len == CMSG_LEN (sizeof (struct timeval))) { 
	/* Copy to avoid alignment problems: */
	memcpy (tv, CMSG_DATA (cmsg), sizeof (struct timeval));

	// xxx: ick! but we have todo this here since kernel does not
	// know about slowdown!
	if (g_Slowdown > 1.0f) {
	    ApplySlowdown(*tv);
	}
    }

    if (error >= 0) { 
	fromWhom->m_IP = junk.sin_addr.s_addr;
	fromWhom->m_Port = ntohs (junk.sin_port);
    }

    return error;
}

bool RealNet::WaitForWritable(Socket sock, TimeVal *t) {
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
	
    select((int) sock + 1, 0, &ready, 0, t);
    if (FD_ISSET(sock, &ready) != 0)
        return true;
    else
        return false;
}

int RealNet::WriteDatagram(Socket sock, IPEndPoint *toWhom,
			   byte *buffer, int length) {
#ifndef ENABLE_REALNET_THREAD
    static TimeVal wait = { 0, 100*1000 }; // XXX HACK
    static int error;
    static struct sockaddr_in addr;
    static bool inited = false;
    if (!inited) {
	inited = true;
	bzero((void *) &addr, sizeof(struct sockaddr_in));
    }
#else
    int error;
    struct sockaddr_in addr; // can not be static, otherwise not thread safe!
    memset((void *) address, 0, sizeof(struct sockaddr_in));
#endif

    toWhom->ToSockAddr(&addr);

    //START(REALNET::sendto);
    // XXX stick this in OS::
    
    // XXX Jeff: this is a big BIG hack -- retries should be done at a higher
    // layer but none of the merc code or anything else checks send status!!
    do {
	// wait for socket to become writable
	if (!WaitForWritable(sock, &wait))
	    continue;
	error = sendto(sock, (char *) buffer, length, 0, 
		       (struct sockaddr *) &addr, sizeof(struct sockaddr_in));	
    } while (error < 0 && (errno == EAGAIN || 
			   errno == ENOBUFS || 
			   errno == EINTR || 
			   errno == ENOMEM));
    if (error < 0) {
	WARN << "errno: " << errno << " str: " << strerror(errno) << endl;
    }

    //STOP(REALNET::sendto);

    return error;
}

int RealNet::WriteBlock(Socket fd, byte *buffer, int length) {
    int totalWritten = 0;
    while (totalWritten < length) {
	int nWritten = OS::Write(fd, (const void *) (buffer + totalWritten), 
				 length - totalWritten);

	if (nWritten < 0 && errno == EINTR) {
	    continue; // ignore and try again
	}
	if (nWritten < 0) {
	    return -1;
	} else if (nWritten == 0)
	    return totalWritten;					  
	else
	    totalWritten += nWritten;
    }
    return totalWritten;
}

//
// CHECK: socket is known to have data - so this read wouldn't block.
// read as much as you can...
//
int RealNet::ReadNoBlock(Socket fd, byte *buffer, int length) {
    int ret;

    do {
	ret = OS::Read(fd, (const void *) buffer, length);
    } while (ret < 0 && errno == EINTR); // ignore and try again

    return ret;
}

//
// blocking read for a certain number of bytes
//
int RealNet::ReadBlock(Socket fd, byte *buffer, int length) {
    int         nRead = 0;
    int         totalRead = 0;

    while (totalRead < length) {
	nRead = OS::Read(fd, (const void *) (buffer + totalRead), length - totalRead);

	if (nRead < 0 && errno == EINTR)
	    continue; // ignore and try again
	else if (nRead < 0) 
	    return -1;
	else if (nRead == 0)
	    return totalRead;
	else
	    totalRead += nRead;
    }

    return totalRead;
}


///////////////////////////////////////////////////////////////////////////////
///// BANDWIDTH PREDICTION

void RealNet::RecordOutbound(uint32 size, TimeVal& now) 
{
    Measurement last;
    if (m_OutboundWindow.size() != 0) {
	last = m_OutboundWindow.back();
    }
    if (m_OutboundWindow.size() != 0 && now - last.time < BUCKET_SIZE) {
	m_OutboundWindow.back().size += size;
    } else {
	Measurement m;
	m.time = now;
	m.size = size;
	m_OutboundWindow.push_back(m);
    }
}

void RealNet::RecordInbound(uint32 size, TimeVal& now) 
{
    Measurement last;
    if (m_InboundWindow.size() != 0) {
	last = m_InboundWindow.back();
    }
    if (m_InboundWindow.size() != 0 && now - last.time < BUCKET_SIZE) {
	m_InboundWindow.back().size += size;
    } else {
	Measurement m;
	m.time = now;
	m.size = size;
	m_InboundWindow.push_back(m);
    }
}

bwidth_t RealNet::GetOutboundUsage(TimeVal& now)
{
    // XXX This should be in "DoWork()"
    while ( m_OutboundWindow.size() > 0 && 
	    m_OutboundWindow.front().time + m_WindowSize < now ) {
	m_OutboundWindow.pop_front();
    }

    double used = 0;
    for (WindowIter i = m_OutboundWindow.begin(); 
	 i != m_OutboundWindow.end(); i++) {
	used += i->size;
    }

    return (bwidth_t)(used/m_WindowSize)*MSEC_IN_SEC;
}

bwidth_t RealNet::GetInboundUsage(TimeVal& now)
{
    // XXX This should be in "DoWork()"
    while ( m_InboundWindow.size() > 0 && 
	    m_InboundWindow.front().time + m_WindowSize < now ) {
	m_InboundWindow.pop_front();
    }

    double used = 0;
    for (WindowIter i = m_InboundWindow.begin(); 
	 i != m_InboundWindow.end(); i++) {
	used += i->size;
    }

    return (bwidth_t)(used/m_WindowSize)*MSEC_IN_SEC;
}

///////////////////////////////////////////////////////////////////////////////
// Debugging

#ifdef ENABLE_TEST

#include <util/Thread.h>
#include <om/test/TestObject.h>
#include <om/test/TestAdaptor.h>

class TestThread : public Thread 
{
public:
    RealNet *net;
    IPEndPoint local;
    IPEndPoint target;

    TestThread(RealNet *net, IPEndPoint local, IPEndPoint target) :
	net(net), local(local), target(target) {}
};

class TestTCPSender : public TestThread 
{
public:

    TestTCPSender(RealNet *net, IPEndPoint local, IPEndPoint target) :
	TestThread(net, local, target) {}

    void Run() {
	GUID guid;
	guid.m_IP   = target.m_IP;
	guid.m_Port = target.m_Port;

	MsgAppRemoteUpdate msg(local);
	TestObject *obj = new TestObject(guid);
	TestAttributes attrs;
	for (int i=0; i<30; i++) {
	    char buf[16];
	    sprintf(buf, "attr%d", i);
	    TestAttribute a;
	    a.key = string(buf);
	    a.isNamed = true;
	    attrs.push_back(a);
	}
	g_TestAdaptor.SetAttributes(attrs);
	for (int i=0; i<30; i++) {
	    char buf[16];
	    char *blob = new char[1024];
	    sprintf(buf, "attr%d", i);
	    Value v = Value(1024, blob);
	    obj->SetAttribute(string(buf), v);
	}
	GConnection conn;
	obj->PackUpdate(&conn, DELTA_MASK_ALL);
	LazyBlob *blob = conn.NextDelta();
	ASSERT(blob);
	GUpdate *update = new GUpdate(true, guid);
	update->mask = DELTA_MASK_ALL;
	update->delta = blob;
	msg.updates[GUID_NONE] = update;

	DBG << "send: sending msg(" << target << "): " << &msg << endl;
	net->SendMessage(&msg, &target, NetworkLayer::PROTO_RELIABLE);
    }
};

class TestTCPReceiver : public TestThread 
{
public:

    bool gotMsg;

    TestTCPReceiver(RealNet *net, IPEndPoint local, IPEndPoint target) :
	TestThread(net, local, target) {}

    void Run() {
	while (!gotMsg) {
	    Message *msg;
	    IPEndPoint from;
	    ConnStatusType type = net->GetNextMessage(&from, &msg);
	    switch(type) {
	    case CONN_NEWINCOMING:
	    case CONN_OK:
		DBG << "recv: got msg: " << msg << endl;

		ASSERT(from == target);
		ASSERT(msg->type == MSG_APP_REMOTE_UPDATE);
		gotMsg = true;

		break;
	    case CONN_CLOSED:
		DBG << "connection closed from: " << from.ToString() << endl;
		ASSERT(false); // WTF
		break;
	    case CONN_ERROR:
		DBG << "connection error from: " << from.ToString() << endl;
		ASSERT(false); // WTF
		break;
	    case CONN_NOMSG:
		//DBG << "got nothing..." << endl;
		break;
	    default:
		break;
	    }
	} 
    }
};

void RealNet::Test(int argc, char **argv)
{
    DBG_INIT(&SID_NONE);
    IPEndPoint sender, receiver;
    sender.m_IP   = inet_addr("127.0.0.1");
    sender.m_Port = 20000;
    receiver.m_IP   = inet_addr("127.0.0.1");
    receiver.m_Port = 20001;

    RealNet *senderNet   = new RealNet(sender);
    RealNet *receiverNet = new RealNet(receiver);

    senderNet->StartListening(NetworkLayer::PROTO_RELIABLE);
    receiverNet->StartListening(NetworkLayer::PROTO_RELIABLE);

    TestTCPSender *senderThread = 
	new TestTCPSender(senderNet, sender, receiver);
    TestTCPReceiver *receiverThread = 
	new TestTCPReceiver(receiverNet, receiver, sender);

    receiverThread->Start();
    sleep(1);
    senderThread->Start();
    sleep(1);

    ASSERT(receiverThread->gotMsg);
    sleep(360);
}

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
