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

/***************************************************************************
  MercuryNode.cpp

begin          :  Nov 2, 2002
copyright      :  (C) 2002-2005 Ashwin R. Bharambe (ashu@cs.cmu.edu)
(C) 2004-2005 Jeff Pang (jeffpang@cs.cmu.edu)

**************************************************************************/

#include <fcntl.h>
#include <cctype>
#include <csignal>
#include <cmath>
#include <fstream>

#include <util/ExpLog.h>
#include <mercury/common.h>
#include <util/debug.h>
#include <mercury/Router.h>
#include <mercury/MercuryNode.h>
#include <mercury/Message.h>
#include <mercury/Utils.h>
#include <mercury/Parameters.h>
#include <mercury/BufferManager.h>
#include <util/Benchmark.h>
#include <mercury/RoutingLogs.h>
#include <mercury/BufferManager.h>
#include <mercury/Sampling.h>
#include <mercury/HubManager.h>
#include <util/Utils.h>

#include <mercury/ObjectLogs.h>       // FIXME: annoying

    pref_t g_Preferences;
ofstream g_MercEventsLog;

static const int MAX_PENDING_REQUESTS = 5;
static const int MAX_CONNECT_TRIALS = 2;	// low for testing purposes...
static const uint32 CONNECT_SLEEP_TIME = 1000;	// milliseconds

/* copied from Transport.cpp */

#ifndef streq
#define streq(a,b) (!strcmp((a),(b)))
#endif

TransportType GetProtoByName(const char *proto_name)
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

void MercuryNode::RegisterMessageHandler(MsgType type, MessageHandler *handler)
{
    HandlerMap::iterator iter = m_HandlerMap.find(type);
    if (iter == m_HandlerMap.end()) {
	vector<MessageHandler *> vec;
	vec.push_back(handler);
	m_HandlerMap.insert(HandlerMap::value_type(type, vec));
    }
    else
	iter->second.push_back(handler);
}

namespace MNODE_CPP {
    class PeriodicTimer : public Timer {
	MercuryNode *m_MercuryNode;
    public:
	PeriodicTimer (MercuryNode *mnode) : Timer (0), m_MercuryNode (mnode) {}

	void OnTimeout () { 
	    m_MercuryNode->DoPeriodic ();
	    _RescheduleTimer (1000);
	}
    };
};

MercuryNode::MercuryNode(NetworkLayer *network, Scheduler *scheduler, IPEndPoint& addr):
    Node (network, scheduler, addr), 
    Router(), m_Epoch(0), m_AllJoined(false), m_Application (new DummyApp ())
{
    m_MercuryNode = this;
    m_StartTime = TIME_NONE;
    m_Simulating = false;

    // get transport protocol
    Parameters::TransportProto = GetProtoByName(g_Preferences.merctrans);

    // logging
    if (g_MeasurementParams.enabled) {
	InitRoutingLogs(m_Address);
	// Jeff 06/29/2005: require this for DiscoveryLatLog
	InitObjectLogs(m_Address);
	/// FIXME: BEFORE GOING ETOOFAR ((RealNet *)m_Network)->EnableLog();
    }

    m_BufferManager = new BufferManager();
    m_HubManager = new HubManager(this, m_BufferManager);          // collects information about all hubs

    RegisterMessageHandler(MSG_CB_ALL_JOINED, this);
    RegisterMessageHandler(MSG_PUB, this);    // HACK: only for the matched-pubs; cleaner way might be to create a new message type

    m_Scheduler->RaiseEvent (new refcounted<MNODE_CPP::PeriodicTimer>(this), m_Address, 0);

    // open app publication log file
    DBG_DO { g_MercEventsLog.open(merc_va("MercEvents.%d", GetPort ())); }
}

MercuryNode::~MercuryNode()
{
    if (IsJoined())
	Stop();

    DBG_DO { g_MercEventsLog.close(); }
}

void MercuryNode::SendEvent(Event * evt)
{
    // make a copy of the event here, since the app can end up 
    // deleting it right away.

    /// debug
    DB_DO (10) {
	int nhubs = m_HubManager->GetNumHubs ();
	ASSERT (nhubs > 0);

	for (int i = 0; i < nhubs; i++) {
	    Hub *h = m_HubManager->GetHubByID (i);
	    if (!h->IsMine ()) 
		continue;

	    MemberHub *mh = (MemberHub *) h;

	    Constraint *cst = evt->GetConstraintByAttr (i);
	    if (cst == NULL)
		continue;

	    ASSERT (cst->GetMin () >= mh->GetAbsMin ());
	    ASSERT (cst->GetMin () <= mh->GetAbsMax ());
	    ASSERT (cst->GetMax () >= mh->GetAbsMin ());
	    ASSERT (cst->GetMax () <= mh->GetAbsMax ());
	}
    }

    m_BufferManager->EnqueueAppEvent(evt->Clone ());
}

