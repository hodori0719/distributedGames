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

#ifndef __MESSAGE__H
#define __MESSAGE__H

#include <string>
#include <map>
#include <mercury/NetworkLayer.h>
#include <mercury/common.h>
#include <mercury/Neighbor.h>
#include <mercury/Event.h>
#include <mercury/Interest.h>
#include <mercury/Peer.h>
#include <mercury/AutoSerializer.h>
#include <util/Utils.h>

#define LOADBAL_TEST

class Packet;

typedef byte MsgType;
extern MsgType
MSG_INVALID, 
    MSG_HEARTBEAT, MSG_LIVENESS_PING, MSG_LIVENESS_PONG, 

    /// Joining the ring
    MSG_JOIN_REQUEST,  MSG_JOIN_RESPONSE,  MSG_NOTIFY_SUCCESSOR,

    /// Maintenance of the successor list and the predecessor
    //  also, partly responsible for the join process
    MSG_GET_PRED, MSG_PRED, MSG_GET_SUCCLIST, MSG_SUCCLIST,

    // Long neighbors
    MSG_NBR_REQ, MSG_NBR_RESP, MSG_LINK_BREAK,

    // Publication and subscriptions
    MSG_PUB, MSG_LINEAR_PUB, MSG_ACK, MSG_SUB, MSG_LINEAR_SUB, MSG_SUB_LIST, MSG_TRIG_LIST,

    // Communication with the bootstrap server
    MSG_BOOTSTRAP_REQUEST, MSG_BOOTSTRAP_RESPONSE,

    // Sampling
    MSG_SAMPLE_REQ, MSG_SAMPLE_RESP, 
    MSG_POINT_EST_REQ, MSG_POINT_EST_RESP,

    // Load-balancing
    MSG_LOCAL_LB_REQUEST, MSG_LOCAL_LB_RESPONSE,
    MSG_LEAVE_NOTIFICATION,
    MSG_LEAVEJOIN_LB_REQUEST, /* NO RESPONSE */
    MSG_LEAVEJOIN_DENIAL,
    MSG_LCHECK_REQUEST, MSG_LCHECK_RESPONSE,

    // signal indicated all expected nodes have joined
    // CB = from (C)entralized (B)ootstrap node	
    MSG_CB_ALL_JOINED, MSG_CB_ESTIMATE_REQ, MSG_CB_ESTIMATE_RESP,

    // utility
    MSG_BLOB, MSG_COMPRESSED, MSG_PING,

    /// XXX: Start: These are just for logging purposes and 
    //       should NOT be used for other things -- Ashwin [10/08/2004]
    MSG_MATCHED_PUB, MSG_RANGE_PUB, MSG_RANGE_MATCHED_PUB,
    MSG_RANGE_PUB_NOTROUTING, MSG_SUB_NOTROUTING,
    MSG_RANGE_PUB_LINEAR, MSG_SUB_LINEAR,
    //  XXX: End 	

    MSG_MERCURY_SENTINEL
    ;

void RegisterMessageTypes();

typedef Message* (*DeSerializer)(Packet *pkt);

struct Message : public Serializable {

    DECLARE_BASE_TYPE(Message);

    IPEndPoint   sender;    // not "really" needed; but would help for NATs and firewalls

    uint32       nonce;     // only for measurement
    uint16       hopCount;  // only for measurement
    TimeVal      recvTime;  // only for measurement; NOT serialized

    ///// MERCURY-ONLY FIELDS
    byte         hubID;

    Message ();
    Message (const Message& other);
    Message (byte hID, IPEndPoint& sder);  // mercury message

    virtual ~Message() {}

    ///////////////////////////////////////////////////////////////////////////

    inline bool IsMercMsg() { return (GetType() < MSG_MERCURY_SENTINEL); }
    inline bool IsMercMsg(MsgType t) { return (t < MSG_MERCURY_SENTINEL); }
    virtual const char *TypeString() = 0;

    ///////////////////////////////////////////////////////////////////////////

    Message(Packet *pkt);
    virtual void Serialize(Packet *pkt);
    virtual uint32 GetLength();
    virtual void Print(FILE *stream);
    virtual void Print(ostream& os);
};

ostream& operator<<(ostream& os, Message *msg);

struct MsgLinkBreak : public Message {
    protected:
DECLARE_TYPE(Message, MsgLinkBreak);

    public:
MsgLinkBreak (byte hubID, IPEndPoint& sender) : Message (hubID, sender) {}
    MsgLinkBreak (Packet *pkt) : Message (pkt) {}
    virtual ~MsgLinkBreak () {} 

    const char *TypeString () { return "MSG_LINK_BREAK"; }
};

struct MsgHeartBeat : public Message {
    private:    
NodeRange range;
    protected:
DECLARE_TYPE(Message, MsgHeartBeat);

    public:    
MsgHeartBeat(byte hubID, IPEndPoint& sender, NodeRange &r);
    MsgHeartBeat(Packet *pkt);
    virtual ~MsgHeartBeat();

    NodeRange &GetRange() { return range; }

    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_HEARTBEAT"; }
    void Print (ostream& os) {
	Message::Print (os);
	os << " range=" << range;
    }
};

struct MsgLivenessPing : public MsgHeartBeat {
    byte seqno;
    byte peertype;
    protected:
    DECLARE_TYPE (Message, MsgLivenessPing);
    public:
    MsgLivenessPing (byte hubID, IPEndPoint& sender, NodeRange& r, byte seqno) 
	: MsgHeartBeat (hubID, sender, r), seqno (seqno), peertype (PEER_NONE) {}
    virtual ~MsgLivenessPing () {}

