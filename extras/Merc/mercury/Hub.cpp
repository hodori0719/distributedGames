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

#include <mercury/common.h>
#include <mercury/Message.h>
#include <mercury/NetworkLayer.h>
#include <mercury/Peer.h>
#include <mercury/LinkMaintainer.h>
#include <mercury/MercuryNode.h>
#include <mercury/PubsubRouter.h>
#include <mercury/Hub.h>
#include <mercury/Sampling.h>
#include <mercury/BufferManager.h>
#include <mercury/Cache.h>
#include <mercury/Parameters.h>
#include <mercury/Scheduler.h>
#include <mercury/ObjectLogs.h>
#include <mercury/Sampling.h>
#include <mercury/Histogram.h>
#include <mercury/MetricInfo.h>
#include <mercury/LoadBalancer.h>

HubInitInfo::HubInitInfo() : rep((uint32)0,0), staticjoin(false) {}

HubInitInfo::HubInitInfo(const HubInitInfo& h) {
    HubInitInfo& hh = (HubInitInfo&) h;
    
    ID = hh.ID;
    name = hh.name;
    absmin = hh.absmin;
    absmax = hh.absmax;
    isMember = hh.isMember;
    rep = hh.rep;
    
    staticjoin = hh.staticjoin;
    range = hh.range;
    succs = hh.succs;
    preds = hh.preds;
}

HubInitInfo::HubInitInfo(Packet *pkt) {
    ID = pkt->ReadByte();
    pkt->ReadString(name);
    rep = IPEndPoint(pkt);
    isMember = (bool) pkt->ReadByte();
    absmin = Value(pkt);
    absmax = Value(pkt);
    
    staticjoin = pkt->ReadBool();
    if (staticjoin) {
	uint32 sz;
	
	range = NodeRange(pkt);
	
	sz = pkt->ReadInt();
	for (uint32 i=0; i<sz; i++) {
	    succs.push_back( PeerInfo(pkt) );
	}
	
	sz = pkt->ReadInt();
	for (uint32 i=0; i<sz; i++) {
	    preds.push_back( PeerInfo(pkt) );
	}
    }
}

HubInitInfo::~HubInitInfo() {}

void HubInitInfo::Serialize(Packet *pkt) {
    pkt->WriteByte(ID);
    pkt->WriteString(name);
    rep.Serialize(pkt);
    pkt->WriteByte((byte)isMember);
    absmin.Serialize(pkt);
    absmax.Serialize(pkt);
    
    pkt->WriteBool(staticjoin);
    if (staticjoin) {
	range.Serialize(pkt);
	pkt->WriteInt(succs.size());
	for (PeerInfoList::iterator it = succs.begin(); it != succs.end(); it++) {
	    it->Serialize(pkt);
	}
	pkt->WriteInt(preds.size());
	for (PeerInfoList::iterator it = preds.begin(); it != preds.end(); it++) {
	    it->Serialize(pkt);
	}
    }
}

uint32 HubInitInfo::GetLength() {
    uint32 len = 0;
    
    len +=
	// Jeff: the null byte in the string is NOT serialized
	1 + (name.length()/* + 1*/ + 4) + rep.GetLength() + 
	1 + absmin.GetLength() + absmax.GetLength();
    
    len += 1;
    
    if (staticjoin) {
	len += range.GetLength();
	len += 4;
	for (PeerInfoList::iterator it = succs.begin(); it != succs.end(); it++) {
	    len += it->GetLength();
	}
	len += 4;
	for (PeerInfoList::iterator it = preds.begin(); it != preds.end(); it++) {
	    len += it->GetLength();
	}
    }
    return len;
}

void HubInitInfo::Print (FILE *stream) {
    fprintf (stream, "id=%d name=%s ismember=%d", ID, name.c_str (), isMember);
    fprintf (stream, " absmin=");
    absmin.Print (stream);
    
    fprintf (stream, " absmax=");
    absmax.Print (stream);
}


struct less_peer_t {
    Value myself;      // wrapped-up circle is a nuisance

    less_peer_t (const Value& v) : myself (v) {}

    bool operator () (const ref<Peer> ap, const ref<Peer> bp) const {
	const Value& av = ap->GetRange ().GetMin ();
	const Value& bv = bp->GetRange ().GetMin ();
	
	// Jeff: enforce that myself goes last, otherwise we can set ourselves
	// to be a predecessor of ourself!
	if (av == myself)
	    return false;
	if (bv == myself)
	    return true;
	    
	if ((av <= myself && bv <= myself) || (av >= myself && bv >= myself))
	    return av > bv;
	if (av <= myself && bv >= myself)
	    return true;
	return false;
    }
};

struct greater_peer_t {
    Value myself;      // wrapped-up circle is a nuisance

    greater_peer_t (const Value& v) : myself (v) {}

    bool operator () (const ref<Peer> ap, const ref<Peer> bp) const {
	const Value& av = ap->GetRange ().GetMin ();
	const Value& bv = bp->GetRange ().GetMin ();

	if ((av <= myself && bv <= myself) || (av >= myself && bv >= myself))
	    return av < bv;
	if (av <= myself && bv >= myself)
	    return false;
	return true;
    }
};

Hub::Hub(MercuryNode *mnode, HubInitInfo &initinfo)
    : m_MercuryNode (mnode), m_Initinfo(initinfo)
{
    if (g_Preferences.use_cache) {
	g_Preferences.cache_type = CACHE_LRU;
	if (g_Preferences.cache_type == CACHE_LRU)
	    m_Cache = new LRUCache(g_Preferences.cache_size, this);
	else
	    m_Cache = new SingleCache(this);
    } else {
	m_Cache = 0;			/* make sure we segfault otherwise! :) */
    }
}

void Hub::HandleAck(IPEndPoint * from, MsgAck * amsg)
{
    if (amsg->hubID != GetID())
	return;

    if (g_Preferences.use_cache) {
	CacheEntry *entry = new CacheEntry(amsg->sender, amsg->GetRange());
	m_Cache->InsertEntry(entry);
    }
}

void Hub::Print(FILE *stream)
{
    fprintf(stream, "(Hub;id=%d name=%s rep=%s", m_Initinfo.ID, m_Initinfo.name.c_str(), m_Initinfo.rep.ToString());
}

ostream& operator<<(ostream& out, Hub *hub)
{
    out << "name=" << hub->m_Initinfo.name << " ";
    out << "rep=" << hub->m_Initinfo.rep.ToString() << " ";
    return out;
}

ostream& operator<<(ostream& out, HubInitInfo *info)
{
    out << "id=" << (int) info->ID << " name=" << info->name << " ismember=" << info->isMember;
    out << " rep=" << info->rep << " absmin=" << info->absmin << " absmax=" << info->absmax;
    out << " staticjoin=" << info->staticjoin;
    if (info->staticjoin) {
	out << " range=" << info->range;
	out << " succs=[";
	for (PeerInfoList::iterator it=info->succs.begin();
	     it != info->succs.end(); it++) {
	    if (it != info->succs.begin())
		out << " ";
	    out << "(" << it->GetAddress() << "," << it->GetRange()
		<< ")";
	}
	out << "] preds=[";
	for (PeerInfoList::iterator it=info->preds.begin();
	     it != info->preds.end(); it++) {
	    if (it != info->preds.begin())
		out << " ";
	    out << "(" << it->GetAddress() << "," << it->GetRange()
		<< ")";
	}
	out << "]";
    }

    return out;
}

ostream& operator<<(ostream& out, HubInitInfo& info) {
    return out << &info;
}

NonMemberHub::NonMemberHub(MercuryNode *mnode, BufferManager *bm, HubInitInfo &initinfo)
    : Hub(mnode, initinfo), m_BufferManager (bm)
{
    mnode->RegisterMessageHandler(MSG_ACK, this);
    //	mnode->RegisterMessageHandler(MSG_PUB, this);
}

void NonMemberHub::Print(FILE * stream)
{
    Hub::Print(stream);
}

ostream& operator<<(ostream& out, NonMemberHub *hub)
{
    return out << (Hub *) hub;
}

void NonMemberHub::ProcessMessage(IPEndPoint *from, Message *msg)
{
    MsgType t = msg->GetType();

    if (t == MSG_ACK) 
	Hub::HandleAck(from, (MsgAck *) msg);
    else  
	WARN << merc_va("Whoa! received some idiotic message in NonMemberHub [%s]", msg->TypeString()) << endl;
}

class NCHistogramMaker : public Timer {
    MemberHub *m_Hub;

public:
    NCHistogramMaker (MemberHub *hub) : m_Hub (hub), Timer (0) {}

    void OnTimeout () {
	m_Hub->MakeHistogram ();

	_RescheduleTimer (Parameters::NCHistoBuildInterval);
    }
};

#define NBR_PRINTER_INTERVAL 30000

