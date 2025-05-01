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

#include <zlib.h>
#include <mercury/Message.h>
#include <mercury/Utils.h>
#include <mercury/Histogram.h>
#include <mercury/Interest.h>
#include <mercury/Event.h>
#include <mercury/HubManager.h>
#include <util/Benchmark.h>
#include <mercury/Sampling.h>
#include <mercury/Peer.h>

MsgType MSG_INVALID, MSG_HEARTBEAT, MSG_LIVENESS_PING, MSG_LIVENESS_PONG, 
    MSG_JOIN_REQUEST,  MSG_JOIN_RESPONSE,  MSG_NOTIFY_SUCCESSOR,
    MSG_GET_PRED, MSG_PRED, MSG_GET_SUCCLIST, MSG_SUCCLIST,
    MSG_NBR_REQ, MSG_NBR_RESP, MSG_LINK_BREAK,
    MSG_PUB, MSG_LINEAR_PUB, MSG_ACK, MSG_SUB, MSG_LINEAR_SUB, MSG_SUB_LIST, MSG_TRIG_LIST,
    MSG_BOOTSTRAP_REQUEST, MSG_BOOTSTRAP_RESPONSE,

    MSG_SAMPLE_REQ, MSG_SAMPLE_RESP,
    MSG_POINT_EST_REQ, MSG_POINT_EST_RESP,

    MSG_LOCAL_LB_REQUEST, MSG_LOCAL_LB_RESPONSE,
    MSG_LEAVE_NOTIFICATION,
    MSG_LEAVEJOIN_LB_REQUEST, /* NO RESPONSE */
    MSG_LEAVEJOIN_DENIAL,
    MSG_LCHECK_REQUEST, MSG_LCHECK_RESPONSE,

    MSG_CB_ALL_JOINED, MSG_CB_ESTIMATE_REQ, MSG_CB_ESTIMATE_RESP,
    MSG_BLOB, MSG_COMPRESSED, MSG_PING,
    // XXX: Start
    MSG_MATCHED_PUB, MSG_RANGE_PUB, MSG_RANGE_MATCHED_PUB,
    MSG_RANGE_PUB_NOTROUTING, MSG_SUB_NOTROUTING,
    MSG_RANGE_PUB_LINEAR, MSG_SUB_LINEAR,
    //  XXX: End 	
    MSG_MERCURY_SENTINEL;

void RegisterMessageTypes() 
{
    MSG_INVALID = 0xff;
    MSG_HEARTBEAT = REGISTER_TYPE (Message, MsgHeartBeat);
    MSG_LIVENESS_PING = REGISTER_TYPE (Message, MsgLivenessPing);
    MSG_LIVENESS_PONG = REGISTER_TYPE (Message, MsgLivenessPong);

    MSG_JOIN_REQUEST = REGISTER_TYPE (Message, MsgJoinRequest);
    MSG_JOIN_RESPONSE = REGISTER_TYPE (Message, MsgJoinResponse);
    MSG_NOTIFY_SUCCESSOR = REGISTER_TYPE (Message, MsgNotifySuccessor);
    MSG_GET_PRED = REGISTER_TYPE (Message, MsgGetPred);
    MSG_PRED = REGISTER_TYPE (Message, MsgPred);
    MSG_GET_SUCCLIST = REGISTER_TYPE (Message, MsgGetSuccessorList);
    MSG_SUCCLIST = REGISTER_TYPE (Message, MsgSuccessorList);
    MSG_NBR_REQ = REGISTER_TYPE (Message, MsgNeighborRequest);
    MSG_NBR_RESP = REGISTER_TYPE (Message, MsgNeighborResponse);
    MSG_LINK_BREAK = REGISTER_TYPE (Message, MsgLinkBreak);

    MSG_PUB = REGISTER_TYPE (Message, MsgPublication);
    MSG_LINEAR_PUB = REGISTER_TYPE (Message, MsgLinearPublication);
    MSG_ACK = REGISTER_TYPE (Message, MsgAck);
    MSG_SUB = REGISTER_TYPE (Message, MsgSubscription);
    MSG_LINEAR_SUB = REGISTER_TYPE (Message, MsgLinearSubscription);
    MSG_SUB_LIST = REGISTER_TYPE (Message, MsgSubscriptionList);
    MSG_TRIG_LIST = REGISTER_TYPE (Message, MsgTriggerList);

    MSG_BOOTSTRAP_REQUEST = REGISTER_TYPE (Message, MsgBootstrapRequest);
    MSG_BOOTSTRAP_RESPONSE = REGISTER_TYPE (Message, MsgBootstrapResponse);

    MSG_SAMPLE_REQ = REGISTER_TYPE (Message, MsgSampleRequest);
    MSG_SAMPLE_RESP = REGISTER_TYPE (Message, MsgSampleResponse);
    MSG_POINT_EST_REQ = REGISTER_TYPE (Message, MsgPointEstimateRequest);
    MSG_POINT_EST_RESP = REGISTER_TYPE (Message, MsgPointEstimateResponse);

    MSG_LOCAL_LB_REQUEST = REGISTER_TYPE (Message, MsgLocalLBRequest);
    MSG_LOCAL_LB_RESPONSE = REGISTER_TYPE (Message, MsgLocalLBResponse);
    MSG_LEAVE_NOTIFICATION = REGISTER_TYPE (Message, MsgLeaveNotification);
    MSG_LEAVEJOIN_LB_REQUEST = REGISTER_TYPE (Message, MsgLeaveJoinLBRequest);
    MSG_LEAVEJOIN_DENIAL = REGISTER_TYPE (Message, MsgLeaveJoinDenial);
    MSG_LCHECK_REQUEST = REGISTER_TYPE (Message, MsgLeaveCheckRequest);
    MSG_LCHECK_RESPONSE = REGISTER_TYPE (Message, MsgLeaveCheckResponse);

    MSG_CB_ALL_JOINED = REGISTER_TYPE (Message, MsgCB_AllJoined);
    MSG_CB_ESTIMATE_REQ = REGISTER_TYPE (Message, MsgCB_EstimateReq);
    MSG_CB_ESTIMATE_RESP = REGISTER_TYPE (Message, MsgCB_EstimateResp);

    MSG_BLOB = REGISTER_TYPE (Message, MsgBlob);
    MSG_COMPRESSED = REGISTER_TYPE (Message, MsgCompressed);
    MSG_PING = REGISTER_TYPE (Message, MsgPing);

    /// XXX: this is for logging purposes. if a message ever arrives with 
    //       this "type", it will cause an assertion failure
    MSG_MATCHED_PUB = REGISTER_TYPE (Message, MsgDummy);
    MSG_RANGE_PUB = REGISTER_TYPE (Message, MsgDummy);
    MSG_RANGE_MATCHED_PUB = REGISTER_TYPE (Message, MsgDummy);

    MSG_RANGE_PUB_NOTROUTING = REGISTER_TYPE (Message, MsgDummy);
    MSG_SUB_NOTROUTING = REGISTER_TYPE (Message, MsgDummy);
    MSG_RANGE_PUB_LINEAR = REGISTER_TYPE (Message, MsgDummy);
    MSG_SUB_LINEAR = REGISTER_TYPE (Message, MsgDummy);

    /// This is a "sentinel" which demarcates mercury messages from
    //  other app-defined ones

    MSG_MERCURY_SENTINEL = REGISTER_TYPE (Message, MsgDummy);
}