    byte GetSeqno () { return seqno; }

    void AddPeerType (PeerType p) { peertype |= p; }
    const bool IsSuccessorPing () const { return peertype & PEER_SUCCESSOR; }
    const bool IsLongNeighborPing () const { return peertype & PEER_LONG_NBR; }

    MsgLivenessPing (Packet *pkt) : MsgHeartBeat (pkt) {
	seqno = pkt->ReadByte ();
	peertype = pkt->ReadByte ();
    }
    void Serialize(Packet *pkt) { 
	MsgHeartBeat::Serialize (pkt);
	pkt->WriteByte (seqno);
	pkt->WriteByte (peertype);
    }

    uint32 GetLength() {
	return MsgHeartBeat::GetLength () + 1 + 1;
    }

    char *get_peer_type () { 
	static char s[80];
	s[0] = '\0';
	if (peertype & PEER_SUCCESSOR) sprintf (s, "succ;");
	if (peertype & PEER_LONG_NBR) sprintf (s, "lnbr;");
	return s;
    }

    void Print(FILE *stream) {
	MsgHeartBeat::Print (stream);
	fprintf (stream, " seqno=%d peertype=%s", (int) seqno, get_peer_type ());
    }

    const char *TypeString () { return "MSG_LIVENESS_PING"; }
    void Print (ostream& os) {
	MsgHeartBeat::Print (os);
	os << " seqno=" << (int) seqno << " peertype=" << get_peer_type ();
    }
};

struct MsgLivenessPong : public MsgLivenessPing {
    protected:
DECLARE_TYPE (Message, MsgLivenessPong);
    public:
MsgLivenessPong (byte hubID, IPEndPoint& sender, NodeRange& r, byte seqno) : MsgLivenessPing (hubID, sender, r, seqno) {} 
    MsgLivenessPong (Packet *pkt) : MsgLivenessPing (pkt) {}
    virtual ~MsgLivenessPong () {}

    const char *TypeString () { return "MSG_LIVENESS_PONG"; }
};

//////////////////////////////////////////////////////////////////////
// Join-related messages

struct MsgJoinRequest : public Message {
    private:
bool usedesired;
    MercuryID desiredmax;
#ifdef LOADBAL_TEST
    bool is_lb_leavejoin;
#endif
    protected:
    DECLARE_TYPE(Message, MsgJoinRequest);

    public:
    MsgJoinRequest(byte hubID, IPEndPoint& sender) : Message (hubID, sender), usedesired(false) {}

#ifdef LOADBAL_TEST
    MsgJoinRequest (byte hubID, IPEndPoint& sender, bool is_lb_lj) 
	: Message(hubID, sender), is_lb_leavejoin (is_lb_lj), usedesired(false) {}

    bool IsLeaveJoinRelated () { return is_lb_leavejoin; }
#endif

    MsgJoinRequest(Packet *pkt) : Message(pkt) {
	usedesired = pkt->ReadBool ();
	if (usedesired)
	    desiredmax = MercuryID(pkt);
#ifdef LOADBAL_TEST
	is_lb_leavejoin = pkt->ReadBool ();
#endif
    }

    virtual ~MsgJoinRequest() {}

    void Serialize(Packet *pkt) {
	Message::Serialize (pkt);
	pkt->WriteBool(usedesired);
	if (usedesired)
	    desiredmax.Serialize(pkt);
#ifdef LOADBAL_TEST
	pkt->WriteBool (is_lb_leavejoin);
#endif
    }

    uint32 GetLength() {
	uint32 len = Message::GetLength ();
	len += (1 + (usedesired ? desiredmax.GetLength() : 0));
#ifdef LOADBAL_TEST
	len += 1;
#endif
	return len;
    }
    void Print(FILE *stream) {
	Message::Print (stream);
#ifdef LOADBAL_TEST
	fprintf (stream, "leave-join=%s ", (is_lb_leavejoin ? "true" : "false"));
#endif
    }
    void Print(ostream& os) { 
	Message::Print(os);
#ifdef LOADBAL_TEST
	os << "leave-join=" << (is_lb_leavejoin ? "true" : "false");
#endif
    }
    const char *TypeString() { return "MSG_JOIN_REQUEST"; }

    void SetDesiredMax(const MercuryID& id) {
	usedesired = true;
	desiredmax = id;
    }

    bool UseDesired() {
	return usedesired;
    }

    const MercuryID& GetDesiredMax() {
	return desiredmax;
    }
};

typedef struct timeval TimeVal;
typedef enum { JOIN_NO_ERROR, JOIN_AM_UNJOINED } eJoinError;

//// Response to a join request. This message contains the range assigned 
//// to the newly joining node
struct MsgJoinResponse : public Message {
    private:
eJoinError	  eError;
    NodeRange    *assigned_range;
    PeerInfoList  sil;
    bool          succIsOnlyNode;

#ifdef LOADBAL_TEST
    bool          is_lb_leavejoin;
    float         load;
#endif
    protected:
    DECLARE_TYPE(Message, MsgJoinResponse);

    public:
    MsgJoinResponse(byte hubID, IPEndPoint& sender, eJoinError error);
    MsgJoinResponse(byte hubID, IPEndPoint& sender, eJoinError error, NodeRange &assigned);
    MsgJoinResponse(Packet *pkt);
    virtual ~MsgJoinResponse();

#ifdef LOADBAL_TEST
    void SetLoad (float l) { 
	load = l;
	is_lb_leavejoin = true;
    }
    float GetLoad () { return load; }
    bool IsLeaveJoinRelated () { return is_lb_leavejoin; }
#endif