class NbrPrinter : public Timer {
    MemberHub *m_Hub;
public:
    NbrPrinter (MemberHub *h) : Timer (NBR_PRINTER_INTERVAL), m_Hub (h) {}
    void OnTimeout () {
	//	DB_DO (-2) 
	if (!g_Preferences.nosuccdebug) {
	    PrintLine (cerr, '-');
	    cerr << "me=" << m_Hub->GetAddress () << " range=" << m_Hub->GetRange () << "(" << merc_va ("%.3f", m_Hub->GetMercuryNode ()->GetRelativeTime ()) << ")" << endl;
	    cerr << "SUCCESSORS " << endl;
	    PList& sl = m_Hub->GetSuccessorList ();
	    for (PeerListIter it = sl.begin (); it != sl.end (); ++it) {
		ref<Peer>& p = *it;

		cerr << p->GetAddress () << " " << p->GetRange () << endl; 
	    }

	    cerr << "PRECESSORS" << endl;
	    {
		PList& pl = m_Hub->GetPredecessorList ();
		for (PeerListIter it = pl.begin (); it != pl.end (); ++it) {
		    ref<Peer>& p = *it;

		    cerr << p->GetAddress () << " " << p->GetRange () << endl;
		}
	    }
	    cerr << "LONGNBRS" << endl;
	    {
		PList& pl = m_Hub->GetLongNeighborsList ();
		for (PeerListIter it = pl.begin (); it != pl.end (); ++it) {
		    ref<Peer>& p = *it;

		    cerr << p->GetAddress () << " " << p->GetRange () << endl;
		}
	    }
	    cerr << "REV longnbrs" << endl;
	    {
		PList& pl = m_Hub->GetReverseLongNeighborsList ();
		for (PeerListIter it = pl.begin (); it != pl.end (); ++it) {
		    ref<Peer>& p = *it;

		    cerr << p->GetAddress () << " " << p->GetRange () << endl;
		}
	    }
	    PrintLine (cerr, '-');

	    _RescheduleTimer (NBR_PRINTER_INTERVAL);
	}
    }
};

/////////////////////////////   PeerHeartbeatTimer  ////////////////////////
//
class PeerHeartbeatTimer : public Timer {
    MemberHub *m_Hub;	
public:
    PeerHeartbeatTimer(MemberHub *h, int timeout) : Timer(timeout), m_Hub (h) {}

    void OnTimeout() {
	m_Hub->PingPeers ();

	_RescheduleTimer ((int) (Parameters::PeerPingInterval));
    }
};

/////////////////////////////   CheckPeerHeartbeatTimer  ////////////////////////
//
class CheckPeerHeartbeatTimer : public Timer {
    MemberHub *m_Hub;	
public:
    CheckPeerHeartbeatTimer (MemberHub *h, int timeout) : Timer(timeout), m_Hub (h) {}

    void OnTimeout() {
	m_Hub->CheckLiveness ();
	_RescheduleTimer ((int) (Parameters::PeerPongTimeout));
    }
};


MemberHub::MemberHub(MercuryNode *mnode, IPEndPoint& bootstrap, 
		     BufferManager *bufferManager, HubInitInfo &info) :
    Hub (mnode, info), m_Network (mnode->GetNetwork ()),
    m_Scheduler (mnode->GetScheduler ()), m_Address (mnode->GetAddress ()), 
    m_BootstrapIP (bootstrap), m_NCHistogram (NULL),
    m_PeerHeartbeatTimer (new refcounted<PeerHeartbeatTimer> (this, 0)),
    m_CheckPeerHeartbeatTimer (new refcounted<CheckPeerHeartbeatTimer> (this, 0)),
    m_BootstrapHeartbeatTimer (NULL),
    m_SuccListTimer (NULL),

    m_SuccessorList (PList (this, m_PeersByAddress, PEER_SUCCESSOR)),
    m_LongNeighborsList (PList (this, m_PeersByAddress, PEER_LONG_NBR)),
    m_PredecessorList (PList (this, m_PeersByAddress, PEER_PREDECESSOR)),
    m_ReverseLongNeighborsList (PList (this, m_PeersByAddress, PEER_REV_LONG_NBR))

{
    m_BufferManager = bufferManager;

    m_Status = ST_UNJOINED;
    m_Range = NULL;

    m_HistogramMaintainer = new HistogramMaintainer(this);
    m_LinkMaintainer = new LinkMaintainer(this);
    m_PubsubRouter   = new PubsubRouter(this, m_BufferManager, m_LinkMaintainer);
    m_LoadBalancer = NULL;

    MsgType msgs[] = { MSG_ACK, MSG_LIVENESS_PING, MSG_LIVENESS_PONG,
		       MSG_POINT_EST_REQ, MSG_POINT_EST_RESP, 
		       MSG_SAMPLE_REQ, MSG_SAMPLE_RESP 
    };

    for (uint32 i = 0; i < sizeof (msgs) / sizeof (MsgType); i++)
	m_MercuryNode->RegisterMessageHandler(msgs[i], this);

    if (g_Preferences.do_loadbal) {
	ASSERT (g_Preferences.loadbal_delta >= (float) sqrt (2.0f));
	g_Preferences.distrib_sampling = true;
    }

    if (g_Preferences.distrib_sampling) {
	if (g_Preferences.loadbal_routeload) 
	    m_LoadSampler = new PubsubLoadSampler (this);
	else
	    m_LoadSampler = new DummyLoadSampler (this);
	m_NCSampler = new NodeCountSampler (this);

	RegisterMetric (m_LoadSampler);
	RegisterMetric (m_NCSampler);

	m_Scheduler->RaiseEvent (new refcounted<NCHistogramMaker> (this), m_Address, Parameters::NCHistoBuildInterval);

	// m_NCHistogram = InitializeHistogram (NC_HISTOGRAM_BUCKETS, GetID (), GetAbsMin (), GetAbsMax ());
    }

    if (g_Preferences.do_loadbal)
	m_LoadBalancer = new LoadBalancer (this);

    m_Scheduler->RaiseEvent (new refcounted<NbrPrinter> (this), m_Address, 500);
    m_Scheduler->RaiseEvent (m_PeerHeartbeatTimer, m_Address, m_PeerHeartbeatTimer->GetNextDelay ());	
    m_Scheduler->RaiseEvent (m_CheckPeerHeartbeatTimer, m_Address, m_CheckPeerHeartbeatTimer->GetNextDelay ());	
}

void PList::Print (ostream& os) 
{
    for (PeerListIter it = m_List.begin (); it != m_List.end (); it++) {
	// if (it != m_List.begin ()) cerr << ", ";

	ref<Peer>& p = *it;
	cerr << static_cast<Peer *> (p) << endl;
    }
}

ref<Peer> PList::Register (const IPEndPoint& addr, const NodeRange& range)
{
    SIDPeerMapIter it = m_PMap.find (addr);
    if (it == m_PMap.end ()) {
	ref<Peer> t = new refcounted<Peer> (addr, range, m_Hub->GetMercuryNode (), m_Hub);
	m_PMap.insert (SIDPeerMap::value_type (t->GetAddress (), t));
	return t;
    }

    ref<Peer> p = it->second;
    if (range != RANGE_NONE)
	p->SetRange (range);
    return p;
}

void PList::UnRegister (const IPEndPoint& addr)
{
    m_PMap.erase (addr);
}

void PList::Add (const IPEndPoint& addr, const NodeRange& range, listpos_t where)
{
    ref<Peer> p = Register (addr, range);
    p->AddPeerType (m_PeerType);

    for (PeerListIter it = m_List.begin (); it != m_List.end (); ++it) {
	ref<Peer> t = *it;
	if (t->GetAddress () == p->GetAddress ())
	    return;
    }

    if (where == PLIST_FRONT)
	m_List.push_front (p);
    else
	m_List.push_back (p);
}

Peer* PList::Lookup (const IPEndPoint& addr)
{
    for (PeerListIter it = m_List.begin (); it != m_List.end (); ++it) {
	ref<Peer> t = *it;
	if (t->GetAddress () == addr)
	    return t;
    }
    return NULL;
}

void PList::remove_peer (ref<Peer> p)
{
    p->RemovePeerType (m_PeerType);
    if (p->GetPeerType () == PEER_NONE)
	UnRegister (p->GetAddress ());    
}

bool specialDebug = false;

void PList::Remove (const IPEndPoint& addr)    
{
    for (PeerListIter it = m_List.begin (); it != m_List.end (); ++it) {
	ref<Peer> t = *it;

	if (t->GetAddress () == addr) {
	    remove_peer (t);
	    m_List.erase (it);
	    return;
	}
    }
}

void PList::Clear ()
{
    for (PeerListIter it = m_List.begin (); it != m_List.end (); ++it) {
	remove_peer (*it);
    }
    m_List.clear ();
}