void MercuryNode::RegisterInterest(Interest * interest)
{
    // make a copy. the app may delete it right away.
    m_BufferManager->EnqueueAppInterest(interest->Clone ());
}

Event *MercuryNode::ReadEvent()
{
    Event *ev = m_BufferManager->DequeueNetworkEvent();
    return ev;
}

#if 0
Event *MercuryNode::ReadEvent (byte type)
{
    return m_BufferManager->DequeNetworkEvent (type);
}
#endif

void MercuryNode::Start ()
{
    m_HubManager->PerformBootstrap ();

    /// SimMercuryNode does this now

    // m_Scheduler->RaiseEvent (new refcounted<MercuryNodePeriodicTimer> (this), m_Address, 0);	
}

void MercuryNode::Stop(void)
{
    m_HubManager->Shutdown();
}

bool MercuryNode::IsJoined()
{
    return m_HubManager->IsJoined();
}

void MercuryNode::ReceiveMessage (IPEndPoint *from, Message *msg)
{
    // Message dispatch logic: objects register interests in
    // which messages they wish to accept (via
    // RegisterMessageHandler); if there are multiple objects
    // for the same type, they figure out internally who it is
    // that will ultimately handle this message. the typical
    // demultiplexing happens based on hubID.  - Ashwin
    // [01/23/2005]

    HandlerMap::iterator iter = m_HandlerMap.find (msg->GetType ());
    if (iter == m_HandlerMap.end())
	MWARN <<  "received message with an unknown handler " << msg->TypeString () << endl;
    else {
	vector<MessageHandler *> *vec = &(iter->second);
	for (vector<MessageHandler *>::iterator i = vec->begin (); i != vec->end (); i++)
	    (*i)->ProcessMessage (from, msg);
    }
    m_Network->FreeMessage (msg);
}

void MercuryNode::DoPeriodic()
{ 
    // only need to do once per call -- not any more
    // since WANMercuryNode schedules us periodically
    // how will this work from the simulator?? need 
    // to create a SimMercuryNode??
    // 
    // TIME( SendApplicationPackets() );

    TimeVal now = m_Scheduler->TimeNow ();
    DBG_DO {
	PERIODIC2( 1000, now, m_HubManager->Print (stderr) );
    }
}

void MercuryNode::ProcessMessage(IPEndPoint * from, Message * msg)
{
    MsgType t = msg->GetType ();

    if (t == MSG_CB_ALL_JOINED)
	HandleAllJoined(from, (MsgCB_AllJoined *)msg);
    else if (t == MSG_PUB)
	// ONLY matched pubs handled here
	HandlePublication (from, (MsgPublication *) msg);
    else 
	MWARN << "MercuryNode: weird packet, type is: " << msg->TypeString() << endl;

    return;						/* caller deletes packet */
}

#include <fstream>
extern ofstream g_MercEventsLog;

void MercuryNode::HandlePublication (IPEndPoint *from, MsgPublication *pmsg)
{
    // XXX: so hacky!
    if (pmsg->hubID != 0xff)
	return;

    Event *ev = pmsg->GetEvent();

    // i should only be getting matched pubs here
    ASSERT (ev->IsMatched ());

    ///// MEASUREMENT
    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	DiscoveryLatEntry ent (DiscoveryLatEntry::PUB_RECV, 0, ev->GetNonce());
	LOG(DiscoveryLatLog, ent, pmsg->recvTime); // use recv time
    }
    ///// MEASUREMENT

    DBG_DO { g_MercEventsLog << "received matchedpub " << ev << endl; }
    m_BufferManager->EnqueueNetworkEvent (ev->Clone ());
}

void MercuryNode::HandleAllJoined(IPEndPoint *from, MsgCB_AllJoined *msg)
{
    m_AllJoined = true;

    MDB(-10) << "Got ALL_Joined signal! Ready to get going!" << endl;
    for (int i = 0; i < m_HubManager->GetNumHubs(); i++)
    {
	Hub *h = m_HubManager->GetHubByIndex(i);
	if (!h->IsMine())
	    continue;
	MemberHub *hub = (MemberHub *) h;

	// finish the join process for static joins
	if (hub->StaticJoin()) {
	    GetApplication ()->JoinBegin (m_Address);
	    hub->ForcePeerRefresh();
	    hub->OnJoinComplete();
	}

	NodeRange *range = hub->GetRange();
	MINFO << " me=" << m_Address << " " << range << " succ=" << hub->GetSuccessor () << endl;

	if (range)
	    DBG_DO { g_MercEventsLog << "==hub== " << hub->GetName() << " range=" << *range << endl; g_MercEventsLog.flush(); }
    }

}