    void AddSuccessor(const IPEndPoint &a, const NodeRange &r);
    PeerInfoList *GetPeerInfoList() { return &sil; }	
    NodeRange& GetAssignedRange() { return *assigned_range; }

    eJoinError  GetError() { return eError; }

    bool SuccessorIsOnlyNode () { return succIsOnlyNode; }
    void SetSuccessorIsOnlyNode () { succIsOnlyNode = true; }

    void    Serialize(Packet *pkt);
    uint32  GetLength();
    void    Print(FILE *stream);

    const char *TypeString() { return "MSG_JOIN_RESPONSE"; }
};

//// Request for a predecessor; this message is crucial for maintaining 
//// ring connectivity in the face of joins and leaves.
struct MsgGetPred : public Message {
    private:
protected:
DECLARE_TYPE(Message, MsgGetPred);
    public:
MsgGetPred(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}
    MsgGetPred(Packet *pkt) : Message(pkt) {}
    virtual ~MsgGetPred() {}

    void Serialize(Packet *pkt) { Message::Serialize(pkt); }
    uint32 GetLength() { return Message::GetLength(); }
    void Print(FILE *stream) { Message::Print(stream); }

    const char *TypeString() { return "MSG_GET_PRED"; }
    void Print(ostream& os) { Message::Print(os); }
};

//// Response for the MSG_GET_PRED message
//// tell this guy what we think is the predecessor and its range
//// must also tell him what my current range is
struct MsgPred : public Message {
    private:
IPEndPoint  pred;
    NodeRange   predrange;
    NodeRange   currange;
    protected:
    DECLARE_TYPE(Message, MsgPred);

    public:
    MsgPred(byte hubID, IPEndPoint& sender, const IPEndPoint &addr, const NodeRange &predrange, const NodeRange& currange);
    MsgPred(Packet *pkt);
    virtual ~MsgPred();

    IPEndPoint &GetPredAddress() { return pred; }
    NodeRange  &GetPredRange() { return predrange; }
    NodeRange  &GetCurRange () { return currange; }

    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_PRED"; }
    void Print(ostream& os) { 
	Message::Print(os);
	os << "pred=" << pred << " predrange=" << predrange << " currange=" << currange;
    }
};

//// Request for a successor list; this message is used for maintaining 
//// ring connectivity in the face of (multiple) node failures.
struct MsgGetSuccessorList : public Message {
    private:
protected:
DECLARE_TYPE(Message, MsgGetSuccessorList);
    public:
MsgGetSuccessorList(byte hubID, IPEndPoint& sender) : Message (hubID, sender) {}
    MsgGetSuccessorList(Packet *pkt) : Message(pkt) {}
    virtual ~MsgGetSuccessorList() {}

    void Serialize(Packet *pkt) { Message::Serialize(pkt); }
    uint32 GetLength() { return Message::GetLength(); }
    void Print(FILE *stream) { Message::Print(stream); }

    const char *TypeString() { return "MSG_GET_SUCCLIST"; }
    void Print(ostream& os) { Message::Print(os); }
};

//// Response for the MSG_SUCCLIST message. provide a list of 
//// successors and the ranges they are responsible for?

struct MsgSuccessorList : public Message {
    private:
PeerInfoList sil;
    protected:
DECLARE_TYPE(Message, MsgSuccessorList);
    public:
MsgSuccessorList(byte hubID, IPEndPoint& sender);
    MsgSuccessorList(Packet *pkt);
    virtual ~MsgSuccessorList();

    void AddSuccessor(const IPEndPoint &a, const NodeRange &r);
    PeerInfoList *GetPeerInfoList() { return &sil; }

    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_SUCCLIST"; }
    void Print(ostream& os) { 
	Message::Print(os);
	int i = 0;
	for (PeerInfoListIter it = sil.begin(); it != sil.end(); i++, it++) {
	    os << merc_va("succ[%d]={%s; ", i, it->addr.ToString()) << it->range << 
		"}" << endl;
	}
    }
};

//// Notify my successor that I think I am its predecessor
//// The successor will set its predecessor pointer to me 
struct MsgNotifySuccessor : public MsgHeartBeat {
    DECLARE_TYPE(Message, MsgNotifySuccessor);

    MsgNotifySuccessor(byte hubID, IPEndPoint& sender, NodeRange &range) 
	: MsgHeartBeat(hubID, sender, range)  {}

    virtual ~MsgNotifySuccessor() {}

    MsgNotifySuccessor(Packet *pkt) : MsgHeartBeat(pkt) {}
    const char *TypeString() { return "MSG_NOTIFY_SUCCESSOR"; }
};

struct MsgBootstrapRequest : public Message {
    DECLARE_TYPE(Message, MsgBootstrapRequest);

    MsgBootstrapRequest(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}
    MsgBootstrapRequest(Packet *pkt) : Message(pkt) {}
    virtual ~MsgBootstrapRequest() {}

    const char *TypeString() { return "MSG_BOOTSTRAP_REQUEST"; }
};

struct HubInitInfo;

struct MsgBootstrapResponse : public Message {
    protected:
DECLARE_TYPE(Message, MsgBootstrapResponse);

    vector<HubInitInfo *> hubInfoVec;     
    public:
    MsgBootstrapResponse(byte hubID, IPEndPoint& sender) : 
	Message(hubID, sender) {}
    virtual ~MsgBootstrapResponse();

    void AddHubInitInfo(HubInitInfo *h); 

    vector<HubInitInfo *>::iterator begin() { return hubInfoVec.begin(); }
    vector<HubInitInfo *>::iterator end() { return hubInfoVec.end(); }

    MsgBootstrapResponse(Packet *pkt);
    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_BOOTSTRAP_RESPONSE"; }
    void Print(ostream& os);
};