void MemberHub::CheckLiveness ()
{    
    TimeVal now = m_Scheduler->TimeNow ();

    if (GetStatus() != ST_JOINED)
	return;

    // we ping these people and expect them to reply
    // OR these people ping us, so garbage collect them if they havent

    for (SIDPeerMapIter it = m_PeersByAddress.begin (); it != m_PeersByAddress.end (); /* nothing */) {
	ref<Peer> p = it->second;

	// dont expect anything from ourselves :)
	if (p->GetAddress () == m_Address) {
	    ++it;
	    continue; 
	}

	bool skip = false;
	if ((p->IsSuccessor () || p->IsLongNeighbor ()) && p->LostPongs ()) {
	    SIDPeerMapIter oit (it);     // the removals may invalidate the iterator
	    ++oit;

	    ref<Peer> tmp = new refcounted<Peer> (*p);
	    if (p->IsSuccessor ())
		m_SuccessorList.Remove (p->GetAddress ());
	    if (p->IsLongNeighbor ())
		m_LongNeighborsList.Remove (p->GetAddress ());

	    m_LinkMaintainer->OnPeerDeath (tmp);
	    it = oit;
	    skip = true;
	}
	if (p->IsPredecessor () && p->LostPings (PEER_SUCCESSOR)) {
	    SIDPeerMapIter oit (it);
	    ++oit;

	    ref<Peer> tmp = new refcounted<Peer> (*p);

	    m_PredecessorList.Remove (p->GetAddress ());
	    m_LinkMaintainer->OnPeerDeath (tmp);
	    it = oit;
	    skip = true;
	}
	if (p->IsReverseLongNeighbor () && p->LostPings (PEER_LONG_NBR)) {
	    SIDPeerMapIter oit (it);
	    ++oit;

	    ref<Peer> tmp = new refcounted<Peer> (*p);

	    m_ReverseLongNeighborsList.Remove (p->GetAddress ());
	    m_LinkMaintainer->OnPeerDeath (tmp);
	    it = oit;
	    skip = true;	    
	}

	if (!skip)
	    ++it;
    }
}

class SlowPingTimer : public Timer {
    const static uint32 PINGS_PER_INTERVAL = 10;

    PeerList m_ToPingList;
    uint32 m_Interval;
public:
    SlowPingTimer (PeerList toping) : Timer (0), m_ToPingList (toping) {
	// Jeff: if the ping interval is short, then we must space the pings
	// more closely, otherwise they will run into the next set...
	m_Interval = 
	    MIN(750, Parameters::PeerPingInterval/
		(m_ToPingList.size()/PINGS_PER_INTERVAL+1));
    }

    void OnTimeout () {
	for (int i = 0; i < (int)PINGS_PER_INTERVAL; i++) { 
	    if (m_ToPingList.size () <= 0)
		return;

	    ref<Peer> p = m_ToPingList.front ();
	    p->SendLivenessPing ();
	    m_ToPingList.pop_front ();
	}

	_RescheduleTimer (m_Interval);
    }
};

void MemberHub::PingPeers ()
{
    TimeVal &now = m_Scheduler->TimeNow ();

    MTDB (-1) << " about to ping peers for hub " << (int) GetID () << endl;
    if (GetStatus() != ST_JOINED)
	return;

    // send ping to successors
    // send ping to long neighbors

    MTDB (-1) << " pinging peers for hub " << (int) GetID () << endl;

    PeerList toping;

    for (SIDPeerMapIter it = m_PeersByAddress.begin (); it != m_PeersByAddress.end (); ++it)
    {
	ref<Peer>& p = it->second;
	if (!p->IsSuccessor () && !p->IsLongNeighbor ())
	    continue;

	// dont send stuff to ourselves :)
	if (p->GetAddress () == m_Address)
	    continue; 

	if ((sint64)(now - p->GetLastPingTime ()) >= (sint64) Parameters::PeerPingInterval) {
	    toping.push_back (p);
	}
    }

    // if n=10000, #succ=#lnbr <= 25
    // => there are at most 50 peers you should be pinging.
    // send pings to 10 people at once, separated by 1 second.

    m_Scheduler->RaiseEvent (new refcounted<SlowPingTimer> (toping), m_Address, 0);

    // dont send pings to predecessors; expect THEM to send pings to you
}

MemberHub::~MemberHub () {
    if (m_NCHistogram)
	delete m_NCHistogram;
    /// XXX should cancel NCHistogramMaker too!! 
}

void MemberHub::ProcessMessage(IPEndPoint *from, Message *msg)
{
    if (msg->hubID != GetID())
	return;

    MsgType t = msg->GetType();

    if (t == MSG_ACK)
	Hub::HandleAck(from, (MsgAck *) msg);
    else if (t == MSG_LIVENESS_PING)
	HandleLivenessPing (from, (MsgLivenessPing *) msg);
    else if (t == MSG_LIVENESS_PONG)
	HandleLivenessPong (from, (MsgLivenessPong *) msg);
    else if (t == MSG_SAMPLE_REQ)
	HandleSampleRequest (from, (MsgSampleRequest *) msg);
    else if (t == MSG_SAMPLE_RESP)
	HandleSampleResponse (from, (MsgSampleResponse *) msg);
    else if (t == MSG_POINT_EST_REQ)
	HandlePointEstimateRequest (from, (MsgPointEstimateRequest *) msg);
    else if (t == MSG_POINT_EST_RESP)
	HandlePointEstimateResponse (from, (MsgPointEstimateResponse *) msg);
    else 		
	WARN << merc_va("Whoa! received some idiotic message in MemberHub [%s]", msg->TypeString()) << endl;
}

void MemberHub::SetRange (NodeRange *range)
{
    bool changed = false;

    if (m_Range && range && *m_Range != *range) {
	changed = true;
    }

    if (m_Range)
	delete m_Range;

    if (range) 
	m_Range = new NodeRange (*range);
    else
	m_Range = NULL;

    if (changed)
	m_PubsubRouter->SetRangeChanged ();
    MTDB (-1) << " setting range to " << m_Range << endl;
}


// Set the successor list (obtained from the MsgJoinResponse)

void MemberHub::SetSuccessorList (PeerInfoList *ml)
{    
    m_SuccessorList.Clear ();

    for (PeerInfoListIter it = ml->begin(); it != ml->end(); it++)
    {
	PeerInfo& inf = *it;

	// am I in the successor list? looks like we wrapped around!
	if (inf.addr == m_Address)
	    break;
	if (m_SuccessorList.Size () > Parameters::NSuccessorsToKeep)
	    break;

	m_SuccessorList.Add (inf.addr, inf.range);
    }

    // if (m_Range)
    // 	m_SuccessorList.Sort (greater_peer_t (m_Range->GetMin ()));
    UpdateSortedPeerList();
}

// All we want to do is a simple thing: our succlist = succ + (succ's
// list - last guy) But, we probably should not erase the old list and
// re-create a new one since we will lose "information" we might have
// collected about our successors. For example, at some future point, we
// will presumably have some RTT information; so we do this more complex
// merge... - Ashwin [01/21/2005]

// a better way is to maintain RTT information separately about locations
// indexed by SIDs. so, here we can get rid of the old list too. however, 
// i think the reason we still want to carefully merge the list is to 
// make sure our successorlist remains correct from our POV. - Ashwin [02/20/2005]

// i have gone back to a simple "just use what our successor says" 
// model, instead of carefully merging the list. - Ashwin [07/17/2005]

void MemberHub::MergeSuccessorList (PeerInfoList *newlist)
{
    // Jeff: what the heck was this for? we already skip this below...
    //if ((int) newlist->size () > Parameters::NSuccessorsToKeep)
    //   newlist->pop_back (); // remove the last guy

    // Jeff 2/12/2005: (in regards to the long message above this
    // method) Forget RTT information, we *must* perform the more
    // careful merge of the successor lists because discarding the old
    // peers will make us lose all the timeout info for our peers!
    // This severely screws up timeout processing when the successor
    // list maintenance interval is "short" (relative to the ping or
    // pong intervals)
    //
    // xxx: would be more efficient to keep the succ list indexed by
    // SID, but I don't want to futz with too much stuff to fix this

    //SetSuccessorList (newlist);

    // delete succs that are not in what our succ gave us
    SIDSet newsuccs;
    // add all succs that our succ gave us (existing ones will be modified)
    for (PeerInfoListIter it = newlist->begin(); it != newlist->end(); it++)
    {
	PeerInfo& inf = *it;

	// am I in the successor list? looks like we wrapped around!
	if (inf.addr == m_Address)
	    break;
	if ((int)newsuccs.size () > Parameters::NSuccessorsToKeep)
	    break;
	
	newsuccs.insert(inf.addr);
    }
    SIDSet remove;
    for (PeerListIter it = m_SuccessorList.begin(); 
	 it != m_SuccessorList.end(); it++) {
	ptr<Peer> pr = *it;
	
	if (newsuccs.find(pr->GetAddress()) == newsuccs.end()) {
	    remove.insert(pr->GetAddress());
	}
    }
    for (SIDSetIter it = remove.begin(); it != remove.end(); it++) {
	m_SuccessorList.Remove(*it);
    }

    // add all succs that our succ gave us (existing ones will be modified)
    for (PeerInfoListIter it = newlist->begin(); it != newlist->end(); it++)
    {
	PeerInfo& inf = *it;

	// am I in the successor list? looks like we wrapped around!
	if (inf.addr == m_Address)
	    break;
	if (m_SuccessorList.Size () > Parameters::NSuccessorsToKeep)
	    break;
	
	m_SuccessorList.Add (inf.addr, inf.range);
    }
    
    // sort the succ list to ensure its still in the right order
    m_SuccessorList.Sort (greater_peer_t (m_Range->GetMin ()));
    
    // update the peer list
    UpdateSortedPeerList();
}