static void _dump_type (FILE *fp, char *str, MsgType t)
{
    ASSERT (str != NULL);
    fprintf (fp, "%s %d\n", str, (int) t);
}

#define DUMP_TYPE(T)   _dump_type(fp, #T, T)

/// For post-processing scripts

void DumpMessageTypes (FILE *fp)
{
#if 0
    for (int i = 0; i < BaseclassInfo<Message>::type_pool; i++) {
	fprintf (fp, "%s=%d\n", BaseclassInfo<Message>::typeinfo[i].description, i);
    }
#else
    DUMP_TYPE(MSG_HEARTBEAT);
    DUMP_TYPE(MSG_LIVENESS_PING);
    DUMP_TYPE(MSG_LIVENESS_PONG);

    DUMP_TYPE(MSG_JOIN_REQUEST);
    DUMP_TYPE(MSG_JOIN_RESPONSE);
    DUMP_TYPE(MSG_NOTIFY_SUCCESSOR);
    DUMP_TYPE(MSG_GET_PRED);
    DUMP_TYPE(MSG_PRED);
    DUMP_TYPE(MSG_GET_SUCCLIST);
    DUMP_TYPE(MSG_SUCCLIST);
    DUMP_TYPE(MSG_NBR_REQ);
    DUMP_TYPE(MSG_NBR_RESP);
    DUMP_TYPE(MSG_LINK_BREAK);

    DUMP_TYPE(MSG_PUB);
    DUMP_TYPE(MSG_LINEAR_PUB);
    DUMP_TYPE(MSG_ACK);
    DUMP_TYPE(MSG_SUB);
    DUMP_TYPE(MSG_LINEAR_SUB);
    DUMP_TYPE(MSG_SUB_LIST);
    DUMP_TYPE(MSG_TRIG_LIST);

    DUMP_TYPE(MSG_BOOTSTRAP_REQUEST);
    DUMP_TYPE(MSG_BOOTSTRAP_RESPONSE);

    DUMP_TYPE(MSG_SAMPLE_REQ);
    DUMP_TYPE(MSG_SAMPLE_RESP);
    DUMP_TYPE(MSG_POINT_EST_REQ);
    DUMP_TYPE(MSG_POINT_EST_RESP);

    DUMP_TYPE(MSG_LOCAL_LB_REQUEST);
    DUMP_TYPE(MSG_LOCAL_LB_RESPONSE);
    DUMP_TYPE(MSG_LEAVE_NOTIFICATION);
    DUMP_TYPE(MSG_LEAVEJOIN_LB_REQUEST);
    DUMP_TYPE(MSG_LEAVEJOIN_DENIAL);
    DUMP_TYPE(MSG_LCHECK_REQUEST);
    DUMP_TYPE(MSG_LCHECK_RESPONSE);

    DUMP_TYPE(MSG_CB_ALL_JOINED);
    DUMP_TYPE(MSG_CB_ESTIMATE_REQ);
    DUMP_TYPE(MSG_CB_ESTIMATE_RESP);

    DUMP_TYPE(MSG_BLOB);
    DUMP_TYPE(MSG_COMPRESSED);
    DUMP_TYPE(MSG_PING);

    /// XXX: this is for logging purposes. if a message ever arrives with 
    //       this "type", it will cause an assertion failure
    DUMP_TYPE(MSG_MATCHED_PUB);
    DUMP_TYPE(MSG_RANGE_PUB);
    DUMP_TYPE(MSG_RANGE_MATCHED_PUB);

    DUMP_TYPE(MSG_RANGE_PUB_NOTROUTING);
    DUMP_TYPE(MSG_SUB_NOTROUTING);
    DUMP_TYPE(MSG_RANGE_PUB_LINEAR);
    DUMP_TYPE(MSG_SUB_LINEAR);

    /// This is a "sentinel" which demarcates mercury messages from
    //  other app-defined ones

    DUMP_TYPE(MSG_MERCURY_SENTINEL);
#endif
}


ostream& operator<<(ostream& os, Message *msg)
{
    os << "(Message ";
    msg->Print(os);
    os << ")";
    return os;
}


Message::Message() {
    hubID = 0;
    hopCount = 0;
    nonce = CreateNonce();
    recvTime = TIME_NONE;

    sender = SID_NONE;
}

Message::Message(byte hID, IPEndPoint& sder) : hubID (hID), sender (sder) {
    hopCount = 0;
    nonce = CreateNonce();
    recvTime = TIME_NONE;

    ASSERT(IsMercMsg());
}