struct MsgAck : public MsgHeartBeat {
    DECLARE_TYPE(Message, MsgAck);

    MsgAck(byte hubID, IPEndPoint& sender, NodeRange &range) : MsgHeartBeat(hubID, sender, range) {}
    virtual ~MsgAck() {}
    MsgAck(Packet *pkt) : MsgHeartBeat(pkt) {}
    const char *TypeString() { return "MSG_ACK"; }
};

#ifdef DEBUG
#define RECORD_ROUTE
#endif

struct MsgPublication : public Message {
    private:
// for routing range publications
enum { ROUTING_TO_LEFTEND = 0x1, ROUTING_LINEARLY = 0x2}; 
    byte        metadata;
    Event      *pub;               // because events can be extended, and we must call Clone() on them.
    IPEndPoint  creator; 
    protected:
    DECLARE_TYPE(Message, MsgPublication);
    public:        
#ifdef RECORD_ROUTE 
    list<Neighbor> routeTaken;
#endif

    MsgPublication(byte hubID, IPEndPoint& sender, Event *p, IPEndPoint &cr);
    MsgPublication(const MsgPublication&);
    MsgPublication& operator=(const MsgPublication&);

    MsgPublication(Packet *pkt);
    virtual ~MsgPublication();

    void Serialize(Packet *pkt);
    uint32  GetLength();
    void Print(FILE *stream);

    Event *GetEvent() { return pub; }
    IPEndPoint &GetCreator() { return creator; }

    void ChangeModeToLinear() { metadata = ROUTING_LINEARLY; }
    bool IsRoutingModeLinear() { return metadata == ROUTING_LINEARLY; }

    virtual const char *TypeString() { return "MSG_PUB";}

    void Print(ostream& os);
};

struct MsgSubscription : public Message {
    private:    
enum { UNSUB = 0x1, ROUTING_TO_LEFTEND = 0x2, ROUTING_LINEARLY = 0x4};
    byte        metadata;
    Interest   *sub;            // pointer, because Interests can be extended....
    IPEndPoint  creator;
    protected:
    DECLARE_TYPE(Message, MsgSubscription);

    public:    

#ifdef RECORD_ROUTE
    list<Neighbor> routeTaken;
#endif

    MsgSubscription(byte hubID, IPEndPoint& sender, Interest *s, IPEndPoint &cr);
    MsgSubscription(const MsgSubscription& m);
    MsgSubscription& operator=(const MsgSubscription&);

    MsgSubscription(Packet *pkt);

    virtual ~MsgSubscription();

    void Serialize(Packet *pkt);
    uint32  GetLength();
    void Print(FILE *stream);

    IPEndPoint &GetCreator() { return creator; }
    Interest   *GetInterest() { return sub; }

    void SetUnsubscribe() { metadata |= UNSUB; }
    void ChangeModeToLinear() { 
	metadata &= ~ROUTING_TO_LEFTEND;
	metadata |= ROUTING_LINEARLY; 
    }

    bool IsRoutingModeLinear() { return metadata & ROUTING_LINEARLY; }

    const char *TypeString() { return "MSG_SUB"; }
    void Print(ostream& os);
};

struct MsgLinearPublication : public MsgPublication {
    private:
Value stopval;
    protected:
DECLARE_TYPE (Message, MsgLinearPublication);
    public:
MsgLinearPublication (const MsgPublication& pmsg, Value& stopval) :
    MsgPublication (pmsg), stopval (stopval) {}
    ~MsgLinearPublication () {}

    Value& GetStopVal () { return stopval; }
    void SetStopVal (const Value& v) { stopval = v; }

    MsgLinearPublication (Packet *pkt) : MsgPublication (pkt), stopval (pkt) {}
    void Serialize (Packet *pkt) {
	MsgPublication::Serialize (pkt);
	stopval.Serialize (pkt);
    }
    uint32 GetLength () { 
	return MsgPublication::GetLength () + stopval.GetLength ();
    }
    void Print (FILE *stream) {
	MsgPublication::Print (stream);
	fprintf (stream, " stopval=");
	stopval.Print (stream);
    }
    const char *TypeString () { return "MSG_LINEAR_PUB"; }
    void Print (ostream& os) { 
	MsgPublication::Print (os);
	os << " stopval=" << stopval;
    }
};

struct MsgLinearSubscription : public MsgSubscription {
    private:
Value stopval;
    protected:
DECLARE_TYPE (Message, MsgLinearSubscription);
    public:
MsgLinearSubscription (const MsgSubscription& pmsg, Value& stopval) :
    MsgSubscription (pmsg), stopval (stopval) {}
    ~MsgLinearSubscription () {}

    Value& GetStopVal () { return stopval; }
    void SetStopVal (const Value& v) { stopval = v; }

    MsgLinearSubscription (Packet *pkt) : MsgSubscription (pkt), stopval (pkt) {}
    void Serialize (Packet *pkt) {
	MsgSubscription::Serialize (pkt);
	stopval.Serialize (pkt);
    }
    uint32 GetLength () { 
	return MsgSubscription::GetLength () + stopval.GetLength ();
    }
    void Print (FILE *stream) {
	MsgSubscription::Print (stream);
	fprintf (stream, " stopval=");
	stopval.Print (stream);
    }
    const char *TypeString () { return "MSG_LINEAR_SUB"; }
    void Print (ostream& os) { 
	MsgSubscription::Print (os);
	os << " stopval=" << stopval;
    }
};