void MemberHub::PrintSuccessorList ()
{
    cerr << "successor list=[" << endl;
    m_SuccessorList.Print (cerr);
    cerr << "]" << endl;
}

void MemberHub::PrintPredecessorList ()
{
    cerr << "predecessor list=[" << endl;
    m_PredecessorList.Print (cerr);
    cerr << "]" << endl;
}

void MemberHub::PrintLongNeighborList ()
{
    cerr << "long neighbor list=[" << endl;
    m_LongNeighborsList.Print (cerr);
    cerr << "]" << endl;
}

void MemberHub::PrintSortedPeers ()
{
    cerr  << "sorted peers=[" << endl;
    for (PeerMapIter it = m_SortedPeers.begin (); it != m_SortedPeers.end (); ++it)
    {
	if (it != m_SortedPeers.begin ()) cerr << ", ";
	Peer *p = it->second;
	cerr << p;
    }
}

Peer *MemberHub::LookupSuccessor (const IPEndPoint& addr)
{
    return m_SuccessorList.Lookup (addr);
}

Peer *MemberHub::LookupLongNeighbor (const IPEndPoint& addr)
{
    return m_LongNeighborsList.Lookup (addr);
}

Peer *MemberHub::LookupPeer (const IPEndPoint& addr)
{
    SIDPeerMapIter it = m_PeersByAddress.find (addr);
    if (it != m_PeersByAddress.end ())
	return it->second;
    return NULL;    
}

Peer *MemberHub::GetPredecessor() { 
    if (m_PredecessorList.Empty ())
	return NULL;
    return m_PredecessorList.Front (); 
}

Peer *MemberHub::GetSuccessor() { 
    if (m_SuccessorList.Empty ())
	return NULL;
    return (m_SuccessorList.Front()); 
}

void MemberHub::ClearPredecessor () 
{
    m_PredecessorList.Clear ();
}

void MemberHub::AddLongNeighbor (const IPEndPoint& addr, const NodeRange& range)
{
    DB_DO (-1) { 
	if (m_LongNeighborsList.Lookup (addr) == NULL) { 
	    MTDB (-1) << " adding LONG neighbor " << addr << "; " << range << endl;
	}
    }
    m_LongNeighborsList.Add (addr, range);
    UpdateSortedPeerList ();
}

void MemberHub::AddPredecessor (const IPEndPoint& addr, const NodeRange& range)
{
    DB_DO (-1) { 
	if (m_PredecessorList.Lookup (addr) == NULL) { 
	    MTDB (-1) << " adding predecessor " << addr << "; " << range << endl;
	}
    }
    m_PredecessorList.Add (addr, range);
    if (m_Range)
	m_PredecessorList.Sort (less_peer_t (m_Range->GetMin ()));
    UpdateSortedPeerList ();
}

void MemberHub::AddReverseLongNeighbor (const IPEndPoint& addr, const NodeRange& range)
{
    DB_DO (-1) { 
	if (m_ReverseLongNeighborsList.Lookup (addr) == NULL) { 
	    MTDB (-1) << " adding reverse long neighbor " << addr << "; " << range << endl;
	}
    }
    m_ReverseLongNeighborsList.Add (addr, range);
}

void MemberHub::SetPredecessor (const IPEndPoint& addr, const NodeRange& range)
{
    DB_DO (-1) { 
	MTDB (-1) << " setting predecessor " << addr << "; " << range << endl;
    }
    m_PredecessorList.Add (addr, range, PLIST_FRONT);
    if (m_Range)
	m_PredecessorList.Sort (less_peer_t (m_Range->GetMin ()));

    UpdateSortedPeerList();
}

// Get rid of the successor list. and set this 
// dude to be out successor. This method is (should be) 
// called only when we are joining...
// 
void MemberHub::SetSuccessor (const IPEndPoint& addr, const NodeRange& range) 
{
    DB_DO (-1) { 
	MTDB (-1) << " setting successor " << addr << "; " << range << endl;
    }
    m_SuccessorList.Clear ();
    m_SuccessorList.Add (addr, range, PLIST_FRONT);
    UpdateSortedPeerList ();
}

void MemberHub::MergeSuccessor (const IPEndPoint& addr, const NodeRange& range)
{
    MTDB(10) << " merging successor " << addr << ";" << range << " into succlist" << endl;
    
    m_SuccessorList.Add (addr, range);
    if (m_Range)
	m_SuccessorList.Sort (greater_peer_t (m_Range->GetMin ()));
}

void MemberHub::RemovePeer (const IPEndPoint addr) 
{
    SIDPeerMapIter it = m_PeersByAddress.find (addr);
    if (it == m_PeersByAddress.end ())
	return;

    ref<Peer> p = it->second;

    if (p->IsSuccessor ())
	m_SuccessorList.Remove (addr);
    if (p->IsLongNeighbor ())
	m_LongNeighborsList.Remove (addr);
    if (p->IsPredecessor ())
	m_PredecessorList.Remove (addr);
    if (p->IsReverseLongNeighbor ())
	m_ReverseLongNeighborsList.Remove (addr);

    UpdateSortedPeerList ();

    m_LinkMaintainer->OnPeerDeath (p);
}

void MemberHub::HandleLivenessPong (IPEndPoint *from, MsgLivenessPong *msg)
{
    ASSERT (from != NULL);

    SIDPeerMapIter it = m_PeersByAddress.find (*from);
    if (it == m_PeersByAddress.end ()) {
	DBG_DO { MWARN << " pong from a peer " << from << " I don't know? " << endl; }
	return;
    }

    ref<Peer> p = it->second;

    bool rangeChanged = false;
    bool nonContiguous = false;

    const NodeRange& newr = msg->GetRange ();
    const NodeRange& oldr = p->GetRange ();

    if (newr != oldr) 
	rangeChanged = true;
    if (newr.GetMin () != oldr.GetMin () && newr.GetMax () != oldr.GetMax ())
	nonContiguous = true;

    if (nonContiguous) 
	RemovePeer (*from);
    else {
	if (p->IsSuccessor () || p->IsLongNeighbor ())
	    p->HandleLivenessPong (msg);
    }

    if (rangeChanged || nonContiguous) 
	UpdateSortedPeerList ();
}

void MemberHub::HandleLivenessPing (IPEndPoint *from, MsgLivenessPing *ping)
{
    // 'pred's are somewhat special, they dont tell us before hand 
    // that they are pred's

    if (ping->IsSuccessorPing ()) {
	MDB (10) << " recvd predecessor ping from " << from << endl;	

	// our first predecessor is always installed through MsgNotify...
	// so if we have none => this is likely a spurious ping
	if (GetPredecessor () != NULL) 
	    AddPredecessor (*from, ping->GetRange ());
    }

    // otoh, reverse long neighbors are "already known" - hence
    // can be looked up in the hash table

    Peer *p = LookupPeer (*from);
    if (!p) {
	MTDB (-2) << " ping from unknown " << from << endl;
	return;
    }

    if (!p->IsReverseLongNeighbor () && !p->IsPredecessor ()) { 
	MTDB (-2) << " ping from dude who aint rev-lnbr or pred " << from << endl;
	return;
    }

    DB_DO (10) {
	MTDB (-2) << " received ping from " << *from; 
	if (ping->IsSuccessorPing ())
	    cerr << " rev-succ;";
	if (ping->IsLongNeighborPing ())
	    cerr << " rev-lnbr;";
	cerr << endl;
    }

    if (ping->IsSuccessorPing ())	
	p->RegisterIncomingPing (PEER_SUCCESSOR, m_Scheduler->TimeNow ());
    if (ping->IsLongNeighborPing ())
	p->RegisterIncomingPing (PEER_LONG_NBR, m_Scheduler->TimeNow ());

    NodeRange r = RANGE_NONE;
    if (m_Range)
	r = *m_Range;

    MsgLivenessPong *pong = new MsgLivenessPong (GetID (), m_Address, r, ping->GetSeqno ());
    m_Network->SendMessage (pong, from, Parameters::TransportProto);
    delete pong;
}

