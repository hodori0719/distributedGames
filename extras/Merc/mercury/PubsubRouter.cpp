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

//#define BENCHMARK_REQUIRED
#include <mercury/NetworkLayer.h>
#include <mercury/Hub.h>
#include <mercury/Cache.h>
#include <mercury/PubsubRouter.h>
#include <mercury/Interest.h>
#include <mercury/Event.h>
#include <mercury/Message.h>
#include <mercury/Parameters.h>
#include <mercury/LinkMaintainer.h>
#include <mercury/ObjectLogs.h>      // XXX
#include <mercury/RoutingLogs.h>
#include <mercury/Application.h>
#include <mercury/LoadBalancer.h>
#include <mercury/Scheduler.h>
#include <mercury/MercuryNode.h>
#include <util/Benchmark.h>
#include <fstream>
#include <mercury/BufferManager.h>
#include <util/stacktrace.h>
// #include <om/OMInterest.h>      // FOR DEBUG ONLY

const uint32 NUM_LOAD_WINDOWS = 30;      // each window is worth `Parameters::LoadAggregationInterval' milliseconds

// A default, very simple publication, subscription
// store.

class MercPubsubStore : public PubsubStore {
public:
    typedef list<MsgPublication *> PubMsgLst;
    typedef PubMsgLst::iterator    PubMsgLstIter;

    typedef list<Interest *> IntLst;
    typedef IntLst::iterator IntLstIter;
private:
    PubMsgLst            m_TriggerList;
    IntLst               m_SubList;

public:
    MercPubsubStore () {}
    virtual ~MercPubsubStore () {}

    void StoreTrigger (MsgPublication *pmsg) { m_TriggerList.push_back (pmsg->Clone ()); }
    void StoreSub (Interest *in) { m_SubList.push_back (in->Clone ()); }

    void DeleteTriggers (callback<bool, MsgPublication *>::ref delpred) {
	for (PubMsgLstIter it = m_TriggerList.begin (); it != m_TriggerList.end (); /* ++it */) {
	    if (delpred (*it)) {
		delete *it;
		it = m_TriggerList.erase (it);
	    }
	    else {
		++it;
	    }
	}
    }
    void DeleteSubs (callback<bool, Interest *>::ref delpred) {
	for (IntLstIter it = m_SubList.begin (); it != m_SubList.end (); /* ++it */)
	{
	    if (delpred (*it)) {
		delete *it;
		it = m_SubList.erase (it);
	    }
	    else {
		++it;
	    }
	}			
    }

    void GetOverlapSubs (MsgPublication *pmsg, list<Interest *> *pmatch) {
	Event *pub = pmsg->GetEvent ();
	for (IntLstIter it = m_SubList.begin (); it != m_SubList.end (); ++it) {
	    if ((*it)->Overlaps (pub))
		pmatch->push_back (*it);
	}
    }

    void GetOverlapTriggers (Interest *in, list<MsgPublication *> *pmatch) {
	for (PubMsgLstIter it = m_TriggerList.begin (); it != m_TriggerList.end (); ++it) {
	    Event *pub = (*it)->GetEvent ();
	    if (in->Overlaps (pub))
		pmatch->push_back (*it);
	}		
    }

    void Clear () {
	DeleteSubs (wrap (this, &MercPubsubStore::delsub_pred));
	DeleteTriggers (wrap (this, &MercPubsubStore::deltrigger_pred));
    }
private:
    bool delsub_pred (Interest *i) { return true; }
    bool deltrigger_pred (MsgPublication *pmsg) { return true; }
};

extern ofstream g_MercEventsLog;
static void DoMeasurementLog(Message *msg, MemberHub *hub, IPEndPoint *from);

class CountResetter : public Timer {
    PubsubRouter *m_PubsubRouter;
public:
    CountResetter (PubsubRouter *pr) : Timer (0), m_PubsubRouter (pr) {}

    void OnTimeout () { 
	m_PubsubRouter->NewLoadWindow ();
	_RescheduleTimer (Parameters::LoadAggregationInterval);
    }
};

namespace PS_RTR {
    static const int EXPIRY_TIMEOUT = 1000;

    class ExpiryTimer : public Timer {
	PubsubStore *store;
	Scheduler *sched;
    public:
	ExpiryTimer (Scheduler *sched, PubsubStore *store) : Timer (0), sched (sched), store (store)
	    {}

	bool expiretrigger_predicate (MsgPublication *pmsg) {
	    Event *ev = pmsg->GetEvent();
	    return (ev->GetDeathTime() < sched->TimeNow ());
	}

	bool expiresub_predicate (Interest *i) {
	    return (i->GetDeathTime () < sched->TimeNow ());
	}

	void OnTimeout () {
	    store->DeleteTriggers (wrap (this, &ExpiryTimer::expiretrigger_predicate));
	    store->DeleteSubs (wrap (this, &ExpiryTimer::expiresub_predicate));

	    _RescheduleTimer (EXPIRY_TIMEOUT);
	}
    };
};		

PubsubRouter::PubsubRouter(MemberHub *hub, BufferManager *bm, LinkMaintainer *lm) 
    : m_Hub(hub), m_BufferManager(bm), m_LinkMaintainer(lm), m_RoutedPubs (0), 
      m_RoutedSubs (0), m_RoutingLoad (0), m_LastHop (SID_NONE),
      m_StopRangeChangeTimer (NULL), m_WindowIndexAtChange (0), m_RangeRatioAtChange (1.0)
{
    m_MercuryNode = m_Hub->GetMercuryNode ();
    m_Network = m_Hub->GetNetwork();
    m_Scheduler = m_Hub->GetScheduler ();
    m_Address = m_Hub->GetAddress ();
    m_Cache   = m_Hub->GetCache();

    m_Store = m_MercuryNode->GetApplication ()->GetPubsubStore (m_Hub->GetID ());
    if (!m_Store)
	m_Store = new MercPubsubStore ();

    MsgType msgs[] = { MSG_PUB, MSG_SUB, MSG_LINEAR_PUB, MSG_LINEAR_SUB, MSG_SUB_LIST, MSG_TRIG_LIST };

    for (uint32 i = 0; i < sizeof(msgs) / sizeof(MsgType); i++)
	m_MercuryNode->RegisterMessageHandler(msgs[i], this);

    if (g_Preferences.loadbal_routeload)
	m_Scheduler->RaiseEvent (new refcounted<CountResetter> (this), m_Address, Parameters::LoadAggregationInterval);
    m_Scheduler->RaiseEvent (new refcounted<PS_RTR::ExpiryTimer> (m_Scheduler, m_Store), m_Address, PS_RTR::EXPIRY_TIMEOUT);
}