Message::Message(Packet *pkt) {
    MsgType type = (MsgType)pkt->ReadByte();
    sender = IPEndPoint(pkt);

    // Must check the type we just read and *not* GetType() because our
    // type is not set yet by the ConstructObject<> template!
    if (IsMercMsg(type)) {
	hubID = pkt->ReadByte();
    }

    hopCount = pkt->ReadShort();
    nonce = pkt->ReadInt();
}

void Message::Serialize(Packet *pkt) {
    byte type = GetType();

    ASSERT(type != MSG_INVALID);
    pkt->WriteByte (type);

    sender.Serialize (pkt);
    if (IsMercMsg())
	pkt->WriteByte (hubID);

    pkt->WriteShort (hopCount);
    pkt->WriteInt (nonce);
}

uint32 Message::GetLength() {
    /* type + sender + hubID */
    uint32 len = 1 + sender.GetLength() + ( IsMercMsg() ? 1 : 0 );

    /* hopCount + nonce */
    len += 2 + 4;

    return len;
}

Message::Message (const Message& other) : sender (other.sender),
					  nonce (other.nonce), hopCount (other.hopCount), recvTime (other.recvTime),
					  hubID (other.hubID)  {}


void Message::Print(FILE *stream) {
    fprintf(stream, "type=%s\n", TypeString());
}

void Message::Print(ostream& os) {
    os << TypeString()	<< " sender=" << sender;
    if (IsMercMsg()) {
	os << " hubID=" << (int)hubID;
    }
    os << " length=" << GetLength();

    os << " hopCount=" << hopCount	<< " nonce=" << nonce;
}

/////////////////////////////////////////////////////////////////////// 
//// MSG_HEARTBEAT 
MsgHeartBeat::MsgHeartBeat(byte hubID, IPEndPoint& sender, NodeRange &r)
    : Message(hubID, sender), range(r)
{
}

MsgHeartBeat::MsgHeartBeat(Packet * pkt):
    Message(pkt), range(pkt)
{
}

MsgHeartBeat::~MsgHeartBeat()
{
}

void MsgHeartBeat::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    range.Serialize(pkt);
}

uint32 MsgHeartBeat::GetLength()
{
    return Message::GetLength() + range.GetLength();
}

void MsgHeartBeat::Print(FILE * stream)
{
    Message::Print(stream);

    fprintf(stream, "range: ");
    range.Print(stream);
}

/////////////////////////////////////////////////////////////////////// 
////  MSG_JOIN_RESPONSE 
MsgJoinResponse::MsgJoinResponse(byte hubID, IPEndPoint& sender, eJoinError error) :
    Message (hubID, sender), eError (error), assigned_range (NULL), succIsOnlyNode (false)
{
    ASSERT(eError != JOIN_NO_ERROR);
#ifdef LOADBAL_TEST
    is_lb_leavejoin = false;
    load = 0.0;
#endif
}

MsgJoinResponse::MsgJoinResponse(byte hubID, IPEndPoint& sender, eJoinError error,
				 NodeRange &assigned)
    : Message(hubID, sender), eError(error), succIsOnlyNode (false)
{
    assigned_range = new NodeRange (assigned);
#ifdef LOADBAL_TEST
    is_lb_leavejoin = false;
    load = 0.0;
#endif
}

void MsgJoinResponse::AddSuccessor(const IPEndPoint &a, const NodeRange &r) 
{
    PeerInfo inf(a, r);
    sil.push_back(inf);
}	

MsgJoinResponse::MsgJoinResponse(Packet * pkt) : Message(pkt)
{
    eError = (eJoinError) pkt->ReadByte();

    if (eError == JOIN_NO_ERROR) {
	assigned_range = new NodeRange(pkt);
	succIsOnlyNode = pkt->ReadBool ();

	int nsucc = pkt->ReadInt();		
	for (int i = 0; i < nsucc; i++) {
	    IPEndPoint a(pkt);
	    NodeRange  r(pkt);

	    PeerInfo   inf(a, r);
	    sil.push_back(inf);
	}

#ifdef LOADBAL_TEST
	is_lb_leavejoin = pkt->ReadBool ();
	load = pkt->ReadFloat ();
#endif
    }
    else 
	assigned_range = NULL;
}

MsgJoinResponse::~MsgJoinResponse() 
{
    if (assigned_range)
	delete assigned_range;
}

void MsgJoinResponse::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);

    pkt->WriteByte((byte) eError);

    if (eError == JOIN_NO_ERROR) {
	assigned_range->Serialize(pkt);
	pkt->WriteBool (succIsOnlyNode);

	int nsucc = sil.size();

	pkt->WriteInt(nsucc);

	for (PeerInfoListIter it = sil.begin(); it != sil.end(); it++) {
	    it->addr.Serialize(pkt);
	    it->range.Serialize(pkt);
	}
#ifdef LOADBAL_TEST
	pkt->WriteBool (is_lb_leavejoin);
	pkt->WriteFloat (load);
#endif
    }
}

uint32 MsgJoinResponse::GetLength()
{
    if (eError == JOIN_NO_ERROR) {
	uint32 len = Message::GetLength();
	len += 1 /* eError cast into a byte */;
	len += assigned_range->GetLength() + 1 /* succIsOnlyNode */ ;

	int nsucc = sil.size();
	len += 4;           // nsucc

	for (PeerInfoListIter it = sil.begin(); it != sil.end(); it++) {
	    len += it->addr.GetLength() + it->range.GetLength();
	}

#ifdef LOADBAL_TEST
	len += 1 + 4;
#endif
	return len;
    } else {
	return Message::GetLength() + sizeof(eError);
    }
}

void MsgJoinResponse::Print(FILE * stream)
{
    Message::Print(stream);
    if (eError == JOIN_AM_UNJOINED)
	fprintf(stream, " (unjoined)");
    else {
	assigned_range->Print(stream);
	fprintf (stream, " succ is only node=%d ", succIsOnlyNode);

	int i = 0;
	for (PeerInfoListIter it = sil.begin(); it != sil.end(); i++, it++) {
	    fprintf(stream, "succ[%d]={", i);
	    it->addr.Print(stream);
	    fprintf(stream, ";");
	    it->range.Print(stream);
	    fprintf(stream, "} ");
	}		

#ifdef LOADBAL_TEST
	fprintf (stream, " is_lb_leavejoin=%d load=%f", is_lb_leavejoin, load);
#endif
    }
}