// This function is kind of expensive (list clearing and copying
// destroys 'Peer' objects and re-creates them); however, it should not
// be called more than twice a second or something. So, optimizing it
// may not be worthwhile. - Ashwin [01/24/2005]

struct less_peer_ptr {
    bool operator () (const Peer *a, const Peer *b) const {
	return a->GetRange ().GetMin () < b->GetRange ().GetMin ();
    }
};

/// This (having m_SortedPeers do a bunch of things)
//  is very dangerous and incorrect for two reasons: - Ashwin [03/21/2005]
//  
//  (a) GetMin () can be updated due to a LivenessPong () without 
//      making any changes to the map!
//
//  (b) Because SortedPeers is just a map (and not a multi-map), 
//      if two neighbors happen to have the same GetMin () (due 
//      to staleness), we will insert only one in SortedPeers. 
//      Only this guy will be sent heartbeats! Finally, as pongs
//      from the other guy come and if m_SortedPeers is updated, 
//      we will suddenly start sending pings to this guy -- but then
//      we will also notice that he hasn't sent us a pong from a 
//      long time and then kick him out!

// FIXES: (b) we must have a separate address based hash of peers. 
//            this should be used for looking up peers.
//
//        (a) handle liveness pong should call UpdateSortedPeerList ()
// <these fixes have been implemented>

void MemberHub::UpdateSortedPeerList()
{
    m_SortedPeers.clear ();

    // given that SortedPeers is a map, this will make sure peers 
    // with the same min-value (i.e., same peers) will not get 
    // inserted twice

    for (PeerListIter it = m_SuccessorList.begin (); it != m_SuccessorList.end (); ++it) {
	ref<Peer> p = *it;

	Value *v = (Value *) &(p->GetRange ().GetMin ());
	m_SortedPeers.insert (PeerMap::value_type (v, p));
    }

    for (PeerListIter it = m_LongNeighborsList.begin (); it != m_LongNeighborsList.end (); it++) {
	ref<Peer> p = *it;

	Value *v = (Value *) &(p->GetRange ().GetMin ());
	m_SortedPeers.insert (PeerMap::value_type (v, p));
    }

    for (PeerListIter it = m_PredecessorList.begin (); it != m_PredecessorList.end (); it++) {
	ref<Peer> p = *it;

	Value *v = (Value *) &(p->GetRange ().GetMin ());
	m_SortedPeers.insert (PeerMap::value_type (v, p));
    }
}

// returns all nodes in my peer list which will intersect
// the range 'cst'. for the linear-pub-routing purposes, 
// this routine also ends up returning the nodes in sorted
// order (since "stuff can't wrap around because we do this
// after reaching the left-most end") - Ashwin [03/07/2005]

void MemberHub::GetCoveringPeers (Constraint *cst, vector<Peer *> *nodes)
{
    // p1 -- p2 -- p3 -- ME -- cst-begin -- p4 -- p5 -- p6 -- cst-end -- p7 -- p8 
    nodes->clear ();
    if (cst->GetMin () == cst->GetMax ())
	return;

    MDB (20) << " -- [cst=" << cst << "]----------------- " << endl;
    for (PeerMapIter it = m_SortedPeers.begin (); it != m_SortedPeers.end (); ++it) {
	Peer *p = it->second;
	NodeRange *r = (NodeRange *) &p->GetRange ();

	MDB (20) << "considering " << p ; 
	if (IsBetweenInclusive (r->GetMin (), cst->GetMin (), cst->GetMax ()) ||
	    IsBetweenLeftInclusive (cst->GetMin (), r->GetMin (), r->GetMax ())) {
	    DB_DO (20) { cerr << " OK" << endl; }
	    nodes->push_back (p);
	}
	else {
	    DB_DO (20) { cerr << " REJECT" << endl; }
	}
    }
}

bool MemberHub::Less (const Value& val, const Value& other)
{
    if (val == GetAbsMax ()) 
	return val <= other;
    else
	return val < other;
}

// Preconditions: 'val' is not in my range
//                SortedPeerList contains at least 1 entry (successor must be there!)

extern bool loopingMessage;

Peer *MemberHub::GetNearestPeer(const Value &val)
{
    // (---- p1 --- p2 --- p3 --- val --- p4 --- p5 --- p6 ----)
    Peer *nearest = NULL;

    if (m_SortedPeers.size () == 0) 
	return NULL;

    MDB (10) << " <<< val=" << val << endl;

    //// XXX: this function is way more complex than it needs to 
    //// be. despite the fact that wrapped circle causes problems. - Ashwin [06/28/2005]

    for (PeerMapIter it = m_SortedPeers.begin () ; it != m_SortedPeers.end(); ++it)
    {
	const NodeRange& cur = it->second->GetRange ();
	MDB (10) << " <<<<<< considering peer=" << it->second << endl;
	if (loopingMessage) {
	    MDB (-5) << " <<<<<< considering peer=" << it->second << endl;
	}

	// GetMax () for a peer remains same even when other nodes join 
	// near that peer (since other nodes install themselves as 
	// predecessors) - so this way, information is less likely 
	// to be stale

	PeerMapIter nit (it);
	++nit;

	if (nit != m_SortedPeers.end ()) {
	    const NodeRange& next = nit->second->GetRange ();

	    Value comp = next.GetMax ();
	    if (next.GetMin () > next.GetMax ())
		comp = GetAbsMax ();

	    if (val >= cur.GetMax () && Less (val, comp)) {
		if (val < next.GetMin ())
		    nearest = it->second;
		else
		    nearest = nit->second;

		MDB (10) << " <<<<<<<<<<<<<<< found nearest " << nearest->GetRange () << endl;
		if (loopingMessage) {
		    MDB (-5) << " <<<<<<<<<<<<<<< found nearest A " << nearest << endl;
		}
		break;
	    }
	}
	else {
	    if (val >= cur.GetMax ())
		nearest = it->second;
	    else { 
		// this can only happen when val < first.GetMax ()

		const NodeRange& first = m_SortedPeers.begin ()->second->GetRange ();
		ASSERT (val < first.GetMax ());
		if (val >= first.GetMin ())  // belongs to first guy
		    nearest = m_SortedPeers.begin ()->second;
		else                         // between last and first
		    nearest = it->second;
	    }
	    MDB (10) << " <<<<<<<<<<<<<<< found nearest " << nearest->GetRange () << endl;
	    if (loopingMessage) {
		MDB (-5) << " <<<<<<<<<<<<<<< found nearest B " << nearest << endl;
	    }
	    break;
	}
    }

    ASSERT (nearest != NULL);
    return nearest;
}

void MemberHub::HandleMercPub(IPEndPoint *from, MsgPublication *msg )
{
    m_PubsubRouter->RouteData(from, msg);
}

void MemberHub::HandleMercSub( IPEndPoint *from, MsgSubscription *msg )
{
    m_PubsubRouter->RouteData(from, msg);
}

class TmpBootstrapHbeater : public Timer {
    MemberHub *m_Hub;

public:
    TmpBootstrapHbeater (MemberHub *hub) : Timer (0), m_Hub (hub) {}

    void OnTimeout () {
	m_NTimeouts++;
	if (m_NTimeouts > 3)
	    return;

	if (m_Hub->GetRange () == NULL)
	    return;

	MsgHeartBeat *hbeat = new MsgHeartBeat (m_Hub->GetID (), m_Hub->GetAddress (), *m_Hub->GetRange ());
	m_Hub->GetNetwork ()->SendMessage (hbeat, &m_Hub->GetBootstrapIP (), Parameters::TransportProto);
	delete hbeat;

	_RescheduleTimer (100);
    }
};

// Force refresh of all my peers so they don't timeout
void MemberHub::ForcePeerRefresh()
{
    for (PeerListIter it = m_SuccessorList.begin();
	 it != m_SuccessorList.end(); it++) {
	(*it)->RecvdMessage ();
    }
    for (PeerListIter it = m_LongNeighborsList.begin();
	 it != m_LongNeighborsList.end(); it++) {
	(*it)->RecvdMessage ();
    }

    TimeVal now = m_MercuryNode->GetScheduler ()->TimeNow ();
    for (PeerListIter it = m_PredecessorList.begin();
	 it != m_PredecessorList.end(); it++) {
	(*it)->RegisterIncomingPing(PEER_SUCCESSOR, now);
    }
    for (PeerListIter it = m_ReverseLongNeighborsList.begin();
	 it != m_ReverseLongNeighborsList.end(); it++) {
	(*it)->RegisterIncomingPing(PEER_SUCCESSOR, now);
    }
}