PubsubRouter::~PubsubRouter() {
    delete m_Store;
}

void PubsubRouter::ClearData () 
{
    m_Store->Clear ();
    m_LoadWindows.clear ();
}

class StopRangeChange : public Timer {
    PubsubRouter *m_PR;
public:
    StopRangeChange (PubsubRouter *pr) : Timer (0), m_PR (pr) {} 
    void OnTimeout () {
	m_PR->m_RangeChanged = false;
    }
};

// This method has a strange name, coz I could not up with sth better
// Handle a range change; so we scale our load appropriately
void PubsubRouter::UpdateRangeLoad (const NodeRange& newrange)
{
    NodeRange *currange = m_Hub->GetRange ();
    if (currange == NULL)
	return;
    Value curspan = m_Hub->GetRangeSpan ();
    if (curspan.getui () == 0)
	return;

    Value newspan = newrange.GetSpan (m_Hub->GetAbsMin (), m_Hub->GetAbsMax ());
    double ratio = newspan.double_div (curspan);

    if (ratio > 1.0e-3 && ratio <= 1.00)  {
	m_WindowIndexAtChange = NUM_LOAD_WINDOWS;	
	INFO << " range changed by a factor=" << merc_va ("%.3f", ratio) << endl;
	m_RangeRatioAtChange *= ratio;	
    }
}

void PubsubRouter::SetRangeChanged ()
{
    m_RangeChanged = true;

    // send range-changed "fast pongs" for 1 second
    if (m_StopRangeChangeTimer != NULL)
	m_StopRangeChangeTimer->Cancel ();

    m_StopRangeChangeTimer = new refcounted<StopRangeChange> (this);
    m_Scheduler->RaiseEvent (m_StopRangeChangeTimer, m_Address, 1000);
}

PubsubLoadSampler::PubsubLoadSampler (MemberHub *hub) : LoadSampler (hub), m_PSRouter (hub->GetPubsubRouter ())
{
}

Metric *PubsubLoadSampler::GetPointEstimate () {
    return new LoadMetric (m_PSRouter->GetRoutingLoad ());
}

void PubsubRouter::NewLoadWindow ()
{
    // keep the window-size bounded
    if (m_LoadWindows.size () > NUM_LOAD_WINDOWS) 
	m_LoadWindows.pop_front ();

    // measure load for the current bucket
    m_LoadWindows.push_back (m_RoutedPubs + m_RoutedSubs);
    m_RoutedSubs = 0;
    m_RoutedPubs = 0;

    // compute aggregate load for the window
    // if there was a range change in the recent
    // past, any measurement before that range 
    // change should be weighted by the range change
    // ratio. 
    //
    // i.e., if we were heavy and our new range is
    // 0.5 times the original, we should report our 
    // current load as 'half' times the measured 
    // load. (assuming that the range-change would 
    // change our load in the near future.) of course,
    // once NUM_LOAD_WINDOWS measurement intervals pass,
    // we discard the weighting factor since we have 
    // enough history to report a "sane" measurement.

    m_RoutingLoad = 0.0;
    int index = 0;
    for (list<float>::iterator it = m_LoadWindows.begin (); it != m_LoadWindows.end (); ++it) {
	if (index < m_WindowIndexAtChange) 
	    m_RoutingLoad += (*it) * m_RangeRatioAtChange;
	else
	    m_RoutingLoad += (*it);
	index++;
    }

    // log 
    double avg = m_Hub->GetLoadBalancer ()->GetAverageLoad ();
    if (avg < LoadBalancer::EPSILON)
	avg = 0.0;
    LoadBalEntry ent (LoadBalEntry::LOADREP, (float) m_RoutingLoad, (float) avg);
    LOG(LoadBalanceLog, ent);

    if (m_WindowIndexAtChange > 0) {
	m_WindowIndexAtChange--;
	if (m_WindowIndexAtChange == 0) 
	    m_RangeRatioAtChange = 1.0;
    }
}

void PubsubRouter::SendQuickPong (IPEndPoint *from)
{
    if (from && m_RangeChanged) {
	MsgLivenessPong *pong = new MsgLivenessPong (m_Hub->GetID (), m_Address, *m_Hub->GetRange (), 0xff);
	m_Network->SendMessage (pong, from, Parameters::TransportProto);
	delete pong;
    }
}

void PubsubRouter::ProcessMessage(IPEndPoint *from, Message *msg)
{
    if (msg->hubID != m_Hub->GetID())
	return;

    if (m_Hub->GetStatus () != ST_JOINED)
	return;

    // heard from a dude...
    Peer *p = m_Hub->LookupPeer (*from);
    if (p) 
	p->RecvdMessage ();

    SendQuickPong (from);

    START(PubsubRouter::ProcessMessage);

    // there is no application registered for this message
    if (from)
	m_LastHop = *from;
    else
	m_LastHop = m_Address;

    MsgType t = msg->GetType();

#ifdef RECORD_ROUTE
    if (from) {
	Neighbor n (m_Address, *m_Hub->GetRange (), 0);
	if (t == MSG_SUB)
	    ((MsgSubscription *) msg)->routeTaken.push_back (n);
	else if (t == MSG_PUB)
	    ((MsgPublication *) msg)->routeTaken.push_back (n);
    }
#endif

    if (t == MSG_PUB) {
	Event *evt = ((MsgPublication *) msg)->GetEvent();

	ASSERT (!evt->IsMatched ());

	m_RoutedPubs++;
	RouteData(from, msg);
    }
    else if (t == MSG_SUB) { 
	m_RoutedSubs++;
	RouteData(from, msg);
    }
    else if (t == MSG_LINEAR_PUB) {
	m_RoutedPubs++;
	START(HandleLinearPublication);
	HandleLinearPublication (from, (MsgLinearPublication *) msg);
	STOP(HandleLinearPublication);
    }
    else if (t == MSG_LINEAR_SUB) {
	m_RoutedSubs++;
	START(HandleLinearSubscription);
	HandleLinearSubscription (from, (MsgLinearSubscription *) msg);
	STOP(HandleLinearSubscription);
    }
    else if (t == MSG_SUB_LIST) {
	START(HandleSubscriptionList);
	HandleSubscriptionList (from, (MsgSubscriptionList *) msg);
	STOP(HandleSubscriptionList);
    }
    else if (t == MSG_TRIG_LIST) {
	START(HandleTriggerList);
	HandleTriggerList (from, (MsgTriggerList *) msg);
	STOP(HandleTriggerList);
    }
    else {
	WARN << merc_va("PubsubRouter:: received some idiotic message [%s]", msg->TypeString()) << endl;
    }
    STOP(PubsubRouter::ProcessMessage);
}