/////////////////////////////////////////////////////////////////////////
//// MSG_PRED
MsgPred::MsgPred(byte hubID, IPEndPoint& sender, const IPEndPoint &addr, const NodeRange &predrange, const NodeRange& currange)
    : Message(hubID, sender), pred(addr), predrange (predrange), currange (currange)
{
}

MsgPred::MsgPred(Packet *pkt) : Message(pkt), pred(pkt), predrange(pkt), currange (pkt)
{
}

MsgPred::~MsgPred() 
{
}

void MsgPred::Serialize(Packet *pkt)
{
    Message::Serialize(pkt);
    pred.Serialize(pkt);
    predrange.Serialize(pkt);
    currange.Serialize (pkt);
}

uint32 MsgPred::GetLength()
{
    uint32 len = Message::GetLength();
    len += pred.GetLength() + predrange.GetLength() + currange.GetLength ();
    return len;
}

void MsgPred::Print(FILE *stream)
{
    Message::Print(stream);
    fprintf(stream, "pred="); pred.Print(stream);
    fprintf(stream, " pred's range="); predrange.Print(stream);
    fprintf(stream, " mycurrent range="); currange.Print(stream);
}

/////////////////////////////////////////////////////////////////////////
//// MSG_SUCCLIST
MsgSuccessorList::MsgSuccessorList(byte hubID, IPEndPoint& sender) : Message(hubID, sender)
{
}

MsgSuccessorList::MsgSuccessorList(Packet *pkt) : Message(pkt)
{
    int nsucc = pkt->ReadInt();

    for (int i = 0; i < nsucc; i++) {
	IPEndPoint a(pkt);
	NodeRange  r(pkt);

	PeerInfo   inf(a, r);
	sil.push_back(inf);
    }
}

MsgSuccessorList::~MsgSuccessorList() 
{
}

void MsgSuccessorList::AddSuccessor(const IPEndPoint &a, const NodeRange &r) 
{
    PeerInfo inf(a, r);
    sil.push_back(inf);
}	

void MsgSuccessorList::Serialize(Packet *pkt)
{
    Message::Serialize(pkt);
    int nsucc = sil.size();

    pkt->WriteInt(nsucc);
    for (PeerInfoListIter it = sil.begin(); it != sil.end(); it++) {
	it->addr.Serialize(pkt);
	it->range.Serialize(pkt);
    }
}

uint32 MsgSuccessorList::GetLength()
{
    uint32 len = Message::GetLength();

    int nsucc = sil.size();
    len += 4;           // nsucc

    for (PeerInfoListIter it = sil.begin(); it != sil.end(); it++) {
	len += it->addr.GetLength() + it->range.GetLength();
    }
    return len;
}

void MsgSuccessorList::Print(FILE *stream)
{
    Message::Print(stream);

    int i = 0;
    for (PeerInfoListIter it = sil.begin(); it != sil.end(); i++, it++) {
	fprintf(stream, "succ[%d]={", i);
	it->addr.Print(stream);
	fprintf(stream, ";");
	it->range.Print(stream);
	fprintf(stream, "}\n");
    }
}

////////////////////////////////////////////////////////////////////////
// MSG_BOOTSTRAP_RESPONSE
MsgBootstrapResponse::MsgBootstrapResponse(Packet * pkt):Message(pkt)
{
    uint32 len = pkt->ReadInt();
    for (uint32 i = 0; i < len; i++) {
	HubInitInfo *info = new HubInitInfo(pkt);
	hubInfoVec.push_back(info);
    }
}

void MsgBootstrapResponse::AddHubInitInfo(HubInitInfo *info) 
{
    hubInfoVec.push_back(new HubInitInfo(*info));
}

MsgBootstrapResponse::~MsgBootstrapResponse() 
{
    for (int i = 0, len = hubInfoVec.size(); i < len; i++) 
	delete hubInfoVec[i];
    hubInfoVec.clear();
}

void MsgBootstrapResponse::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    pkt->WriteInt(hubInfoVec.size());

    for (int i = 0, len = hubInfoVec.size(); i < len; i++) {
	HubInitInfo *info = hubInfoVec[i];
	info->Serialize(pkt);
    }
}

uint32 MsgBootstrapResponse::GetLength()
{
    uint32 msg_len = Message::GetLength() + 4;
    for (int i = 0, len = hubInfoVec.size(); i < len; i++) {
	HubInitInfo *info = hubInfoVec[i];
	msg_len += info->GetLength();
    }
    return msg_len;
}

void MsgBootstrapResponse::Print(FILE * stream)
{
    Message::Print(stream);

    fprintf (stream, " hubinfo=[");
    for (uint32 i = 0; i < hubInfoVec.size (); i++) {
	if (i != 0) fprintf (stream, ",");
	hubInfoVec[i]->Print (stream);
    }
    fprintf (stream, "]");
}

void MsgBootstrapResponse::Print(ostream& os)
{
    Message::Print(os);
    os << " hubinfo=[";
    for (uint32 i=0; i<hubInfoVec.size(); i++) {
	if (i != 0) os << ",";
	os << hubInfoVec[i];
    }
    os << "]";
}

///////////////////////////////////////////////////////////////////////
// MSG_PUB
MsgPublication::MsgPublication(byte hubID, IPEndPoint& sender, Event *p, IPEndPoint &cr) :
    Message(hubID, sender), metadata(ROUTING_TO_LEFTEND), creator(cr)
{
    pub = p->Clone();     // make a private copy
}

MsgPublication::MsgPublication (const MsgPublication& op) : 
    Message (op), metadata (op.metadata), creator (op.creator)
{
    pub = op.pub->Clone ();
#ifdef RECORD_ROUTE
    routeTaken = op.routeTaken;
#endif
}

MsgPublication& MsgPublication::operator= (const MsgPublication& op) 
{
    if (this == &op) return *this;

    Message::operator= (op);
    metadata = op.metadata;
    creator = op.creator;

    delete pub;
    pub = op.pub->Clone ();
#ifdef RECORD_ROUTE
    routeTaken = op.routeTaken;
#endif
    return *this;
}