void MemberHub::StartBootstrapHbeater()
{
    // Jeff: I separated this part out from OnJoinComplete to faciliate
    // static joins. When we get the bootstrap response in static join
    // mode we HB the bootstrap so it knows about us, but don't "actually"
    // join until we get the all joined signal
    if (m_BootstrapHeartbeatTimer == NULL) {
	m_BootstrapHeartbeatTimer = new refcounted<TmpBootstrapHbeater> (this);
	m_Scheduler->RaiseEvent (m_BootstrapHeartbeatTimer, m_Address, 0);
    }
}

void MemberHub::OnJoinComplete()
{
    m_Status = ST_JOINED;

    m_HistogramMaintainer->Start();
    m_PubsubRouter->SetRangeChanged ();
    if (g_Preferences.do_loadbal) {
	MDB (-10) << " (re)starting load balancing timer " << endl;
	m_LoadBalancer->Start ();
    }

    m_SuccListTimer = new refcounted<SuccListMaintenanceTimer> (m_LinkMaintainer, Parameters::SuccessorMaintenanceTimeout);

    m_Scheduler->RaiseEvent (m_SuccListTimer, m_Address, 0);
    StartBootstrapHbeater();
    m_MercuryNode->GetApplication ()->JoinEnd (GetSuccessor ()->GetAddress ());
}

// LATERTODO: inform our neighbors we are leaving so they can take repair actions
// of course, if we dont do this now, it's fine - it will just take them
// a little bit longer to realize something's amiss.

// Right now we just use it to break all our neighbor links 
// clear up all the hash-tables and lists.

void MemberHub::PrepareLeave()
{
    MsgLinkBreak *lb = new MsgLinkBreak (GetID (), m_Address);

    Peer *succ = GetSuccessor ();
    Peer *pred = GetPredecessor ();

    for (SIDPeerMapIter it = m_PeersByAddress.begin (); it != m_PeersByAddress.end (); ++it)
    {
	ref<Peer>& p = it->second;

	// dont send to succ or pred, coz they get MsgLeaveNotification  
	// rename these messages, since LeaveNotification carries 
	// subscriptions and trigger state with itself.
	if (succ && p->GetAddress () == succ->GetAddress ())
	    continue;
	if (pred && p->GetAddress () == pred->GetAddress ())
	    continue;

	if (p->IsPredecessor () || p->IsReverseLongNeighbor ())
	    m_Network->SendMessage (lb, (IPEndPoint *) &p->GetAddress (), Parameters::TransportProto);
    }
    delete lb;

    m_SortedPeers.clear ();

    // this will clear the underlying hash-tables as well
    m_LongNeighborsList.Clear ();
    m_ReverseLongNeighborsList.Clear (); 
    m_SuccessorList.Clear ();
    m_PredecessorList.Clear ();

    m_HistogramMaintainer->Pause ();
    m_PubsubRouter->ClearData ();
    if (g_Preferences.do_loadbal) {
	m_LoadBalancer->Pause ();
    }
    m_SuccListTimer->Cancel ();

    m_Status = ST_UNJOINED;
    SetRange (NULL);
}


void MemberHub::StartJoin()
{
    m_Status = ST_UNJOINED;
    m_LinkMaintainer->Start();
}

/////////////////////////////////////////////////////////////////////
/// Sampling related functionality 

void MemberHub::GetSamples (Sampler *s, vector<Sample *> *ret)
{
    uint32 handle = hash<char *> () (s->GetName ());
    MetricInfo *minfo = GetRegisteredMetric (handle);
    if (!minfo) {
	MWARN << "Metric " << s->GetName () << " not REGISTERED!! " << endl;
	return;
    }

    minfo->ExpireSamples (m_Scheduler->TimeNow ());
    minfo->FillSamples (ret);
}

void MemberHub::RegisterMetric (Sampler *s)
{
    uint32 handle = hash<char *> () (s->GetName ());

    MDB (10) << " registering metric " << s->GetName () << endl;

    ref<RandomWalkTimer> rwt = new refcounted<RandomWalkTimer> (this, handle, s->GetRandomWalkInterval ());
    ref<LocalSamplingTimer> lst = new refcounted<LocalSamplingTimer> (this, handle, Parameters::LocalSamplingInterval);

    MetricInfo *minfo = new MetricInfo (this, handle, s, rwt, lst);
    m_SamplerRegistry.insert (SamplerMap::value_type (handle, minfo));

    m_Scheduler->RaiseEvent (rwt, m_Address, 0);
    m_Scheduler->RaiseEvent (lst, m_Address, 0);
}

void MemberHub::UnRegisterMetric (Sampler *s)
{
    uint32 handle = hash<char *> () (s->GetName ());
    SamplerMapIter it = m_SamplerRegistry.find (handle);

    MDB (10) << "  unregistering metric " << s->GetName () << endl;

    if (it == m_SamplerRegistry.end ())
	return;
    delete it->second;
    m_SamplerRegistry.erase (it);
}   

void MemberHub::RegisterLoadSampler (LoadSampler *s)
{
    if (!g_Preferences.distrib_sampling)
	return;

    if (m_LoadSampler) {
	UnRegisterMetric (m_LoadSampler);
	delete m_LoadSampler;
    }
    m_LoadSampler = s;
    RegisterMetric (m_LoadSampler);
}

void MemberHub::UnRegisterLoadSampler (LoadSampler *s)
{
    if (!g_Preferences.distrib_sampling)
	return;

    // XXX: pointer comparison!
    if (m_LoadSampler != s) 
	return;

    if (m_LoadSampler) {
	UnRegisterMetric (m_LoadSampler);
	delete m_LoadSampler;
    }
    if (g_Preferences.loadbal_routeload) 
	m_LoadSampler = new PubsubLoadSampler (this);
    else
	m_LoadSampler = new DummyLoadSampler (this);
    RegisterMetric (m_LoadSampler);
}

MetricInfo *MemberHub::GetRegisteredMetric (uint32 handle) 
{
    SamplerMapIter it = m_SamplerRegistry.find (handle);
    if (it == m_SamplerRegistry.end ()) {
	MWARN << "sampling metric not registered!" << endl;
	return NULL;
    }
    return it->second;
}

MetricInfo *MemberHub::GetRegisteredMetric (Sampler *sampler)
{
    uint32 handle = hash<char *> () (sampler->GetName ());
    return GetRegisteredMetric (handle);
}

void MemberHub::HandlePointEstimateResponse (IPEndPoint *from, MsgPointEstimateResponse *resp)
{
    if (m_Status != ST_JOINED)
	return;

    MDB (10) << "received point estimate response (dist=" << (int) resp->GetDistance () << ") from " << from << endl;
    MetricInfo *minfo = GetRegisteredMetric (resp->GetMetricHandle ());
    if (!minfo)
	return;

    // heard from the dude;
    Peer *p = LookupPeer (*from);
    if (p)
	p->RecvdMessage ();
    
    MDB (10) << "registering point estimate (" << resp->GetPointEstimate () << ")" << endl;
    minfo->AddPointEstimate ((int) resp->GetDistance (), resp->GetPointEstimate ());
}

void MemberHub::HandlePointEstimateRequest (IPEndPoint *from, MsgPointEstimateRequest *req)
{
    if (m_Status != ST_JOINED)
	return;

    MDB (10) << "received point estimate REQUEST (ttl=" << (int) req->GetTTL () << ") from " << from << endl;
    MetricInfo *minfo = GetRegisteredMetric (req->GetMetricHandle ());
    if (!minfo)
	return;

    // heard from the dude;
    Peer *p = LookupPeer (*from);
    if (p)
	p->RecvdMessage ();
    
    Sample* estimate = minfo->GetPointEstimate ();
    if (estimate == NULL)
	return;

    MDB (10) << "sending point estimate response with estimate=" << estimate << endl;
    MsgPointEstimateResponse *resp = new MsgPointEstimateResponse (GetID (), m_Address, req->GetMetricHandle (),
								   (req->GetDirection () == DIR_SUCC ? req->hopCount : -req->hopCount), estimate);	
    m_Network->SendMessage (resp, &req->GetCreator (), Parameters::TransportProto);
    delete resp;      
    delete estimate;

    req->DecrementTTL ();
    if (req->GetTTL () != 0) {
	// forward this to the next hop

	IPEndPoint *next_hop = NULL;
	if (req->GetDirection () == DIR_SUCC) 
	    next_hop = (IPEndPoint *) &GetSuccessor ()->GetAddress ();
	else 
	    next_hop = (GetPredecessor () != NULL) ? (IPEndPoint *) &GetPredecessor ()->GetAddress () : NULL;

	if (next_hop) {
	    MDB (10) << "forwarding point-estimate-request to " << next_hop << endl;
	    req->sender = m_Address;
	    m_Network->SendMessage (req, next_hop, Parameters::TransportProto);
	}
    }
}