struct MsgSubscriptionList : public Message {
    private:
list<Interest *> subscriptions;
    protected:
DECLARE_TYPE(Message, MsgSubscriptionList);
    public:	
MsgSubscriptionList(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}
    MsgSubscriptionList(const MsgSubscriptionList& m);
    MsgSubscriptionList(Packet *pkt);

    virtual ~MsgSubscriptionList();

    void AddSubscription(Interest *s);
    list<Interest *>::iterator begin() { return subscriptions.begin(); }
    list<Interest *>::iterator end() { return subscriptions.end(); }
    size_t size() { return subscriptions.size(); }

    void  Serialize(Packet *pkt);
    uint32 GetLength();
    void  Print(FILE *stream);

    const char *TypeString() { return "MSG_SUB_LIST"; }
    void Print(ostream& os);
};

struct MsgTriggerList : public Message {
    private:
list<MsgPublication *>    triggers;
    protected:
DECLARE_TYPE(Message, MsgTriggerList);

    public:
MsgTriggerList(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}
    MsgTriggerList(const MsgTriggerList& m);
    MsgTriggerList& operator=(const MsgTriggerList& m);
    MsgTriggerList(Packet *pkt);

    virtual ~MsgTriggerList();

    void AddTrigger(MsgPublication *e);
    list<MsgPublication *>::iterator begin() { return triggers.begin(); }
    list<MsgPublication *>::iterator end() { return triggers.end(); }
    size_t size() { return triggers.size(); }

    void  Serialize(Packet *pkt);
    uint32 GetLength();
    void  Print(FILE *stream);

    const char *TypeString() { return "MSG_TRIG_LIST"; }
    void Print(ostream& os);
};

struct MsgCB_AllJoined : public Message {
    DECLARE_TYPE(Message, MsgCB_AllJoined);

    MsgCB_AllJoined(IPEndPoint& sender) : Message((byte) 0, sender) {}
    virtual ~MsgCB_AllJoined() {}
    MsgCB_AllJoined(Packet *pkt) : Message(pkt) {}

    const char *TypeString() { return "MSG_CB_ALL_JOINED"; }
};

////////////////////////////////////////////////////////////////////////////
////  Sampling-related messages

enum { DIR_PRED, DIR_SUCC };

struct MsgPointEstimateRequest : public Message {
    private:
uint32 metric_handle;
    byte direction;
    byte ttl;
    IPEndPoint creator;
    protected:
    DECLARE_TYPE(Message, MsgPointEstimateRequest);
    public:
    MsgPointEstimateRequest (byte hubID, IPEndPoint& sender, IPEndPoint& creator, uint32 handle, byte direction, byte ttl) :
	Message (hubID, sender), creator (creator), metric_handle (handle), direction (direction), ttl (ttl) {}
    ~MsgPointEstimateRequest () {}

    IPEndPoint& GetCreator () { return creator; }
    uint32 GetMetricHandle () { return metric_handle; }
    byte GetDirection () { return direction; }

    byte GetTTL () { return ttl; }
    void DecrementTTL () { ttl -= 1; }

    MsgPointEstimateRequest (Packet *pkt) : Message (pkt) {
	metric_handle = pkt->ReadInt ();
	direction = pkt->ReadByte ();
	ttl = pkt->ReadByte ();
	creator = IPEndPoint (pkt);
    }

    void Serialize(Packet *pkt) {
	Message::Serialize (pkt);
	pkt->WriteInt (metric_handle);
	pkt->WriteByte (direction);
	pkt->WriteByte (ttl);
	creator.Serialize (pkt);
    }
    uint32 GetLength() {
	uint32 len = Message::GetLength ();
	return len + 4 + 1 + 1 + creator.GetLength ();
    }

    void Print(FILE *stream) {
	Message::Print (stream);
	fprintf (stream, " handle=%0x dir=%s ttl=%d", metric_handle, (direction == DIR_SUCC ? "DIR_SUCC" : "DIR_PRED"), ttl);
	fprintf (stream, " creator=");
	creator.Print (stream);
    }

    const char *TypeString () { return "MSG_POINT_EST_REQ"; }
    void Print (ostream& os) {
	Message::Print (os);
	os << " handle=" << merc_va ("%0x", metric_handle) << " dir=" << (direction == DIR_SUCC ? "DIR_SUCC" : "DIR_PRED")
	   << " ttl=" << (int) ttl << " creator=" << creator;
    }

};

class Sample;

struct MsgPointEstimateResponse : public Message {
    private:
uint32 metric_handle;
    signed char distance;
    Sample *estimate;
    protected:
    DECLARE_TYPE(Message, MsgPointEstimateResponse);
    public:
    MsgPointEstimateResponse (byte hubID, IPEndPoint& sender, uint32 handle, signed char distance, Sample *estimate);
    MsgPointEstimateResponse (const MsgPointEstimateResponse& other);
    ~MsgPointEstimateResponse ();

    uint32 GetMetricHandle () const { return metric_handle; }
    signed int GetDistance () const { return distance; }
    Sample *GetPointEstimate () { return estimate; }

    MsgPointEstimateResponse (Packet *pkt);

    void Serialize (Packet *pkt);
    uint32 GetLength ();
    void Print (FILE *stream);
    void Print (ostream& os);

    const char *TypeString () { return "MSG_POINT_EST_RESP"; }
};

struct MsgSampleRequest : public Message {
    private:
uint32 metric_handle;
    byte ttl;
    IPEndPoint creator;
    byte seqno;
    // the seqno stuff is needed coz this message
    // also serves as a ping to this neighbor and
    // suppresses the normal liveness check
    protected:
    DECLARE_TYPE(Message, MsgSampleRequest);