MsgPublication::MsgPublication(Packet * pkt)
    : Message(pkt), creator(pkt)
{
    metadata = pkt->ReadByte();	
    pub = CreateObject<Event>(pkt);

#ifdef RECORD_ROUTE
    int routeLen = pkt->ReadInt();
    for (int i = 0; i < routeLen; i++)
    {
	Neighbor h(pkt);
	routeTaken.push_back(h);
    }
#endif
}

MsgPublication::~MsgPublication()
{
    delete pub;
}

void MsgPublication::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    creator.Serialize(pkt);
    pkt->WriteByte(metadata);
    pub->Serialize(pkt);

#ifdef RECORD_ROUTE
    int routeLen = routeTaken.size();
    pkt->WriteInt(routeLen);
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); it++)
    {
	(*it).Serialize(pkt);
    }
#endif
}

uint32 MsgPublication::GetLength()
{
    uint32 len = Message::GetLength() + creator.GetLength() + 1 + pub->GetLength();

#ifdef RECORD_ROUTE
    len += 4;
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); it++)
    {
	len += (*it).GetLength();
    }
#endif

    return len;
}

void MsgPublication::Print(FILE * stream)
{
    Message::Print(stream);
    fprintf(stream, "pub-creator:");
    creator.Print(stream);
    pub->Print(stream);
}

void MsgPublication::Print(ostream &os)
{
    Message::Print(os);
    os << " creator=" << creator
       << " pub=" << pub
       << " mode=" << (metadata & ROUTING_TO_LEFTEND ? "TOLEFT" : "LINEAR");
#ifdef RECORD_ROUTE
    os << " route=[";
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); it++)
    {
	if (it != routeTaken.begin())
	    os << " -> ";
	os << *it;
    }
    os << "]";
#endif	   
}

///////////////////////////////////////////////////////////////////////
// MSG_SUB
MsgSubscription::MsgSubscription(byte hubID, IPEndPoint& sender, Interest *i, IPEndPoint &cr) :
    Message(hubID, sender), creator(cr)
{
    sub = i->Clone();
    metadata = ROUTING_TO_LEFTEND;
}

MsgSubscription::MsgSubscription (const MsgSubscription& op) : 
    Message (op), metadata (op.metadata), creator (op.creator)
{
    sub = op.sub->Clone ();
#ifdef RECORD_ROUTE
    routeTaken = op.routeTaken;
#endif
}

MsgSubscription& MsgSubscription::operator= (const MsgSubscription& op) 
{
    if (this == &op) return *this;

    Message::operator= (op);
    metadata = op.metadata;
    creator = op.creator;
    delete sub;
    sub = op.sub->Clone ();
#ifdef RECORD_ROUTE
    routeTaken = op.routeTaken;
#endif
    return *this;
}
MsgSubscription::MsgSubscription(Packet * pkt): Message(pkt), creator(pkt)
{
    metadata = pkt->ReadByte();	
    sub = CreateObject<Interest>(pkt);

#ifdef RECORD_ROUTE
    int routeLen = pkt->ReadInt();
    for (int i = 0; i < routeLen; i++)
    {
	Neighbor h(pkt);
	routeTaken.push_back(h);
    }
#endif
}

MsgSubscription::~MsgSubscription()
{
    delete sub;
}

void MsgSubscription::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    creator.Serialize(pkt);
    pkt->WriteByte(metadata);
    sub->Serialize(pkt);

#ifdef RECORD_ROUTE
    int routeLen = routeTaken.size();
    pkt->WriteInt(routeLen);
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); it++)
    {
	(*it).Serialize(pkt);
    }
#endif
}

uint32 MsgSubscription::GetLength()
{
    /* header + creator + metadata + sub */
    uint32 len = Message::GetLength() + creator.GetLength() + 1 +
	sub->GetLength();

#ifdef RECORD_ROUTE
    len += 4;
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); it++)
    {
	len += (*it).GetLength();
    }
#endif

    return len;
}

void MsgSubscription::Print(FILE * stream)
{
    Message::Print(stream);
    fprintf(stream, "sub-creator:");
    creator.Print(stream);
    fprintf(stream, " unsubscribed:%s mode:%s\n", (metadata & UNSUB ? "yes" : "no"), 
	    (metadata & ROUTING_TO_LEFTEND ? "TOLEFT" : "LINEAR"));
    sub->Print(stream);
}

void MsgSubscription::Print(ostream& os)
{
    Message::Print(os);
    os << " creator=" << creator
       << " sub=" << sub
       << " unsub=" << (metadata & UNSUB ? "true" : "false")
       << " mode=" << (metadata & ROUTING_TO_LEFTEND ? "TOLEFT" : "LINEAR");
#ifdef RECORD_ROUTE
    os << " route=[";
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); it++)
    {
	if (it != routeTaken.begin())
	    os << " -> ";
	os << *it;
    }
    os << "]";
#endif
}

/////////////////////////////////////////////////////////////////////////
//// MSG_SUB_LIST
MsgSubscriptionList::MsgSubscriptionList(Packet * pkt):Message(pkt) {
    int nSubs = pkt->ReadInt();
    for (int i = 0; i < nSubs; i++) {
	subscriptions.push_back(CreateObject<Interest>(pkt));
    }
}

MsgSubscriptionList::MsgSubscriptionList (const MsgSubscriptionList& other) : Message (other) 
{
    MsgSubscriptionList& ooother = (MsgSubscriptionList &) other;

    for (list<Interest *>::iterator it = ooother.subscriptions.begin (); it != ooother.subscriptions.end (); ++it)
	subscriptions.push_back ((*it)->Clone ());
}

void MsgSubscriptionList::AddSubscription(Interest *in) {
    subscriptions.push_back(in->Clone());
}

MsgSubscriptionList::~MsgSubscriptionList() {
    for (list<Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++)
	delete *it;
    subscriptions.clear();
}