void PubsubRouter::SendAck(MsgPublication * pmsg)
{
    MsgAck *amsg = new MsgAck(pmsg->hubID, m_Address, *m_Hub->GetRange());
    m_Network->SendMessage(amsg, &pmsg->GetCreator(), Parameters::TransportProto);
    delete amsg;
}

#if 0 
// MercuryNode takes care of this now - HACK!
// introduce a new MsgMatchedPublication class actually ...

void PubsubRouter::HandleMercMatchedPub(IPEndPoint * from,  MsgPublication * pmsg)
{
    Event *ev = pmsg->GetEvent()->Clone();

    ///// MEASUREMENT
    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	DiscoveryLatEntry ent(DiscoveryLatEntry::PUB_RECV, 0, ev->GetNonce());
	LOG(DiscoveryLatLog, ent, pmsg->recvTime); // use recv time
    }
    ///// MEASUREMENT

    DBG_DO { g_MercEventsLog << "received matchedpub " << ev << endl; }
    m_BufferManager->EnqueueNetworkEvent(ev);
}
#endif

// XXX this is really a hack!   
#define VALUE_EPSILON 1 

#define SUB_TO_IN(msg)       (((MsgSubscription *) msg)->GetInterest ())
#define PUB_TO_EV(msg)       (((MsgPublication *) msg)->GetEvent ())

bool PubsubRouter::CheckAppLinear (Message *msg)
{
    MsgType t = msg->GetType ();
    int proceed = 1;

    Application *app = m_MercuryNode->GetApplication ();

    if (t == MSG_SUB || t == MSG_LINEAR_SUB)
	proceed = app->InterestLinear (SUB_TO_IN (msg), m_LastHop);
    else if (t == MSG_PUB || t == MSG_LINEAR_PUB)
	proceed = app->EventLinear (PUB_TO_EV (msg), m_LastHop);
    else
	ASSERT (false);

    return (proceed > 0);
}

bool PubsubRouter::CheckAppRoute (Message *msg)
{
    MsgType t = msg->GetType ();
    int proceed = 1;

    Application *app = m_MercuryNode->GetApplication ();

    if (t == MSG_SUB)
	proceed = app->InterestRoute (SUB_TO_IN (msg), m_LastHop);
    else if (t == MSG_PUB)
	proceed = app->EventRoute (PUB_TO_EV (msg), m_LastHop);
    else
	ASSERT (false);

    return (proceed > 0);
}

static void _SetStopVal (Message *msg, const Value &v)
{
    if (msg->GetType () == MSG_LINEAR_SUB)
	((MsgLinearSubscription *) msg)->SetStopVal (v);
    else
	((MsgLinearPublication *) msg)->SetStopVal (v);
}

void PubsubRouter::DoFanout (Constraint *newcst, Message *msg)
{
    if (!CheckAppLinear (msg))
	return;

    vector<Peer *> eligible;
    m_Hub->GetCoveringPeers (newcst, &eligible);

    for (int i = 0, len = eligible.size (); i < len; i++) {
	if (i == len - 1) 
	    _SetStopVal (msg, newcst->GetMax ());
	else {
	    Value v (eligible[i + 1]->GetRange ().GetMin ());
	    v -= VALUE_EPSILON;
	    _SetStopVal (msg, v);
	}

	m_Network->SendMessage (msg, (IPEndPoint *) &eligible[i]->GetAddress (), Parameters::TransportProto);
    }
}

void PubsubRouter::HandleLinearPublication (IPEndPoint *from, MsgLinearPublication *lpmsg)
{
    Constraint *cst = (Constraint *) lpmsg->GetEvent()->GetConstraintByAttr (m_Hub->GetID());
    Constraint *newcst = new Constraint (cst->GetAttrIndex (), m_Hub->GetRange ()->GetMax (), lpmsg->GetStopVal ()); 

    bool left, center, right;
    newcst->GetRouteDirections (*m_Hub->GetRange (), left, center, right, m_Hub->AmRightMost ());

    if (m_Hub->GetRange ()->GetMin () > m_Hub->GetRange ()->GetMax ())
	right = false;

    // slice it; we dont want the whole linear pub hanging 
    // around in the trigger-set etc.

    MsgPublication p = * (MsgPublication *) lpmsg; 
    HandlePubAtRendezvous (from, &p, left, right);

    if (right) {
	DoFanout (newcst, lpmsg);
    }
}

void PubsubRouter::HandleLinearSubscription (IPEndPoint *from, MsgLinearSubscription *lsmsg)
{
    Constraint *cst = (Constraint *) lsmsg->GetInterest()->GetConstraintByAttr (m_Hub->GetID());
    Constraint *newcst = new Constraint (cst->GetAttrIndex (), m_Hub->GetRange ()->GetMax (), lsmsg->GetStopVal ()); 

    bool left, center, right;
    newcst->GetRouteDirections (*m_Hub->GetRange (), left, center, right, m_Hub->AmRightMost ());

    if (m_Hub->GetRange ()->GetMin () > m_Hub->GetRange ()->GetMax ())
	right = false;

    // slice it; we dont want the whole linear sub hanging 
    // around in the sub-list etc.

    MsgSubscription s = * (MsgSubscription *) lsmsg; 

    HandleSubAtRendezvous (from, &s);

    if (right) {
	DoFanout (newcst, lsmsg);
    }
}