    public:
    MsgSampleRequest (byte hubID, IPEndPoint& sender, IPEndPoint& creator, uint32 handle, byte ttl, byte seqno) : 
	Message (hubID, sender), creator (creator), metric_handle (handle), ttl (ttl), seqno (seqno) {}
    virtual ~MsgSampleRequest() {}

    byte GetTTL () const { return ttl; }
    void DecrementTTL () { ttl -= 1; }
    uint32 GetMetricHandle () const { return metric_handle; }
    IPEndPoint& GetCreator () { return creator; }

    byte GetSeqno () const { return seqno; }
    void SetSeqno (byte no) { seqno = no; }

    MsgSampleRequest (Packet *pkt) : Message (pkt) {
	metric_handle = pkt->ReadInt ();
	ttl = pkt->ReadByte ();
	creator = IPEndPoint (pkt);
	seqno = pkt->ReadByte ();
    }

    void Serialize(Packet *pkt) {
	Message::Serialize (pkt);

	pkt->WriteInt (metric_handle);
	pkt->WriteByte (ttl);
	creator.Serialize (pkt);
	pkt->WriteByte (seqno);
    }

    uint32 GetLength() {
	uint32 len = Message::GetLength ();
	return len + creator.GetLength () + 4 + 1 + 1;
    }

    void Print(FILE *stream) {
	Message::Print (stream);
	fprintf (stream, " handle=%0x ttl=%d seqno=%d creator=", metric_handle, ttl, seqno);
	creator.Print (stream);
    }

    const char *TypeString () { return "MSG_SAMPLE_REQUEST"; }
    void Print (ostream& os) {
	Message::Print (os);
	os << " handle=" << merc_va ("%0x", metric_handle) << " ttl=" << (int) ttl << 
	    " seqno=" << (int) seqno << " creator=" << creator;
    }
};

struct MsgSampleResponse : public Message {
    private:
uint32 metric_handle;
    list<Sample *> samples;
    protected:
    DECLARE_TYPE(Message, MsgSampleResponse);

    public:
    MsgSampleResponse (byte hubID, IPEndPoint& sender, uint32 handle) : 
	Message (hubID, sender), metric_handle (handle) {}

    MsgSampleResponse (const MsgSampleResponse& other);
    ~MsgSampleResponse ();

    void AddSample (Sample *s);
    list<Sample *>::iterator s_begin () { return samples.begin (); }
    list<Sample *>::iterator s_end ()  { return samples.end (); }

    uint32 GetMetricHandle () const { return metric_handle; }
    size_t size () { return samples.size (); }

    MsgSampleResponse (Packet *pkt);
    void Serialize (Packet *pkt);
    uint32 GetLength ();
    void Print (FILE *stream);

    const char *TypeString () { return "MSG_SAMPLE_RESPONSE"; }
    void Print (ostream& os);
};

struct MsgLocalLBRequest : public Message {
    private:
double load;
    NodeRange range;

    public:
    DECLARE_TYPE (Message, MsgLocalLBRequest);

    MsgLocalLBRequest (byte hubID, IPEndPoint& sender, NodeRange &range, double load) 
	: Message(hubID, sender), range (range), load (load) {}
    ~MsgLocalLBRequest () {}

    double GetLoad () const { return load; }
    const NodeRange& GetRange () const { return range; }

    MsgLocalLBRequest (Packet *pkt) : Message (pkt), range (RANGE_NONE) {
	load = (double) pkt->ReadFloat ();
	range = NodeRange (pkt);
    }

    void Serialize (Packet *pkt) {
	Message::Serialize (pkt);
	pkt->WriteFloat ((float) load);
	range.Serialize (pkt);
    }
    uint32 GetLength () {
	uint32 len = Message::GetLength () + 4 + range.GetLength ();
	return len;
    }

    void Print (FILE *stream) {
	Message::Print (stream);
	fprintf (stream, "load=%f range=", load);
	range.Print (stream);
    }

    const char *TypeString () { return "MSG_LOCAL_LB_REQUEST"; }
    void Print (ostream& os) {
	Message::Print (os);
	os << "load=" << load << " range=" << range;
    }
};

struct MsgLocalLBResponse : public Message {
    private:
NodeRange assigned_range;
    NodeRange range;
    double newload;
    public:
    DECLARE_TYPE (Message, MsgLocalLBResponse);

    MsgLocalLBResponse (byte hubID, IPEndPoint& sender, NodeRange &assigned_range, NodeRange& range, double newload) 
	: Message(hubID, sender), assigned_range (assigned_range), range (range), newload (newload) {}
    ~MsgLocalLBResponse () {} 

    const NodeRange& GetAssignedRange () const { return assigned_range; }
    const NodeRange& GetPeerNewRange () const { return range; }
    double GetNewLoad () const { return newload; }

    MsgLocalLBResponse (Packet *pkt) : Message (pkt), range (RANGE_NONE), assigned_range (RANGE_NONE) {
	assigned_range = NodeRange (pkt);
	range = NodeRange (pkt);
	newload = (double) pkt->ReadFloat ();
    }

    void Serialize (Packet *pkt) {
	Message::Serialize (pkt);
	assigned_range.Serialize (pkt);
	range.Serialize (pkt);
	pkt->WriteFloat ((float) newload);
    }
    uint32 GetLength () {
	uint32 len = Message::GetLength () + assigned_range.GetLength () + range.GetLength () + 4;
	return len;
    }

    void Print (FILE *stream) {
	Message::Print (stream);
	fprintf (stream, "assigned="); 
	assigned_range.Print (stream);
	fprintf (stream, " range=");
	range.Print (stream);
	fprintf (stream, " newload=%.3f", newload);
    }