void MsgSubscriptionList::Print(ostream& os) {
    Message::Print(os);
    os << " subs=[";
    for (list<Interest *>::iterator i = subscriptions.begin();
	 i != subscriptions.end(); i++) {
	if (i != subscriptions.begin()) os << ",";
	os << *i;
    }
    os << "]";
}

void MsgSubscriptionList::Serialize(Packet * pkt) {
    Message::Serialize(pkt);
    pkt->WriteInt(subscriptions.size());
    for (list <Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++) {
	(*it)->Serialize(pkt);
    }
}

uint32 MsgSubscriptionList::GetLength() {
    uint32 retval = Message::GetLength() + 4;
    for (list <Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++) {
	retval += (*it)->GetLength();
    }
    return retval;
}

void MsgSubscriptionList::Print(FILE * stream) {
    Message::Print(stream);
    fprintf(stream, " subs=[");
    for (list <Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++) {
	if (it != subscriptions.begin()) fprintf(stream, ",");
	(*it)->Print(stream);
    }
    fprintf(stream, "]");
}

/////////////////////////////////////////////////////////////////////////
//// MSG_TRIG_LIST

MsgTriggerList::MsgTriggerList(Packet * pkt) : Message(pkt) {
    int nSubs = pkt->ReadInt();
    for (int i = 0; i < nSubs; i++) {
	triggers.push_back(new MsgPublication(pkt));
    }
}

MsgTriggerList::MsgTriggerList (const MsgTriggerList& other) : Message (other) 
{
    MsgTriggerList& ooother = (MsgTriggerList &) other;

    for (list<MsgPublication *>::iterator it = ooother.triggers.begin (); it != ooother.triggers.end (); ++it)
	triggers.push_back ((*it)->Clone ());
}

void MsgTriggerList::AddTrigger(MsgPublication *pmsg) {
    triggers.push_back (pmsg->Clone());
}

MsgTriggerList::~MsgTriggerList() {
    for (list<MsgPublication *>::iterator it = triggers.begin(); it != triggers.end(); it++)
	delete *it;
    triggers.clear();
}

void MsgTriggerList::Print(ostream& os) {
    Message::Print(os);
    os << " triggers=[";
    for (list<MsgPublication *>::iterator i = triggers.begin();
	 i != triggers.end(); i++) {
	if (i != triggers.begin()) os << ",";
	os << *i;
    }
    os << "]";
}

void MsgTriggerList::Serialize(Packet * pkt) {
    Message::Serialize(pkt);
    pkt->WriteInt(triggers.size());
    for (list <MsgPublication *>::iterator it = triggers.begin(); it != triggers.end(); it++) {
	(*it)->Serialize(pkt);
    }
}

uint32 MsgTriggerList::GetLength() {
    uint32 retval = Message::GetLength() + 4;
    for (list <MsgPublication *>::iterator it = triggers.begin(); it != triggers.end(); it++) {
	retval += (*it)->GetLength();
    }
    return retval;
}

void MsgTriggerList::Print(FILE * stream) {
    Message::Print(stream);
    fprintf(stream, " triggers=[");
    for (list <MsgPublication*>::iterator it = triggers.begin(); it != triggers.end(); it++) {
	if (it != triggers.begin()) fprintf(stream, ",");
	(*it)->Print(stream);
    }
    fprintf(stream, "]");
}

//////////////////////////////////////////////////////////////////
//// Sampling related messages

MsgPointEstimateResponse::MsgPointEstimateResponse (byte hubID, IPEndPoint& sender, uint32 handle, signed char distance, Sample *estimate) 
    : Message (hubID, sender), metric_handle (handle), distance (distance), estimate (estimate->Clone ()) 
{
}

MsgPointEstimateResponse::MsgPointEstimateResponse (const MsgPointEstimateResponse& other) 
    : Message (other), metric_handle (other.metric_handle), distance (other.distance), estimate (other.estimate->Clone ())
{
}

MsgPointEstimateResponse::~MsgPointEstimateResponse () { 
    delete estimate; 
}

MsgPointEstimateResponse::MsgPointEstimateResponse (Packet *pkt) : Message (pkt) 
{
    metric_handle = pkt->ReadInt ();
    distance = (signed char) pkt->ReadByte ();
    estimate = new Sample (pkt);
}

void MsgPointEstimateResponse::Serialize (Packet *pkt) 
{ 
    Message::Serialize (pkt);

    pkt->WriteInt (metric_handle);
    pkt->WriteByte ((unsigned char) distance);
    estimate->Serialize (pkt);
}

uint32 MsgPointEstimateResponse::GetLength () 
{
    uint32 len = Message::GetLength ();
    return len + 4 + 1 + estimate->GetLength ();
}

void MsgPointEstimateResponse::Print (FILE *stream) 
{
    Message::Print (stream);
    fprintf (stream, " handle=%0x distance=%d estimate=", metric_handle, (int) distance);
    estimate->Print (stream);
}

void MsgPointEstimateResponse::Print (ostream& os) 
{
    Message::Print (os);
    os << " handle=" << merc_va ("%0x", metric_handle) << " distance=" << (int) distance << " estimate=" << estimate;		
}

//////////////////////////////////////////////////////////////////
// MSG_LEAVE_NOTIFICATION

MsgLeaveNotification::MsgLeaveNotification (Packet *pkt) : Message (pkt), assigned_range (RANGE_NONE) 
{
    assigned_range = NodeRange (pkt);
    addload = pkt->ReadFloat ();

    int ntrigs = pkt->ReadInt();
    for (int i = 0; i < ntrigs; i++) 
	triggers.push_back(new MsgPublication(pkt));

    int nsubs = pkt->ReadInt();
    for (int i = 0; i < nsubs; i++) 
	subscriptions.push_back(CreateObject<Interest> (pkt));

}