static void PrintRoute (list<Neighbor>& routeTaken)
{
    cerr << "{";
    for (list<Neighbor>::iterator it = routeTaken.begin(); it != routeTaken.end(); ++it) {
	Neighbor *n = &(*it);
	if (it != routeTaken.begin ()) 
	    cerr << " -> ";
	cerr << n->addr.GetPort () << ":(" << n->range.GetMin () << "," << n->range.GetMax () << ")";
    }
    cerr << "}" << endl;
}

// The publication has reached the rendezvous point. we are going to match it.
// the 2 boolean arguments tell the results of the CalculateCoverage() call for 
// range pubs. for normal pubs, left = right = false.

void PubsubRouter::HandlePubAtRendezvous(IPEndPoint *from, MsgPublication *pmsg, 
					 bool pubMatchesLeft, bool pubMatchesRight)
{
    Event *evt = pmsg->GetEvent();
    DBG_DO { g_MercEventsLog << "==hub== " << m_Hub->GetName() << " matching event " << evt << endl; g_MercEventsLog.flush(); }
    DBG_DO { MDB(1) << "==hub== " << m_Hub->GetName() << " matching event " << evt << endl; g_MercEventsLog.flush(); }

    EventProcessType app_action = m_MercuryNode->GetApplication ()->EventAtRendezvous (evt, m_LastHop, pmsg->hopCount);

    if (app_action == EV_NUKE)
	return;

    START(PubAtRDV::ALL);

    if (from != NULL) {		// send an ACK if we got this from the network
	if (g_Preferences.use_cache)
	    SendAck(pmsg);
    }

    ///// MEASUREMENT
    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	DiscoveryLatEntry ent(DiscoveryLatEntry::PUB_STORE, 
			      pmsg->hopCount, pmsg->GetEvent()->GetNonce());
	if (from == NULL)
	    LOG(DiscoveryLatLog, ent);
	else
	    LOG(DiscoveryLatLog, ent, pmsg->recvTime); // use msg recv time
    }
    ///// MEASUREMENT

    if (app_action == EV_MATCH || app_action == EV_MATCH_AND_STORE) {
	DeliverPubToSubscribers(pmsg, pubMatchesLeft, pubMatchesRight);
    }

    if (g_Preferences.enable_pubtriggers) {
	if (app_action == EV_STORE || app_action == EV_MATCH_AND_STORE) {
	    Event *ev = pmsg->GetEvent ();
	    DBG_DO { g_MercEventsLog << "==hub== " << m_Hub->GetName() << " storing trigger " << ev << endl; g_MercEventsLog.flush(); }
	    DBG << " storing trigger " << ev << endl;

	    ev->SetDeathTime (m_Scheduler->TimeNow () + ev->GetLifeTime() );
	    m_Store->StoreTrigger (pmsg);
	}
    }
    STOP(PubAtRDV::ALL);
}

bool loopingMessage = false;

// NOTE: the last argument is passed by reference!

bool PubsubRouter::RouteRangeData(IPEndPoint *from, Message *msg, const IPEndPoint* &next_hop)
{
    bool left, right, center;
    bool is_linear = false;
    Constraint *cst = NULL;

    if (msg->GetType() == MSG_SUB) {
	MsgSubscription *smsg = (MsgSubscription *) msg;

	cst = (Constraint *) smsg->GetInterest()->GetConstraintByAttr (m_Hub->GetID());
	is_linear = smsg->IsRoutingModeLinear();
    }
    else {
	MsgPublication *pmsg = (MsgPublication *) msg;

	cst = (Constraint *) pmsg->GetEvent()->GetConstraintByAttr (m_Hub->GetID());
	is_linear = pmsg->IsRoutingModeLinear();
    }

    if (msg->GetType () == MSG_PUB) 
	DB(20) << "routing msg (is_linear=" << is_linear << ")" << msg << "from = " << (from == NULL ? "null" : from->ToString()) << endl;

    if (cst == NULL) {
	WARN << "routing attribute " << m_Hub->GetName() << " not found in subscription - ignoring " << endl;
	return false; 
    }

    ASSERT (cst->GetMin () >= m_Hub->GetAbsMin ());
    ASSERT (cst->GetMin () <= m_Hub->GetAbsMax ());
    ASSERT (cst->GetMax () >= m_Hub->GetAbsMin ());
    ASSERT (cst->GetMax () <= m_Hub->GetAbsMax ());

    cst->Clamp(m_Hub->GetAbsMin(), m_Hub->GetAbsMax());
    DB_DO(20) { MDB(1) << " constraints = " << cst << endl; }

    if (cst->GetMin() > cst->GetMax()) { 
	DBG_DO { WARN << "Very stupid sub/rangepub -- min=(" << cst->GetMin() << ") > max=(" << cst->GetMax() << ")" << endl; }
	return false;
    }

    START(PubsubRouter::RouteRangeData);
#define RETURN(x)  do { \
    STOP(PubsubRouter::RouteRangeData); return (x); } while (0)

    cst->GetRouteDirections (*m_Hub->GetRange(), left, center, right, m_Hub->AmRightMost ());

    if (msg->hopCount >= Parameters::MaxMessageTTL - 5) {
	DB_DO (-5) {
	    MTWARN << " Looping message from " << from << " msg=" << msg << endl;
	}
	loopingMessage = true;
	if (msg->hopCount >= Parameters::MaxMessageTTL)
	    RETURN(false);
    }

    if (!is_linear)
    {
	// We are in the hop-long-pointers mode here
	// Check if we have reached the left most end!
	if (!left && center) 
	{
	    // Change the mode here....
	    MDB(20) << "changing the mode here to linear " << endl;
	    is_linear = true;

	    START(PubsubRouter::nonlinear_to_linear);
	    if (msg->GetType() == MSG_SUB) {
		START(PubsubRouter::nonlinear_to_linear::HandleSub);
		HandleSubAtRendezvous(from, (MsgSubscription *) msg);
		((MsgSubscription *) msg)->ChangeModeToLinear();
		STOP(PubsubRouter::nonlinear_to_linear::HandleSub);
	    }
	    else {
		START(PubsubRouter::nonlinear_to_linear::HandlePub);
		HandlePubAtRendezvous(from, (MsgPublication *) msg, left, right);
		((MsgPublication *) msg)->ChangeModeToLinear();
		STOP(PubsubRouter::nonlinear_to_linear::HandlePub);
	    }
	    STOP(PubsubRouter::nonlinear_to_linear);
	}

	// Did we end up changing the mode?
	if (is_linear) {
	    if (g_Preferences.fanout_pubs) {
		if (right) {
		    if (msg->GetType () == MSG_SUB) {
			MsgLinearSubscription *lsmsg = new MsgLinearSubscription ( *(MsgSubscription *) msg, (Value &) cst->GetMax ());
			DoFanout (cst, lsmsg);
			delete lsmsg;
		    }
		    else {
			MsgLinearPublication *lpmsg = new MsgLinearPublication ( *(MsgPublication *) msg, (Value &) cst->GetMax ());
			DoFanout (cst, lpmsg);
			delete lpmsg;

		    }
		}

		// avoid the normal routing of range-items
		next_hop = NULL;
	    }
	    else {
		if (right) {
		    if (!CheckAppLinear (msg)) 
			return (false);

		    next_hop = &m_Hub->GetSuccessor()->GetAddress();
		}
	    }
	}
	else {
	    MDB (20) << " computing next hop to route for value:" << cst->GetMin() << endl;
	    START(PubsubRouter::ComputeNextHop);

	    if (!CheckAppRoute (msg))
		return false;

	    next_hop = ComputeNextHop (cst->GetMin());
	    STOP(PubsubRouter::ComputeNextHop);
	}
    }
    else {
	// We are in linear mode here... center better be true always!
	if (center) {
	    if (msg->GetType() == MSG_SUB)  
		HandleSubAtRendezvous(from, (MsgSubscription *) msg);
	    else 
		HandlePubAtRendezvous(from, (MsgPublication *) msg, left, right);
	}

	// proceed to the next destination
	if (right) {
	    if (!CheckAppLinear (msg))
		return false;

	    next_hop = &m_Hub->GetSuccessor()->GetAddress();
	}
	else
	    return false;
    }

    STOP (PubsubRouter::RouteRangeData);