void MemberHub::DoLocalSampling (uint32 handle)
{
    if (m_Status != ST_JOINED)
	return;

    MetricInfo *minfo = GetRegisteredMetric (handle);
    ASSERT (minfo != NULL);

    // this probably does not belong to "DoLocalSampling", but... :)
    MDB (10) << "LOCAL SAMPLING: expiring samples AT: " << m_Scheduler->TimeNow () << endl;
    minfo->ExpireSamples (m_Scheduler->TimeNow ());

    // add our own raw measurement estimate (distance=0)
    Sample *pe = minfo->GetPointEstimate ();
    minfo->AddPointEstimate (0, pe);
    delete pe;

    int radius = minfo->GetSampler ()->GetLocalRadius ();

    // request point estimates along the predecessor direction
    if (GetPredecessor () != NULL) {
	MDB (10) << "requesting point estimates on pred-direction" << endl;
	MsgPointEstimateRequest *req = new MsgPointEstimateRequest (GetID (), m_Address /* sender */, m_Address /* creator */, handle, DIR_PRED, (byte) radius);
	m_Network->SendMessage (req, (IPEndPoint *) &GetPredecessor ()->GetAddress (), Parameters::TransportProto);
	delete req;
    }

    // request point estimates along the successor direction

    ASSERT (GetSuccessor () != NULL);

    MDB (10) << "requesting point estimates on succ-direction" << endl;
    MsgPointEstimateRequest *req = new MsgPointEstimateRequest (GetID (), m_Address, m_Address /* sender */, handle, DIR_SUCC, (byte) radius);
    m_Network->SendMessage (req, (IPEndPoint *) &GetSuccessor ()->GetAddress (), Parameters::TransportProto);
    delete req;

    // the replies will arrive and get stored. whenever somebody 
    // requests a sample, we will combine these estimates. 
}

void MemberHub::HandleSampleResponse (IPEndPoint *from, MsgSampleResponse *resp)
{
    if (m_Status != ST_JOINED)
	return;

    MDB (10) << "received sample response. YAY!" << endl;
    MetricInfo *minfo = GetRegisteredMetric (resp->GetMetricHandle ());
    if (!minfo)
	return;

    MDB (10) << "response contains: " << resp->size () << " entries " << endl;

    // Some random guy delivers his own local sample 
    // + the most recent few samples he has received
    for (list<Sample *>::iterator it = resp->s_begin (); it != resp->s_end (); ++it) {
	minfo->AddSample (*it, m_Scheduler->TimeNow ());
    }

    // we heard from this dude!
    Peer *p = LookupPeer (*from);
    if (p)
	p->RecvdMessage ();
}

void MemberHub::HandleSampleRequest (IPEndPoint *from, MsgSampleRequest *req)
{
    if (m_Status != ST_JOINED)
	return;

    MDB (10) << "received sample request from " << req->GetCreator () << endl;
    TimeVal& now = m_Scheduler->TimeNow ();

    // this acts like a ping; send a pong
    {
	NodeRange r = RANGE_NONE;
	if (m_Range)
	    r = *m_Range;

	Peer *p = LookupPeer (*from);
	if (p && p->IsReverseLongNeighbor ())  {
	    p->RegisterIncomingPing (PEER_LONG_NBR, m_Scheduler->TimeNow ());

	    if ((sint64)(now - p->GetLastPingReceived (PEER_LONG_NBR)) >= (sint64) Parameters::PeerPingInterval) {
		MsgLivenessPong *pong = new MsgLivenessPong (GetID (), m_Address, r, req->GetSeqno ());
		m_Network->SendMessage (pong, from, Parameters::TransportProto);
		delete pong;		
	    }
	}
    }

    MetricInfo *minfo = GetRegisteredMetric (req->GetMetricHandle ());
    if (!minfo)
	return;

    req->DecrementTTL ();
    if (req->GetTTL () != 0) {
	Peer *randnbr = GetRandomLongNeighbor ();
	if (randnbr == NULL)
	    return;

	// this acts as a ping to the rand-nbr
	req->SetSeqno (randnbr->GetNextSeqno ());
	randnbr->RegisterSentPing (m_Scheduler->TimeNow (), req->GetSeqno ());

	MDB (10) << "forwarding to " << randnbr->GetAddress () << endl;
	req->sender = m_Address;
	m_Network->SendMessage (req, (IPEndPoint *) &randnbr->GetAddress (), Parameters::TransportProto);
	return;
    }

    MDB (10) << "ttl expired. sending response" << endl;
    MsgSampleResponse *resp = minfo->MakeSampleResponse ();
    if (!resp)
	return;
    m_Network->SendMessage (resp, &req->GetCreator (), Parameters::TransportProto);
    delete resp;
}

Peer* MemberHub::GetRandomLongNeighbor ()
{
    if (m_LongNeighborsList.Size () <= 0)
	return NULL;

    // choose a random long neighbor to send the message to.
    int index = (int) (drand48 () * m_LongNeighborsList.Size ());
    Peer *p = NULL;
    for (PeerListIter it = m_LongNeighborsList.begin (); it != m_LongNeighborsList.end (); ++it) {
	if (index-- == 0) { 
	    p = static_cast<Peer *> (*it);
	    break;
	}
    }
    ASSERT (p != NULL);
    return p;
}

double _log (float f) {
    return log ((double) f);
}

void MemberHub::StartRandomWalk (uint32 handle)
{
    int nodecount = m_HistogramMaintainer->EstimateNodeCount ();

    if (nodecount <= 0) {
	MTDB (20) << " node count=0 " << endl;
	return;
    }

    Peer *randnbr = GetRandomLongNeighbor ();

    if (randnbr == NULL) {
	MTDB (20) << "random neighbor=NULL **************** " << endl;
	return;
    }

    // the seqno stuff is needed coz this message
    // also serves as a ping to this neighbor and
    // suppresses the normal liveness check
    MsgSampleRequest *req = new MsgSampleRequest (GetID (), m_Address, m_Address /* creator */, 
						  handle, 
						  (byte) (_log (nodecount)/_log (2)) + 1, 
						  randnbr->GetNextSeqno ()
	);
    randnbr->RegisterSentPing (m_Scheduler->TimeNow (), req->GetSeqno ());

    MDB (10) << "sending sample request with ttl=" << (int) req->GetTTL () << endl;
    m_Network->SendMessage (req, (IPEndPoint *) &randnbr->GetAddress (), Parameters::TransportProto);
    delete req;	
}

// just utility...
Sample *MemberHub::MakeLocalSample (Sampler *sampler)
{
    uint32 handle = hash<char *> () (sampler->GetName ());
    MetricInfo *minfo = GetRegisteredMetric (handle);

    ASSERT (minfo != NULL);

    return minfo->GetPointEstimate ();
}

struct less_range_t {
    bool operator () (const Sample* as, const Sample* bs) const {
	const Value& av = as->GetRange ().GetMin ();
	const Value& bv = bs->GetRange ().GetMin ();

	if (av == bv) 
	    return as->GetRange ().GetMax () < bs->GetRange ().GetMax ();
	else
	    return av < bv;
    }
};

struct less_value_t {
    bool operator () (const Value& av, const Value& bv) const {
	return av < bv;
    }
};

#define RANGE(sample) (sample)->GetRange()
#define LO(sample) (sample)->GetRange().GetMin()
#define HI(sample) (sample)->GetRange().GetMax()
#define AMIN GetAbsMin()
#define AMAX GetAbsMax()

// NOTE: these histogram operations could be sort of heavy-weight
// especially because they result in a lot of 'Value' construction
// and destruction. but because this wouldn't be called too often
// (as long as the re-construction interval is >= a few 100 millis)
// everything should be fine - Ashwin [03/17/2005]

Value MemberHub::GetWeightedMidPoint (const NodeRange& prev, const NodeRange& next)
{
    Value lspan = prev.GetSpan (AMIN, AMAX);
    Value rspan = next.GetSpan (AMIN, AMAX);

    NodeRange intermed (prev.GetAttrIndex (), prev.GetMax (), next.GetMin ());
    Value mspan = intermed.GetSpan (AMIN, AMAX);

    // accomplish mspan = (lspan / (lspan + rspan)) * mspan;
    Value total = lspan;
    total += rspan;
    ASSERT (total.getui () > 0);

    mspan *= lspan;
    mspan /= total;

    // add mspan to prev.GetMax () making sure where to wrap
    Value ret = prev.GetMax ();
    ret += mspan;

    /*
      if (ret > GetAbsMax ())
      ret -= GetAbsMax ();
    */
    return ret;
}

float MemberHub::GetNumNodeEstimate (const NodeRange& range, const NCMetric *ncm)
{
    Value span = range.GetSpan (AMIN, AMAX);
    return (float) span.double_div (ncm->GetRangeSpan ());
}