MsgLeaveNotification::MsgLeaveNotification (const MsgLeaveNotification& omsg) : Message (omsg), assigned_range (RANGE_NONE)
{
    MsgLeaveNotification& ooother = (MsgLeaveNotification &) omsg;

    assigned_range = ooother.assigned_range;
    addload = ooother.addload;

    for (list<MsgPublication *>::iterator it = ooother.triggers.begin (); it != ooother.triggers.end (); ++it)
	triggers.push_back ((*it)->Clone ());
    for (list<Interest *>::iterator it = ooother.subscriptions.begin (); it != ooother.subscriptions.end (); ++it)
	subscriptions.push_back ((*it)->Clone ());
}

MsgLeaveNotification::~MsgLeaveNotification () {
    for (list<MsgPublication *>::iterator it = triggers.begin(); it != triggers.end(); it++)
	delete *it;
    triggers.clear();	

    for (list<Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++)
	delete *it;
    subscriptions.clear();
}

void MsgLeaveNotification::AddTrigger (MsgPublication *p)
{
    triggers.push_back(p->Clone());
}

void MsgLeaveNotification::AddSubscription (Interest *in)
{
    subscriptions.push_back(in->Clone());	
}

void MsgLeaveNotification::Serialize (Packet *pkt) 
{
    Message::Serialize (pkt);

    assigned_range.Serialize (pkt);
    pkt->WriteFloat (addload);

    pkt->WriteInt (triggers.size());
    for (list <MsgPublication *>::iterator it = triggers.begin(); it != triggers.end(); it++) {
	(*it)->Serialize(pkt);
    }

    pkt->WriteInt(subscriptions.size());
    for (list <Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++) {
	(*it)->Serialize(pkt);
    }
}

uint32 MsgLeaveNotification::GetLength () 
{ 
    uint32 retval = Message::GetLength ();
    retval += assigned_range.GetLength () + 4;

    retval += 4;
    for (list <MsgPublication *>::iterator it = triggers.begin(); it != triggers.end(); it++) {
	retval += (*it)->GetLength ();
    }

    retval += 4;
    for (list <Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++) {
	retval += (*it)->GetLength();
    }
    return retval;
}

void MsgLeaveNotification::Print (FILE *stream)
{
    Message::Print(stream);

    fprintf (stream, " assigned=");
    assigned_range.Print (stream);

    fprintf(stream, " addload=%f subs=[", addload);
    for (list <Interest *>::iterator it = subscriptions.begin(); it != subscriptions.end(); it++) {
	if (it != subscriptions.begin()) fprintf(stream, ",");
	(*it)->Print(stream);
    }
    fprintf(stream, "]");

    fprintf(stream, " triggers=[");
    for (list <MsgPublication*>::iterator it = triggers.begin(); it != triggers.end(); it++) {
	if (it != triggers.begin()) fprintf(stream, ",");
	(*it)->Print(stream);
    }
    fprintf(stream, "]");
}

void MsgLeaveNotification::Print (ostream& os)
{
    Message::Print (os);

    os << " assigned=" << assigned_range << " addload=" << addload;
    os << " subs=[";
    for (list<Interest *>::iterator i = subscriptions.begin();
	 i != subscriptions.end(); i++) {
	if (i != subscriptions.begin()) os << ",";
	os << *i;
    }
    os << "]";
    os << " triggers=[";
    for (list<MsgPublication *>::iterator i = triggers.begin();
	 i != triggers.end(); i++) {
	if (i != triggers.begin()) os << ",";
	os << *i;
    }
    os << "]";
}

//////////////////////////////////////////////////////////////////
// MSG_SAMPLE_RESP
MsgSampleResponse::MsgSampleResponse (const MsgSampleResponse& other) 
    : Message (other), metric_handle (other.metric_handle)
{
    for (list<Sample *>::iterator it = ((MsgSampleResponse& ) other).s_begin (); 
	 it != ((MsgSampleResponse& ) other).s_end (); ++it) {
	Sample *s = *it;
	samples.push_back (s->Clone ());
    }
}

MsgSampleResponse::~MsgSampleResponse() 
{
    for (list<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it) 
	delete *it;
    samples.clear ();
}

void MsgSampleResponse::AddSample (Sample *s) 
{ 
    samples.push_back (s->Clone ()); 
}

MsgSampleResponse::MsgSampleResponse (Packet *pkt) : Message (pkt) 
{
    metric_handle = pkt->ReadInt ();

    int nsamples = pkt->ReadInt ();
    for (int i = 0; i < nsamples; i++)
	samples.push_back (new Sample (pkt));
}

void MsgSampleResponse::Serialize(Packet *pkt) 
{
    Message::Serialize (pkt);
    pkt->WriteInt (metric_handle);

    pkt->WriteInt ((int) samples.size ());
    for (list<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it)
	(*it)->Serialize (pkt);
}

uint32 MsgSampleResponse::GetLength() 
{
    uint32 len = Message::GetLength () + 4 /* metric_handle */ + 4 /* samples.size () */;
    for (list<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it)
	len += (*it)->GetLength ();
    return len;
}

void MsgSampleResponse::Print(FILE *stream) 
{
    Message::Print (stream);
    fprintf (stream, " handle=%0x samples=[", metric_handle);
    for (list<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it) {
	(*it)->Print (stream);
	fprintf (stream, " ");
    }
    fprintf (stream, "]");
}

void MsgSampleResponse::Print (ostream& os) 
{
    Message::Print (os);
    os << " handle=" << merc_va ("%0x", metric_handle) << " samples=[";
    for (list<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it) {
	os << *it << " ";
    }
    os << "]";
}

//////////////////////////////////////////////////////////////////
// MSG_CB_ESTIMATE_RESP
MsgCB_EstimateResp::MsgCB_EstimateResp(Packet * pkt) : Message(pkt)
{
    hist = new Histogram(pkt);    // careful; somebody else is going 'delete' this one...
}

void MsgCB_EstimateResp::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    ASSERT(hist != NULL);
    hist->Serialize(pkt);
}

uint32 MsgCB_EstimateResp::GetLength()
{
    ASSERT(hist != NULL);
    return Message::GetLength() + hist->GetLength();
}

void MsgCB_EstimateResp::Print(FILE * stream)
{
    Message::Print(stream);
    fprintf(stream, "\n");
    ASSERT(hist != NULL);
    hist->Print(stream);
}