#undef RETURN
    return true;
}

void PubsubRouter::RouteData (IPEndPoint *from, Message *msg)
{
    const IPEndPoint *next_hop = NULL;
    Interest *sub = NULL;
    loopingMessage = false;

    if (m_Hub->GetRange () == NULL) {
	DB_DO (10) { MWARN << "asked to route while not joined!"; }
	return;
    }

    START(PubsubRouter::MSRLOG);
    DoMeasurementLog(msg, m_Hub, from);
    STOP(PubsubRouter::MSRLOG);

    if (!RouteRangeData(from, msg, next_hop)) 
	return;

    if (next_hop != NULL)
    {
	m_Network->SendMessage(msg, (IPEndPoint *) next_hop, Parameters::TransportProto);
    }
}


void PubsubRouter::TriggerPublications(MsgSubscription *smsg)
{
    Interest *interest = smsg->GetInterest();
    TimeVal now = m_Scheduler->TimeNow ();
    Application *app = m_MercuryNode->GetApplication ();

#ifdef PUBSUB_DEBUG
    {
	DebugEntry ent (true, interest, NULL);
	LOG(DebugLog, ent);
    }
#endif
    // go through the list of all publications; expire the ones which need expiring

    list<MsgPublication *> matches;
    m_Store->GetOverlapTriggers (interest, &matches);

    for (list<MsgPublication *>::iterator it = matches.begin (); it != matches.end (); ++it) {
	MsgPublication *pmsg = *it;
	Event *ev = pmsg->GetEvent();

	MsgPublication *nmsg = pmsg->Clone();
	Event *nev = nmsg->GetEvent();

	ev->m_TriggerCount++;
	nev->SetMatched();

	// Set the lifetime of the returned pub to the TTL remaining
	// (the min of the two, since that is when a new sub or pub
	// will be regenerated) Jeff: XXX - not sure why I originally
	// did this... we should just return the TTL of the event
	// (the app knows the TTL of the sub)
	nev->SetLifeTime( (uint32)MAX(ev->GetDeathTime() - now, 0) );

	app->EventInterestMatch (nev, interest, smsg->sender);

	///// MEASUREMENT
	uint32 new_nonce = CreateNonce();

	if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	    DiscoveryLatEntry alias1(DiscoveryLatEntry::ALIAS, 0, 
				     interest->GetNonce(),
				     new_nonce);
	    LOG(DiscoveryLatLog, alias1);

	    DiscoveryLatEntry alias2(DiscoveryLatEntry::ALIAS, 0, 
				     nev->GetNonce(),
				     new_nonce);
	    LOG(DiscoveryLatLog, alias2);
	    nev->SetNonce(new_nonce);

	    DiscoveryLatEntry ent(DiscoveryLatEntry::MATCH_SEND, 
				  0, nev->GetNonce());
	    LOG(DiscoveryLatLog, ent);
	}
	///// MEASUREMENT

#ifdef PUBSUB_DEBUG
	{
	    DebugEntry ent (true /* is_trigger */, interest, nev);
	    LOG(DebugLog, ent);
	}
#endif

	// XXX: ASHWIN: GRUESOME hack. look elsewhere too...
	nmsg->hubID = 0xff; 
	m_Network->SendMessage(nmsg, (IPEndPoint *) &interest->GetSubscriber(), Parameters::TransportProto);
	delete nmsg;
    }
}

void PubsubRouter::HandleSubAtRendezvous(IPEndPoint * from,  MsgSubscription * smsg)
{
    Interest *sub = smsg->GetInterest();
    Hub *hub = m_Hub;

    if (!sub)
	return;

    InterestProcessType app_action = m_MercuryNode->GetApplication ()->InterestAtRendezvous (sub, m_LastHop);
    if (app_action == IN_NUKE)
	return;

    START(SubAtRDV::Log);
    ///// MEASUREMENT
    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	DiscoveryLatEntry ent(DiscoveryLatEntry::SUB_STORE, 
			      smsg->hopCount, sub->GetNonce());
	if (from == NULL)
	    LOG(DiscoveryLatLog, ent);
	else
	    LOG(DiscoveryLatLog, ent, smsg->recvTime); // use recv time
    }
    ///// MEASUREMENT

    if (app_action == IN_STORE || app_action == IN_STORE_AND_TRIGGER) {
	DBG_DO { g_MercEventsLog << __FILE__ << ":" << __LINE__ << "==hub== " << hub->GetName() << " storing sub " << sub << endl; g_MercEventsLog.flush(); }
	if (g_Preferences.use_softsubs) 
	    sub->SetDeathTime (m_Scheduler->TimeNow () + sub->GetLifeTime ());

	m_Store->StoreSub (sub);
    }
    STOP(SubAtRDV::Log);

    if (g_Preferences.enable_pubtriggers) {
	if (app_action == IN_TRIGGER || app_action == IN_STORE_AND_TRIGGER) {
	    START(SubAtRDV::Trigger);
	    TriggerPublications(smsg);
	    STOP(SubAtRDV::Trigger);
	}
    }
}