    const char *TypeString () { return "MSG_LOCAL_LB_RESPONSE"; }
    void Print (ostream& os) {
	Message::Print (os);
	os << "assigned=" << assigned_range << " range=" << range << " newload=" << newload;
    }

};

struct MsgLeaveNotification : public Message {
    private:
NodeRange assigned_range;
    list<MsgPublication *> triggers;
    list<Interest *> subscriptions;
    float addload;
    public:
    DECLARE_TYPE (Message, MsgLeaveNotification);

    MsgLeaveNotification (byte hubID, IPEndPoint& sender, const NodeRange &assigned_range, float addload)
	: Message(hubID, sender), assigned_range (assigned_range), addload (addload) {}
    ~MsgLeaveNotification ();

    MsgLeaveNotification (Packet *pkt);
    MsgLeaveNotification (const MsgLeaveNotification& omsg);

    const NodeRange& GetAssignedRange () const { return assigned_range; }
    float GetAdditionalLoad () const { return addload; }

    void AddTrigger(MsgPublication *e);
    list<MsgPublication *>::iterator t_begin() { return triggers.begin(); }
    list<MsgPublication *>::iterator t_end() { return triggers.end(); }
    size_t t_size() { return triggers.size(); }

    void AddSubscription(Interest *s);
    list<Interest *>::iterator s_begin() { return subscriptions.begin(); }
    list<Interest *>::iterator s_end() { return subscriptions.end(); }
    size_t s_size() { return subscriptions.size(); }

    void Serialize (Packet *pkt);
    uint32 GetLength ();

    void Print (FILE *stream);

    const char *TypeString () { return "MSG_LEAVE_NOTIFICATION"; }
    void Print (ostream& os);
};

struct MsgLeaveCheckRequest : public Message {
    protected:
DECLARE_TYPE(Message, MsgLeaveCheckRequest);
    public:
MsgLeaveCheckRequest (byte hubID, IPEndPoint& sender) : Message (hubID, sender) {}
    ~MsgLeaveCheckRequest () {}
    MsgLeaveCheckRequest (Packet *pkt) : Message (pkt) {}

    void Serialize(Packet *pkt) { Message::Serialize(pkt); }
    uint32 GetLength() { return Message::GetLength(); }
    void Print(FILE *stream) { Message::Print(stream); }

    const char *TypeString () { return "MSG_LCHECK_REQUEST"; }

    void Print(ostream& os) { Message::Print(os); }
};

struct MsgLeaveCheckResponse : public Message {
    bool isok;
    protected:
    DECLARE_TYPE(Message, MsgLeaveCheckResponse);
    public:
    MsgLeaveCheckResponse (byte hubID, IPEndPoint& sender) : Message (hubID, sender), isok (true) {}
    ~MsgLeaveCheckResponse () {}
    MsgLeaveCheckResponse (Packet *pkt) : Message (pkt) {
	isok = pkt->ReadBool ();
    }

    void SetOK (bool ok) { isok = ok; }
    bool IsOK () { return isok; }

    void Serialize(Packet *pkt) { 
	Message::Serialize(pkt); 
	pkt->WriteBool (isok);		
    }
    uint32 GetLength() { 
	return Message::GetLength() + 1; 
    }
    void Print(FILE *stream) { 
	Message::Print(stream); 
	fprintf (stream, " %s", (isok ? "OK" : "NOT OK"));
    }

    const char *TypeString () { return "MSG_LCHECK_RESPONSE"; }

    void Print(ostream& os) { 
	Message::Print(os); 
	os << " " << (isok ? "OK" : "NOT OK");
    }
};

struct MsgLeaveJoinLBRequest : public Message {
    private:
//// arent really used!! 
NodeRange range;           
    float load;
    protected:
    DECLARE_TYPE(Message, MsgLeaveJoinLBRequest);

    public:
    MsgLeaveJoinLBRequest(byte hubID, IPEndPoint& sender, NodeRange &r, float l) : Message (hubID, sender), range (r), load (l) {}
    MsgLeaveJoinLBRequest(Packet *pkt) : Message (pkt), range (RANGE_NONE) { 
	range = NodeRange (pkt);
	load = pkt->ReadFloat ();
    }
    virtual ~MsgLeaveJoinLBRequest() {}

    NodeRange& GetRange() { return range; }
    float GetLoad() { return load; }

    void Serialize(Packet *pkt) {
	Message::Serialize (pkt);
	range.Serialize (pkt);
	pkt->WriteFloat (load);
    }

    uint32  GetLength() {
	uint32 retval = Message::GetLength ();
	retval += range.GetLength () + 4;
	return retval;
    }
    void Print(FILE *stream) {
	Message::Print (stream);
	fprintf (stream, " range=");
	range.Print (stream);
	fprintf (stream, " load=%f", load);
    }

    const char *TypeString() { return "MSG_LEAVEJOIN_LB_REQUEST"; }
    void Print (ostream& os) {
	Message::Print (os);
	os << " range=" << range << " load=" << load;
    }
};

struct MsgLeaveJoinDenial : public Message {
    private:
protected:
DECLARE_TYPE(Message, MsgLeaveJoinDenial);
    public:
MsgLeaveJoinDenial(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}
    MsgLeaveJoinDenial(Packet *pkt) : Message(pkt) {}
    virtual ~MsgLeaveJoinDenial() {}

    void Serialize(Packet *pkt) { Message::Serialize(pkt); }
    uint32 GetLength() { return Message::GetLength(); }
    void Print(FILE *stream) { Message::Print(stream); }

    const char *TypeString() { return "MSG_LEAVEJOIN_DENIAL"; }
    void Print(ostream& os) { Message::Print(os); }
};