vector<Constraint> MercuryNode::GetHubConstraints ()
{
    vector<Constraint> ret;

    for (int i = 0; i < m_HubManager->GetNumHubs (); i++) {
	Hub *h = m_HubManager->GetHubByIndex (i);

	Constraint c (h->GetID (), h->GetAbsMin (), h->GetAbsMax ());
	ret.push_back (c);
    }

    return ret;
}

vector< pair<int,string> > MercuryNode::GetHubNames ()
{
    vector< pair<int,string> > ret;

    for (int i = 0; i < m_HubManager->GetNumHubs (); i++) {
	Hub *h = m_HubManager->GetHubByIndex (i);
	string n = h->GetName();
	ret.push_back ( pair<int,string>(h->GetID(),n) );
    }

    return ret;
}

void MercuryNode::RegisterApplication (Application *app)
{
    ASSERT (app != NULL);
    if (m_Application) 
	delete m_Application;

    m_Application = app;
}

vector<Constraint> MercuryNode::GetHubRanges ()
{
    vector<Constraint> ret;

    for (int i = 0; i < m_HubManager->GetNumHubs (); i++) {
	Hub *h = m_HubManager->GetHubByIndex (i);
	if (!h->IsMine ()) {
	    Constraint c (h->GetID (), VALUE_NONE, VALUE_NONE);
	    ret.push_back (c);
	}
	else {
	    MemberHub *mh = (MemberHub *) h;
	    if (mh->GetRange ()) 
		ret.push_back (*mh->GetRange ());
	    else {
		Constraint c (mh->GetID (), 0, 0);     // we need a better convention
		ret.push_back (c);
	    }
	}
    }
    return ret;	
}

static Neighbor _PeerToNeighbor (Peer *p)
{
    Neighbor n (p->GetAddress (), p->GetRange (), p->GetRTTEstimate ());
    return n;
}

static void _MakeNbrsList (PList &l, vector<Neighbor> *ret)
{
    for (PeerListIter it = l.begin (); it != l.end (); it++) {
	Peer *p = static_cast<Peer *> (*it);
	ret->push_back (_PeerToNeighbor (p));
    }
}

vector<Neighbor> MercuryNode::GetSuccessors (int hubid)
{
    vector<Neighbor> ret;

    int nhubs = m_HubManager->GetNumHubs ();
    if (hubid < 0 || hubid >= nhubs) 
	return ret;

    Hub *h = m_HubManager->GetHubByIndex (hubid);
    if (!h->IsMine())
	return ret;

    MemberHub *mh = (MemberHub *) h;
    _MakeNbrsList (mh->GetSuccessorList (), &ret);

    return ret;
}

vector<Neighbor> MercuryNode::GetPredecessors (int hubid)
{
    vector<Neighbor> ret;

    int nhubs = m_HubManager->GetNumHubs ();
    if (hubid < 0 || hubid >= nhubs) 
	return ret;

    Hub *h = m_HubManager->GetHubByIndex (hubid);
    if (!h->IsMine())
	return ret;

    MemberHub *mh = (MemberHub *) h;
    _MakeNbrsList (mh->GetPredecessorList (), &ret);

    return ret;
}

vector<Neighbor> MercuryNode::GetLongNeighbors (int hubid)
{
    vector<Neighbor> ret;

    int nhubs = m_HubManager->GetNumHubs ();
    if (hubid < 0 || hubid >= nhubs) 
	return ret;

    Hub *h = m_HubManager->GetHubByIndex (hubid);
    if (!h->IsMine())
	return ret;

    MemberHub *mh = (MemberHub *) h;
    _MakeNbrsList (mh->GetLongNeighborsList (), &ret);

    return ret;
}

MemberHub *MercuryNode::GetHub (int hubid)
{
    int nhubs = m_HubManager->GetNumHubs ();

    // indicate error better!
    if (hubid < 0 || hubid >= nhubs)
	return NULL;

    Hub *h = m_HubManager->GetHubByIndex (hubid);

    if (!h->IsMine())
	return NULL; 

    return (MemberHub *) h;
}