// returns 'true' if  'first' is nearer to 'start'
// than 'second'; ties are broken in no particular order

static bool IsCloser(const Value &start, const Value &first, const Value &second)
{
    if (first > start) {
	//  start -- 2 -- 1  ==> 2 is closer
	if (second >= start && second <= first)
	    return false;
	else
	    return true;
    }
    else {
	//  1 -- 2 -- start  ==> 1 is closer
	if (second >= first && second <= start)
	    return true;
	else
	    return false;
    }
}

/**
 * Compute the next hop for routing to this value. This is
 * done by keeping all known nodes in a sorted list and
 * selecting the neighbor which makes furthest progress, but
 * is not past the destination value.
 *
 */

const IPEndPoint *PubsubRouter::ComputeNextHop (const Value& val)
{
    DB_DO (-5) {
	if (loopingMessage) {
	    cerr << "*** me (" << m_Address << ") range=" << m_Hub->GetRange()
		 << " computing next hop for " << val << endl;
	}
    }

    // Move thru the sorted list of known nodes.
    START(PubsubRouter::CallingGetNearestPeer);
    Peer *peer = m_Hub->GetNearestPeer (val);
    STOP(PubsubRouter::CallingGetNearestPeer);

    if (peer == NULL) {
	DBG_DO { MWARN << "no known peers to route to! " << endl; }
	return NULL;
    }

    if (peer->GetRange ().Covers (val))
	return &peer->GetAddress();

    if (g_Preferences.use_cache) {
	START(PubsubRouter::CallingLookupEntry);		
	CacheEntry *entry = m_Cache->LookupEntry (val);
	STOP(PubsubRouter::CallingLookupEntry);

	// now we have two candidates; find the closer one
	if (entry && 
	    IsCloser(m_Hub->GetRange()->GetMin(), entry->GetRange().GetMin(), 
		     peer->GetRange().GetMin())) 
	    return &entry->GetAddress();
    }

    // sending to our immediate pred is stupidity unless its range 
    // covers the value (checked above)
    Peer *pred = m_Hub->GetPredecessor ();

    if (pred && pred->GetAddress () == peer->GetAddress () && !peer->IsSuccessor ()) {
	MWARN << "ROUTE FAILURE: hub " << (int) m_Hub->GetID () << 
	    " was sending (" << val << ") to PRED. ring looks unstable." << endl;
	return NULL;
    }
    else 
	return &peer->GetAddress();
}

void PubsubRouter::DeliverPubToSubscribers(MsgPublication * pmsg, 
					   bool pubMatchesLeft, bool pubMatchesRight)
{
    map<SID, list<Interest *> , less_SID> matched_map;
    Event *pub;
    Application *app = m_MercuryNode->GetApplication ();

    MemberHub *hub = m_Hub;
    pub = pmsg->GetEvent();

    TimeVal now = m_Scheduler->TimeNow ();

    START(PubsubRouter::DeliverPubToSubscribers);
    START(PubsubRouter::DeliverPubToSubscribers::Matching);

    list<Interest *> matches;
#ifdef PUBSUB_DEBUG
    {
	DebugEntry ent (false, NULL, pub);
	LOG(DebugLog, ent);
    }
#endif
    m_Store->GetOverlapSubs (pmsg, &matches);

    for (list<Interest *>::iterator it = matches.begin (); it != matches.end (); ++it) 
    {
	Interest *interest = *it;

	// Jeff: only send publication back to the sender if explicitly enabled
	// (dunno why we would actually ever do that, but leave it as opt)
	if (!g_Preferences.send_backpub && 
	    interest->GetSubscriber() == pmsg->GetCreator()) {
	    MDB (10) << "doh! backpub issues?" << endl;
	    continue;
	}

	// might be expired
	if (interest->GetDeathTime() <= now) {
	    MDB (10) << "expired subscription " << interest << endl;
	    continue;
	}

	// check if this event would have overlapped with my predecessor's range 
	// if (yes), i don't need to perform a match.

	MDB (20) << endl << " >>>>> interest=" << interest << endl << " >>>>> matches pub=" << pub << endl;

	bool covers = true;

	Constraint *c = interest->GetConstraintByAttr (m_Hub->GetID());

	bool subMatchesLeft, subMatchesCenter, subMatchesRight;
	c->GetRouteDirections (*m_Hub->GetRange(), subMatchesLeft, subMatchesCenter, subMatchesRight, m_Hub->AmRightMost ());

	// nodes on the left would have matched this one;
	// this is an optimization; robustness issues?
	if (subMatchesLeft && pubMatchesLeft)
	    covers = false;

	if (covers) {			
	    SID *subscriber = (SID *) &interest->GetSubscriber();

	    map<SID, list<Interest *> , less_SID>::iterator map_iter;

	    map_iter = matched_map.find(*subscriber);

	    if (map_iter == matched_map.end()) {
		list<Interest *>  l;
		l.push_back(interest);
		matched_map.insert(
		    map<SID, list<Interest *> , less_SID>::value_type(*subscriber, l));
	    }
	    else {
		map_iter->second.push_back(interest);
	    }
	}
    }
    STOP(PubsubRouter::DeliverPubToSubscribers::Matching);

    NOTE(MATCHED_PEOPLE_COUNT, matched_map.size());

    START(PubsubRouter::DeliverPubToSubscribers::AggregateSending);
    for (map<SID, list<Interest *> , less_SID>::iterator map_iter = matched_map.begin();
	 map_iter != matched_map.end(); 
	 map_iter++) 
    {
	SID subscriber = map_iter->first;
	list<Interest *>  *l = &(map_iter->second);

	MsgPublication *smsg = pmsg->Clone(); 
	pub = smsg->GetEvent();
	pub->SetMatched();

	///// MEASUREMENT
	// We have to alias both the pubs and subs here because they can
	// be matched more than once; if we did not alias them then we might
	// not be able to reconstruct the "tree" formed by spread of 
	// matched pubs.
	uint32 new_nonce = CreateNonce();
	if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	    DiscoveryLatEntry alias(DiscoveryLatEntry::ALIAS, 0, 
				    pub->GetNonce(), new_nonce);
	    LOG(DiscoveryLatLog, alias);
	    pub->SetNonce(new_nonce);
	}
	///// MEASUREMENT

	uint32 maxSubTTLLeft = 0;

	for (list<Interest *>::iterator iter = l->begin(); iter != l->end(); iter++) {
	    app->EventInterestMatch (pub, *iter, subscriber);

	    maxSubTTLLeft = (uint32)MAX( (sint64)maxSubTTLLeft, (*iter)->GetDeathTime() - now );

	    ///// MEASUREMENT
	    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
		DiscoveryLatEntry alias(DiscoveryLatEntry::ALIAS, 0, 
					(*iter)->GetNonce(), new_nonce);
		LOG(DiscoveryLatLog, alias);
	    }
	    ///// MEASUREMENT
	}
	DBG_DO{ g_MercEventsLog << "sending matched publication " << pub << " ==to== " << &(subscriber) << endl; }
	DBG_DO { g_MercEventsLog.flush(); }

	// set the matched pub's lifetime to be the time remaining
	// let this be the min of the pub's life remaining and the max
	// time remaining of the matched subs
	pub->SetLifeTime( MIN(pmsg->GetEvent()->GetLifeTime(), maxSubTTLLeft) );

	///// MEASUREMENT
	if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	    DiscoveryLatEntry ent(DiscoveryLatEntry::MATCH_SEND, 
				  0, pub->GetNonce());
	    LOG(DiscoveryLatLog, ent);
	}
	///// MEASUREMENT