struct MsgCB_EstimateReq : public Message {
    DECLARE_TYPE(Message, MsgCB_EstimateReq);

    MsgCB_EstimateReq(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}
    virtual ~MsgCB_EstimateReq() {}
    MsgCB_EstimateReq(Packet *pkt) : Message(pkt) {}

    const char *TypeString() { return "MSG_CB_ESTIMATE_REQ"; }
};

// - Ashwin [05/11]; Making 'hist' a pointer allows me to just
// do a class declaration for Histogram; changes propagate
// really fast from Message.h since it's included almost everywhere!

class Histogram;

struct MsgCB_EstimateResp : public Message {
    DECLARE_TYPE(Message, MsgCB_EstimateResp);

    Histogram *hist;

    MsgCB_EstimateResp(byte hubID, IPEndPoint& sender) : Message(hubID, sender) {}

    virtual ~MsgCB_EstimateResp() {}

    MsgCB_EstimateResp(Packet *pkt);
    void Serialize(Packet *pkt);
    uint32  GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_CB_ESTIMATE_RESP"; }
};

struct MsgNeighborRequest : public Message {
    IPEndPoint creator;
    Value val;
    int epoch;
    uint32 nonce;
    protected:
    DECLARE_TYPE (Message, MsgNeighborRequest);

    public:
    MsgNeighborRequest (byte hubID, IPEndPoint& sender, IPEndPoint& cr, Value& v, int e, uint32 n)
	: Message (hubID, sender), creator (cr), val (v), epoch (e), nonce (n) {}

    MsgNeighborRequest (Packet *pkt);
    virtual ~MsgNeighborRequest () {}

    const IPEndPoint& GetCreator () const { return creator; }
    const Value& GetValue () const { return val; }
    const int GetEpoch () const { return epoch; }
    const uint32 GetNonce () const { return nonce; }

    void Serialize (Packet *pkt);
    uint32  GetLength ();
    void Print (FILE *stream);
    void Print (ostream& os);

    const char *TypeString() { return "MSG_NBR_REQ"; }
};

struct MsgNeighborResponse : public Message {
    private:
int         epoch;
    uint32      nonce;
    NodeRange   range;           
    protected:
    DECLARE_TYPE(Message, MsgNeighborResponse);

    public:
    MsgNeighborResponse (byte hubID, IPEndPoint& sender, NodeRange &r, int ep, uint32 nonce);
    MsgNeighborResponse (Packet *pkt);
    virtual ~MsgNeighborResponse ();

    NodeRange& GetRange() { return range; }
    int GetEpoch() { return epoch; }
    uint32 GetNonce () { return nonce; }

    void Serialize(Packet *pkt);
    uint32  GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_NBR_RESP"; }
};

///////////////////////////////////////////////////////////////////////
// Utility messages

struct MsgBlob : public Message {
    DECLARE_TYPE(Message, MsgBlob);

    uint32 len;
    byte *data;

    MsgBlob(uint32 size, const IPEndPoint& sender) : Message() { 
	len  = size;
	data = new byte[size];
	bzero(data, size);
	this->sender = sender;
    }
    virtual ~MsgBlob() {
	delete[] data;
    }

    MsgBlob(Packet *pkt) : Message(pkt) { 
	len  = pkt->ReadInt();
	data = new byte[len];
	pkt->ReadBuffer(data, len);
    }
    void Serialize(Packet *pkt) { 
	Message::Serialize(pkt);
	pkt->WriteInt(len);
	pkt->WriteBuffer(data, len);
    }   
    uint32  GetLength() { return Message::GetLength() + 4 + len; }
    void Print(FILE *stream) { ASSERT(false); }

    const char *TypeString() { return "MSG_BLOB"; }
};

struct MsgCompressed : public Message {
    DECLARE_TYPE(Message, MsgCompressed);

    Message *orig;

    byte  *compBuf;
    uint32 origLen;
    uint32 compLen;

    MsgCompressed(Message *msg);
    virtual ~MsgCompressed();

    void MakeCompressed();

    MsgCompressed(Packet *pkt);
    void Serialize(Packet *pkt);
    uint32  GetLength();
    void Print(FILE *stream);

    const char *TypeString() { return "MSG_COMPRESSED"; }
};

struct MsgPing : public Message {
    DECLARE_TYPE(Message, MsgPing);

    TimeVal time;
    uint32  pingNonce;

    MsgPing() : Message() {
	time      = TIME_NONE;
	pingNonce = CreateNonce();
    };
    virtual ~MsgPing() {};

    MsgPing(Packet *pkt) : Message(pkt) { 
	time.tv_sec  = pkt->ReadInt();
	time.tv_usec = pkt->ReadInt();
	pingNonce    = pkt->ReadInt();
    }
    void Serialize(Packet *pkt) { 
	Message::Serialize(pkt);
	pkt->WriteInt(time.tv_sec);
	pkt->WriteInt(time.tv_usec);
	pkt->WriteInt(pingNonce);
    }   
    uint32  GetLength() { return Message::GetLength() + 4 + 4 + 4; }
    void Print(FILE *stream) { ASSERT(false); }

    const char *TypeString() { return "MSG_PING"; }
};

struct MsgDummy : public Message {
    DECLARE_TYPE(Message, MsgDummy);

    MsgDummy() : Message() { ASSERT(false); }
    MsgDummy(Packet *pkt) : Message(pkt) { ASSERT(false); }
    void Print(FILE *stream) { ASSERT(false); }
    const char *TypeString() { ASSERT(false); return ""; }
};


void DumpMessageTypes (FILE *fp);
#endif // __MESSAGE__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