//////////////////////////////////////////////////////////////////////////
// MsgNeighborRequest

MsgNeighborRequest::MsgNeighborRequest (Packet *pkt) : Message (pkt)
{
    creator = IPEndPoint (pkt);
    val = Value (pkt);
    epoch = pkt->ReadInt();
    nonce = (uint32) pkt->ReadInt ();
}

void MsgNeighborRequest::Serialize (Packet *pkt)
{
    Message::Serialize(pkt);
    creator.Serialize (pkt);
    val.Serialize (pkt);
    pkt->WriteInt (epoch);
    pkt->WriteInt ((int) nonce);
}

uint32 MsgNeighborRequest::GetLength() {
    uint32 len = Message::GetLength ();

    len += creator.GetLength () + val.GetLength () + 4 + 4;
    return len;
}

void MsgNeighborRequest::Print (ostream& os) {
    Message::Print (os);
    os << " creator=" << creator << " val=" << val << " epoch=" << epoch << " nonce=" << merc_va ("%x", nonce);
}

void MsgNeighborRequest::Print(FILE *stream) {
    Message::Print (stream);
    fprintf (stream, " creator=");
    creator.Print (stream);
    fprintf (stream, " val="); 
    val.Print (stream);
    fprintf(stream, " epoch=%d nonce=%x", epoch, nonce);
}


/////////////////////////////////////////////////////////////////
// MSG_NBR_RESP
MsgNeighborResponse::MsgNeighborResponse (byte hubID, IPEndPoint& sender, NodeRange &r, int ep, uint32 n) :
    Message(hubID, sender), range(r), epoch(ep), nonce (n)
{
}

MsgNeighborResponse::MsgNeighborResponse(Packet * pkt) : 
    Message (pkt), range (RANGE_NONE) 
{
    epoch = pkt->ReadInt ();
    nonce = (uint32) pkt->ReadInt ();
    range = NodeRange (pkt);
}

MsgNeighborResponse::~MsgNeighborResponse() {
}

void MsgNeighborResponse::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    pkt->WriteInt (epoch);
    pkt->WriteInt ((int) nonce);
    range.Serialize (pkt);
}

uint32 MsgNeighborResponse::GetLength()
{
    uint32 length = Message::GetLength() + 4 /* epoch */  + 4 /* nonce */ + range.GetLength();
    return length;
}

void MsgNeighborResponse::Print(FILE * stream)
{
    Message::Print(stream);
    fprintf(stream, "epoch=%d nonce=%0x range=", epoch, nonce);
    range.Print(stream);
}


////////////////////////////  Utility messages

//////////////////////////////////////////////////////////////////////
// MSG_COMPRESSED

MsgCompressed::MsgCompressed(Message *msg) : Message(), orig(msg) 
{
    ASSERT(orig);
    sender    = orig->sender;
    nonce     = orig->nonce;
    hopCount  = orig->hopCount;
    hubID     = orig->hubID;

    MakeCompressed();
}

void MsgCompressed::MakeCompressed()
{
    START(MSG_COMPRESS_OVERHEAD);

    ASSERT(orig);

    Packet upkt(orig->GetLength());
    orig->Serialize(&upkt);

    origLen = upkt.GetMaxSize ();	
    compLen = (uint32)(origLen*1.015 + 12);
    compBuf = new byte[compLen];
    int ret = compress(compBuf, (uLongf *)&compLen, upkt.GetBuffer (), origLen);
    ASSERT(ret == Z_OK);

    DB(5) << "orig=" << origLen << " comp=" << compLen << endl;

    STOP(MSG_COMPRESS_OVERHEAD);
}

MsgCompressed::~MsgCompressed()
{
    if (compBuf)
	delete[] compBuf;
}

MsgCompressed::MsgCompressed(Packet * pkt) : Message()
{
    START(MSG_UNCOMPRESS_OVERHEAD);

    (void) pkt->ReadByte();     // strip off the leading byte

    compLen = pkt->ReadInt();
    origLen = pkt->ReadInt();

    compBuf = new byte[compLen];
    pkt->ReadBuffer(compBuf, compLen);

    Packet upkt(origLen);
    int ret = uncompress(upkt.GetBuffer (), (uLongf *)&origLen, compBuf, compLen);
    ASSERT(ret == Z_OK);

    orig = CreateObject<Message>(&upkt);

    sender    = orig->sender;
    nonce     = orig->nonce;
    hopCount  = orig->hopCount;
    hubID     = orig->hubID;

    STOP(MSG_UNCOMPRESS_OVERHEAD);
}

void MsgCompressed::Serialize(Packet * pkt)
{
    pkt->WriteByte(GetType());        // don't send it to Message::Serialize() since 
    // it will write the sender, hopcount, information again!

    ASSERT(compBuf);
    pkt->WriteInt(compLen);
    pkt->WriteInt(origLen);
    pkt->WriteBuffer(compBuf, compLen);
}

uint32 MsgCompressed::GetLength()
{
    return 1 + 4 + 4 + compLen;
}

void MsgCompressed::Print(FILE * stream)
{
    Message::Print(stream);
    fprintf(stream, "\n");
}

#if 0
//////////////////////////////////////////////////////////////////////////
// MSG_RANGE_PUB / MSG_MATCHED_RANGE_PUB
MsgRangePublication::MsgRangePublication(Packet * pkt):Message(pkt)
{
    creator = IPEndPoint(pkt);
    pub = RangeEvent(pkt);
    tag = pkt->ReadByte();
}

void MsgRangePublication::Serialize(Packet * pkt)
{
    Message::Serialize(pkt);
    creator.Serialize(pkt);
    pub.Serialize(pkt);
    pkt->WriteByte(tag);
}

uint32 MsgRangePublication::GetLength()
{
    return Message::GetLength() + creator.GetLength() + pub.GetLength() + 1;
}

void MsgRangePublication::Print(FILE * stream)
{
    Message::Print(stream);
    fprintf(stream, "pub-creator:");
    creator.Print(stream);
    fprintf(stream, "tag: %d\n",  tag);
    pub.Print(stream);
}
#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