#ifdef PUBSUB_DEBUG
	{
	    DebugEntry ent (false /* is_trigger */, l->front () /* use somebody? */, pub);
	    LOG(DebugLog, ent);
	}
#endif

	// what to do about so many TCP connections?

	// TOTALLY GRUESOME hack -- cross hub pubs are marked 
	// with this hubID. sometime, i should redo this stupid
	// MessageHandler thing. - Ashwin [03/11/2005]  
	smsg->hubID = 0xff; 
	m_Network->SendMessage(smsg, &(subscriber), Parameters::TransportProto);
	delete smsg;
    }
    STOP(PubsubRouter::DeliverPubToSubscribers::AggregateSending);

    STOP(PubsubRouter::DeliverPubToSubscribers);
}

static bool matchsub_predicate (MemberHub *h, const NodeRange *range, list<Interest *> *ml, Interest *i)
{
    Constraint *cst = i->GetConstraintByAttr (h->GetID ());
    if (cst->OverlapsNodeRange (*range))
	ml->push_back (i);

    /* make sure the interest is not deleted */
    return false;
};

void PubsubRouter::GetMatchingSubscriptions (const NodeRange& range, list<Interest *>& matched)
{
    m_Store->DeleteSubs (wrap (matchsub_predicate, m_Hub, &range, &matched));
}

static bool matchtrigger_predicate (MemberHub *h, const NodeRange* range, list<MsgPublication *> *ml, MsgPublication *tr)
{
    Constraint *cst = tr->GetEvent ()->GetConstraintByAttr (h->GetID ());
    if (cst->OverlapsNodeRange (*range))
	ml->push_back (tr);

    return false;
}

void PubsubRouter::GetMatchingTriggers (const NodeRange& range, list<MsgPublication *>& matched)
{
    m_Store->DeleteTriggers (wrap (matchtrigger_predicate, m_Hub, &range, &matched));
}

static bool handovertrigger_predicate (MemberHub *h, const NodeRange *range, MsgTriggerList *tlm, TimeVal *now, MsgPublication *tr)
{
    Event *ev = tr->GetEvent ();
    if (ev->GetDeathTime () <= *now) 
	return true;

    Constraint *cst = ev->GetConstraintByAttr (h->GetID ());

    if (!cst->OverlapsNodeRange (*range)) 
	return false;

    ev->SetLifeTime (ev->GetDeathTime () - *now);
    tlm->AddTrigger (tr);

    return !cst->OverlapsNodeRange (*h->GetRange ());
}

static bool handoversub_predicate (MemberHub *h, const NodeRange *range, MsgSubscriptionList *slm, TimeVal *now, Interest *i)
{
    if (i->GetDeathTime () <= *now) 
	return true;

    Constraint *cst = i->GetConstraintByAttr (h->GetID ());

    if (!cst->OverlapsNodeRange (*range)) 
	return false;

    i->SetLifeTime (i->GetDeathTime () - *now);
    slm->AddSubscription (i);

    return !cst->OverlapsNodeRange (*h->GetRange ());
}


vector<int> PubsubRouter::HandoverTriggers(IPEndPoint *to)
{
    vector<int> stats;

    TimeVal now = m_Scheduler->TimeNow ();
    MsgTriggerList *tlmsg = new MsgTriggerList(m_Hub->GetID(), m_Address);

    bool is_pred = (*to == m_Hub->GetPredecessor ()->GetAddress ()); 
    const NodeRange& nbr_range = is_pred ? m_Hub->GetPredecessor ()->GetRange () : m_Hub->GetSuccessor ()->GetRange ();	

    m_Store->DeleteTriggers (wrap (handovertrigger_predicate, m_Hub, &nbr_range, tlmsg, &m_Scheduler->TimeNow ()));

    stats.push_back (tlmsg->size ());      // #triggers
    stats.push_back (tlmsg->GetLength ()); // size in bytes

    if (tlmsg->size() > 0)
	m_Network->SendMessage(tlmsg, to, PROTO_TCP);
    delete tlmsg;

    /// somebody joined us; this could be due to load balancing. 
    /// start measuring pub-sub load again...

    m_RoutedSubs = m_RoutedPubs = 0;
    m_RoutingLoad = 0;

    return stats;
}

/**
 * called when we split our range in HandleJoinRequest; 
 * also when load balancing changes ranges 
 * Hand over the subscriptions which no longer belong to us.
 **/