int MercuryNode::RegisterSampler (int hubid, Sampler *s)
{
    MemberHub *h = GetHub (hubid);
    if (!h) return -1;

    h->RegisterMetric (s);
    return 0;
}
int MercuryNode::UnRegisterSampler (int hubid, Sampler *s)
{
    MemberHub *h = GetHub (hubid);
    if (!h) return -1;

    h->UnRegisterMetric (s);
    return 0;
}

int MercuryNode::GetSamples (int hubid, Sampler *s, vector<Sample *> *ret)
{
    MemberHub *h = GetHub (hubid);
    if (!h) return -1;

    h->GetSamples (s, ret);
    return 0;
}

int MercuryNode::RegisterLoadSampler (int hubid, LoadSampler *sampler)
{
    MemberHub *h = GetHub (hubid);
    if (!h) return -1;

    h->RegisterLoadSampler (sampler);
    return 0;
}
int MercuryNode::UnRegisterLoadSampler (int hubid, LoadSampler *sampler)
{
    MemberHub *h = GetHub (hubid);
    if (!h) return -1;

    h->UnRegisterLoadSampler (sampler);
    return 0;
}

bool MercuryNode::SendPacket ()
{
    PubsubData *pdu = m_BufferManager->DequeueAppData();

    if (!pdu)
	return false;

    if (pdu->IsPub()) {
	MsgPublication *pmsg = new MsgPublication((byte) 0, m_Address, pdu->m_Event, m_Address);  /* temporary hub id */

	m_HubManager->SendAppPublication(pmsg);

	delete pdu->m_Event;  // we had copied it from the app in SendEvent ()
	delete pmsg;
    } 
    else {
	ASSERT(pdu->IsSub());

	pdu->m_Interest->SetSubscriber (m_Address);

	MsgSubscription *smsg = new MsgSubscription((byte) 0, m_Address, pdu->m_Interest, m_Address); /* temporary hub id */
	m_HubManager->SendAppSubscription(smsg);

	delete pdu->m_Interest; // we had copied it from the app in RegisterInterest ()
	delete smsg;
    }

    delete pdu;
    return true;
}

void MercuryNode::SendApplicationPackets()
{
    int numPubs = 0, numSubs = 0;

    PubsubData *pdu = 0;
    while ((pdu = m_BufferManager->DequeueAppData()) != 0) {
	if (pdu->IsPub()) {
	    MsgPublication *pmsg = new MsgPublication((byte) 0, m_Address, pdu->m_Event, m_Address);  /* temporary hub id */

	    m_HubManager->SendAppPublication(pmsg);

	    delete pdu->m_Event;  // we had copied it from the app in SendEvent ()
	    delete pmsg;

	    numPubs++;
	} else {
	    ASSERT(pdu->IsSub());

	    pdu->m_Interest->SetSubscriber (m_Address);
	    MsgSubscription *smsg = new MsgSubscription((byte) 0, m_Address, pdu->m_Interest, m_Address); /* temporary hub id */

	    m_HubManager->SendAppSubscription(smsg);

	    delete pdu->m_Interest; // we had copied it from the app in RegisterInterest ()
	    delete smsg;

	    numSubs++;
	}
	delete pdu;
    }

    //NOTE(Application Pubs, numPubs);
    //NOTE(Application Subs, numSubs);
}

void MercuryNode::PrintPeerList(FILE * stream)
{
    m_HubManager->Print(stream);
}

void MercuryNode::Print(FILE * stream)
{
}

void MercuryNode::PrintPublicationList(ostream& stream)
{
    m_HubManager->PrintPublicationList(stream);
}

void MercuryNode::PrintSubscriptionList(ostream& stream)
{
    m_HubManager->PrintSubscriptionList(stream);
}

// OK: one can easily disable address printing by checking some preference
// which says if we are running in simulated mode or not.
ostream& MercuryNode::croak (int mode, int lvl, const char *file, const char *func, int line)
{
    int color = 31 + (m_Address.GetPort () % 6);

    ostream& os = __g_DebugStream (mode, lvl, file, func, line);

    if (m_Simulating)
	return os << "[" << color << "m" << GetAddress () << "[m ";
    else 
	return os << " ";
    /*
      else if (mode == DBG_MODE_WARN)
      return WARN << "[" << color << "m" << GetAddress () << "[m ";
      else 
      return __g_DebugStream (mode, lvl, file, func, line) << "[" << color << "m" << GetAddress () << "[m ";
    */
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