#define METRIC(sample)   (dynamic_cast<const NCMetric*> ((sample)->GetMetric ()))

void MemberHub::AddBucket (Histogram *h, const NodeRange& range, Sample *sample)
{
    ASSERT (range.GetMin () >= AMIN);
    ASSERT (range.GetMax () <= AMAX);

    const NCMetric *met = METRIC(sample);
    Value metspan = met->GetRangeSpan ();
    if (metspan.getui () == 0)
	return;

    h->AddBucket (range, GetNumNodeEstimate (range, met));
}

#if 0
#define ADDBUCKET(range, sample) do { \
    ASSERT (range.GetMin () >= AMIN); \
	ASSERT (range.GetMax () <= AMAX); \
	h->AddBucket (range, GetNumNodeEstimate (range, METRIC (sample))); } while (0)
#endif

// utility debug routine to call from gdb
void print_samples (vector<Sample *>& samples)
{
    for (vector<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it)
    {
	Sample *s = *it;
	cerr << s << endl;
    }
}

void MemberHub::MakeSamplesDisjoint (PSVec& samples)
{
    PSVecIter it, nit, oit;
    Sample *cur, *next;
    NodeRange t = RANGE_NONE;

    it = samples.begin ();    
    while (it != samples.end ()) {
	nit = it;
	++nit;

	if (nit == samples.end ()) 
	    break;

	cur = *it;
	next = *nit;

	if (LO(cur) == LO(next)) {
	    delete cur;
	    it = samples.erase (it);
	    // it = nit;            // THIS FUCKING WONT WORK FOR VECTORS. stl STUMPED ME. - Ashwin

	    continue;
	}

	if (RANGE(cur).Overlaps (RANGE(next))) {
	    ASSERT (LO(cur) != LO(next));
	    t = NodeRange (GetID (), LO(cur), LO(next));
	    cur->SetRange (t);
	}
	++it;
    }
}

Histogram *MemberHub::MakeHistogramFromSamples (vector<Sample *>& sam)
{
    Histogram *h = new Histogram ();	
    NodeRange t = RANGE_NONE;
    int n = sam.size ();

    ASSERT (n > 0);

    if (n == 1) {
	t = NodeRange (GetID (), AMIN, AMAX);
	AddBucket (h, t, sam[0]);
	MDB (15) << "constructed histogram=" << endl << h << endl;
	return h;
    }

    PSVec samples;
    Sample *ts = NULL;

    n = sam.size ();     // get size () again

#if 0
    DB_DO(-15) {
	cerr << "<<<<< samples beforehand" << endl;
	for (int i = 0; i < n; i++) {
	    cerr << "\t[" << i << "]" << sam[i] << endl;
	}
    }
#endif

    // dupe samples; if wrapped, split them and finish the pain! 

    for (vector<Sample *>::iterator it = sam.begin (); it != sam.end (); ++it)
    {
	Sample *s = *it;
	if (LO(s) <= HI(s)) 
	    samples.push_back (s->Clone ());
	else {
	    ts = s->Clone ();
	    t = NodeRange (GetID (), AMIN, HI(s));
	    ts->SetRange (t);

	    samples.push_back (ts);

	    ts = s->Clone ();
	    t = NodeRange (GetID (), LO(s), AMAX);
	    ts->SetRange (t);

	    samples.push_back (ts);
	}
    }

    sort (samples.begin (), samples.end (), less_range_t ());

    n = samples.size ();     // get size () again

#if 0
    DB_DO(-15) {
	cerr << "<<<<< samples before disjointing... " << endl;
	for (int i = 0; i < n; i++) {
	    cerr << "\t[" << i << "]" << samples[i] << endl;
	}
    }
#endif
    // make them disjoint, since they can report 
    // conflicting information due to load balancing 
    // and staleness.
    MakeSamplesDisjoint (samples);

    n = samples.size ();     // get size () again

#if 0
    DB_DO(-15) {
	cerr << "<<<<< samples " << endl;
	for (int i = 0; i < n; i++) {
	    cerr << "\t[" << i << "]" << samples[i] << endl;
	}
    }
#endif

    vector<Value> midpoints;
    for (int i = 0; i < n - 1; i++) {
	Value v = GetWeightedMidPoint (RANGE(samples[i]), RANGE(samples[i + 1]));

	ASSERT (v >= AMIN);
	ASSERT (v <= AMAX);

	MDB (15) << "midpoint[" << i << "] of {" << RANGE(samples[i]) << "} and {" << 
	    RANGE(samples[i + 1]) << "} = " << v << endl;

	midpoints.push_back (v);
    }

    // this takes care of samples[1] to samples[n - 2]
    for (int i = 0; i < n - 2; i++) {
	t = NodeRange (GetID (), midpoints[i], midpoints[i + 1]);
	AddBucket (h, t, samples[i + 1]);
    }

    // take care of samples[0] and samples[n - 1] now 
    Value v = GetWeightedMidPoint (RANGE(samples[n - 1]), RANGE(samples[0]));
    Value wn = GetWeightedMidPoint (RANGE(samples[n - 2]), RANGE(samples[n - 1]));
    Value w1 = GetWeightedMidPoint (RANGE(samples[0]), RANGE(samples[1]));

    if (v > AMAX) {
	v -= AMAX;
	v += AMIN;

	t = NodeRange (GetID (), AMIN, v);
	AddBucket (h, t, samples[n - 1]);

	t = NodeRange (GetID (), wn, AMAX);
	AddBucket (h, t, samples[n - 1]);

	t = NodeRange (GetID (), v, w1);
	AddBucket (h, t, samples[0]);
    }
    else {
	t = NodeRange (GetID (), wn, v);
	AddBucket (h, t, samples[n - 1]);

	t = NodeRange (GetID (), v, AMAX);
	AddBucket (h, t, samples[0]);

	t = NodeRange (GetID (), AMIN, w1);
	AddBucket (h, t, samples[0]);
    }

    h->SortBuckets ();
    MDB (15) << "constructed histogram=" << endl << h << endl;

    for (PSVecIter it = samples.begin (); it != samples.end (); ++it)
	delete *it;
    samples.clear ();

    return h;
}

// histograms are not exchanged; this is just local
// so the size can be sort of large! ~40-50 bytes per
// bucket...
#define NC_HISTOGRAM_BUCKETS  1000 

Histogram* MemberHub::InitializeHistogram (int nbkts, int attr, Value& absmin, Value& absmax)
{
    Histogram *hist = new Histogram ();

    NodeRange tmp (attr, absmin, absmax);

    Value  span = tmp.GetSpan (absmin, absmax);
    Value  incr = span;
    incr /= nbkts;
    Value  start, end;

    start = absmin;

    // initialize the buckets with the proper range values;
    for (int i = 0; i < nbkts; i++) {
	if (i == nbkts - 1) 
	    end = absmax;
	else {
	    end = start;
	    end += incr;
	}

	NodeRange *r = new NodeRange (attr, start, end);
	hist->AddBucket (*r, 0);
	delete r;

	start = end;
    }

    return hist;
}

void MemberHub::MakeHistogram ()
{
    if (m_Status != ST_JOINED || m_Range == NULL)
	return;

    vector<Sample *> ncsamples;
    GetSamples (m_NCSampler, &ncsamples);

    Sample *local = MakeLocalSample (m_NCSampler);	
    if (local != NULL)
	ncsamples.push_back (local);

    if (m_NCHistogram)
	delete m_NCHistogram;
    m_NCHistogram = MakeHistogramFromSamples (ncsamples);

    float count = 0;
    for (int i = 0, len = m_NCHistogram->GetNumBuckets(); i < len; i++) {
	count += m_NCHistogram->GetBucket(i)->GetValue ();
    }

    MDB (15) << ">> count of nodes = " << (int) (count + 0.5) << endl;

    if (g_Preferences.self_histos)
	m_HistogramMaintainer->SetHistogram (m_NCHistogram);

    delete local;
}

ostream& operator<<(ostream& out, MemberHub *hub)
{
    // call the super class...
    out << (Hub *) hub;

    out << "range=";
    if (hub->m_Range)
	out << hub->m_Range;
    else
	out << "null";
    out << " pred=";

    if (hub->GetPredecessor ()) 
	out << hub->GetPredecessor ();
    else 
	out << "(null)";

    out << "[34m";
    hub->PrintSuccessorList ();
    out << "[32m";
    hub->PrintLongNeighborList ();
    out << "[m";

    return out;
}

void MemberHub::Print(FILE * stream)
{
    Hub::Print(stream);

    fprintf(stream, " status=%s range=", g_ProtoStatusStrings[m_Status]);
    if (m_Range)
	m_Range->Print(stream);
    else
	fprintf(stream, "null");
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