vector<int> PubsubRouter::HandoverSubscriptions(IPEndPoint *to)
{
    vector<int> stats;

    TimeVal now = m_Scheduler->TimeNow ();
    MsgSubscriptionList *slmsg = new MsgSubscriptionList (m_Hub->GetID(), m_Address);

    bool is_pred = (*to == m_Hub->GetPredecessor ()->GetAddress ());
    const NodeRange& nbr_range = is_pred ? m_Hub->GetPredecessor ()->GetRange () : m_Hub->GetSuccessor ()->GetRange ();

    m_Store->DeleteSubs (wrap (handoversub_predicate, m_Hub, &nbr_range, slmsg, &m_Scheduler->TimeNow ()));

    stats.push_back (slmsg->size ());          // #subscriptions
    stats.push_back (slmsg->GetLength ());     // size in bytes

    if (slmsg->size() > 0)
	m_Network->SendMessage(slmsg, to, PROTO_TCP);
    delete slmsg;

    /// somebody joined us; this could be due to load balancing. 
    /// start measuring pub-sub load again...

    m_RoutedSubs = m_RoutedPubs = 0;
    m_RoutingLoad = 0;

    return stats;
}

static bool oorsub_predicate (MemberHub *h, const NodeRange *range, TimeVal *now, Interest *i)
{	
    if (i->GetDeathTime () <= *now) 
	return true;

    Constraint *cst = i->GetConstraintByAttr (h->GetID ());
    return !cst->OverlapsNodeRange (*range);
};

static bool oortrigger_predicate (MemberHub *h, const NodeRange *range, TimeVal *now, MsgPublication *tr)
{
    Event *ev = tr->GetEvent ();
    if (ev->GetDeathTime () <= *now)
	return true;

    Constraint *cst = ev->GetConstraintByAttr (h->GetID ());
    return !cst->OverlapsNodeRange (*range);

};

void PubsubRouter::PurgeOutofRangeData ()
{
    m_Store->DeleteSubs (wrap (oorsub_predicate, m_Hub, m_Hub->GetRange (), &m_Scheduler->TimeNow ()));
    m_Store->DeleteTriggers (wrap (oortrigger_predicate, m_Hub, m_Hub->GetRange (), &m_Scheduler->TimeNow ()));
}

void PubsubRouter::AddNewTrigger (MsgPublication *pmsg)
{
    Event *ev = pmsg->GetEvent ();

    Constraint *cst = ev->GetConstraintByAttr (m_Hub->GetID());
    if (!cst->OverlapsNodeRange (*m_Hub->GetRange ()))
	return;

    ev->SetDeathTime (m_Scheduler->TimeNow () + ev->GetLifeTime() );
    m_Store->StoreTrigger (pmsg);
}

void PubsubRouter::AddNewInterest (Interest *nin)
{
    Constraint *cst = nin->GetConstraintByAttr (m_Hub->GetID ());
    if (!cst->OverlapsNodeRange (*m_Hub->GetRange ()))
	return;

    m_Store->StoreSub (nin);
}

void PubsubRouter::HandleSubscriptionList(IPEndPoint * from, MsgSubscriptionList * slmsg)
{
    for (list<Interest *>::iterator it = slmsg->begin(); it != slmsg->end(); it++) 
	AddNewInterest (*it);
}

void PubsubRouter::HandleTriggerList(IPEndPoint *from, MsgTriggerList *tlmsg) 
{
    for (list<MsgPublication *>::iterator it = tlmsg->begin(); it != tlmsg->end(); it++) 
	AddNewTrigger (*it);
}

static bool printsub_predicate (ostream *os, Interest *i)
{
    *os << i->GetSubscriber() << ": " << i << endl;
    return false;
}

void PubsubRouter::PrintSubscriptionList(ostream& stream)
{
    m_Store->DeleteSubs (wrap (printsub_predicate, &stream));
}

static bool printpub_predicate (ostream *os, MsgPublication *pmsg)
{
    *os << pmsg->GetCreator() << ": " << pmsg->GetEvent() << endl;
    return false;
}

void PubsubRouter::PrintPublicationList(ostream& stream)
{
    m_Store->DeleteTriggers (wrap (printpub_predicate, &stream));
}

// Utility routine
static void DoMeasurementLog(Message *msg, MemberHub *hub, IPEndPoint *from)
{   
    MsgType t = msg->GetType();

    if (t == MSG_SUB) {
	Interest *sub = ((MsgSubscription *) msg)->GetInterest();

	DBG_DO { g_MercEventsLog << __FILE__ << ":" << __LINE__ << "==hub== " << hub->GetName() << " routing sub " << sub << endl; g_MercEventsLog.flush(); }
	DBG_DO { 
	    MercuryNode *m_MercuryNode = hub->GetMercuryNode ();
	    MDB(1) << "==hub== " << hub->GetName() << " routing sub " << sub << endl; g_MercEventsLog.flush(); 
	}

	//// MEASUREMENT
	if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	    DiscoveryLatEntry ent(DiscoveryLatEntry::SUB_ROUTE_RECV, 
				  msg->hopCount, sub->GetNonce());
	    if (from == NULL)
		LOG(DiscoveryLatLog, ent);
	    else
		LOG(DiscoveryLatLog, ent, msg->recvTime);
	}
	//// MEASUREMENT
    }
    else if (t == MSG_PUB) {
	Event *evt = ((MsgPublication *) msg)->GetEvent();

	// Matched publications should be handled by MercuryNode directly..
	DBG_DO { g_MercEventsLog << __FILE__ << ":" << __LINE__ << " ==hub== " << hub->GetName() << " routing event " << evt << endl; g_MercEventsLog.flush(); }
	DBG_DO { 
	    MercuryNode *m_MercuryNode = hub->GetMercuryNode ();
	    MDB(1) << "==hub== " << hub->GetName() << " routing event " << evt << endl; g_MercEventsLog.flush(); 
	}

	//// MEASUREMENT
	if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	    DiscoveryLatEntry ent(DiscoveryLatEntry::PUB_ROUTE_RECV, 
				  msg->hopCount, evt->GetNonce());
	    if (from == NULL)
		LOG(DiscoveryLatLog, ent);
	    else
		LOG(DiscoveryLatLog, ent, msg->recvTime);
	}
	//// MEASUREMENT
    }
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
