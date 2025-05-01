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

/**
 * copyright:  (C) 2004-2005 Ashwin R. Bharambe (ashu@cs.cmu.edu)
 *
 * $Id: LinkMaintainer.cpp 2638 2006-01-26 00:29:30Z jeffpang $
 **/

#include <math.h>
#include <mercury/LinkMaintainer.h>
#include <mercury/NetworkLayer.h>
#include <mercury/Message.h>
#include <mercury/Hub.h>
#include <mercury/Sampling.h>
#include <mercury/Parameters.h>
#include <mercury/Utils.h>
#include <mercury/PubsubRouter.h>
#include <mercury/Event.h>
#include <mercury/Interest.h>
#include <mercury/Scheduler.h>
#include <mercury/MercuryNode.h>
#include <mercury/LoadBalancer.h>
#include <mercury/RoutingLogs.h>

LinkMaintainer::LinkMaintainer(MemberHub *hub)
    : m_Hub(hub),  m_JoinRequestTimer (new refcounted<JoinRequestTimer>(m_Hub, m_Hub->GetBootstrapRep (), 0)), m_SLMaintInvocations (0) 
{	
    m_MercuryNode = m_Hub->GetMercuryNode ();
    m_Network = m_Hub->GetNetwork();
    m_Address = m_Hub->GetAddress ();
    m_Scheduler = m_Hub->GetScheduler ();

    m_Epoch = 0;

    // Register interest in the following messages; so that
    // the Handlers will be called programmatically, instead
    // of adding switch statements every which place!

    MsgType msgs[] = { MSG_JOIN_REQUEST, MSG_JOIN_RESPONSE, MSG_NOTIFY_SUCCESSOR, 
		       MSG_GET_PRED, MSG_PRED, MSG_GET_SUCCLIST, MSG_SUCCLIST,
		       MSG_NBR_REQ, MSG_NBR_RESP, MSG_LEAVE_NOTIFICATION, MSG_LINK_BREAK
    };

    for (uint32 i = 0; i < (sizeof(msgs) / sizeof(MsgType)); i++)
	m_MercuryNode->RegisterMessageHandler(msgs[i], this);
}

void LinkMaintainer::ProcessMessage(IPEndPoint *from, Message *msg)
{
    if (msg->hubID != m_Hub->GetID())
	return;

    MsgType t = msg->GetType();

    if (t == MSG_JOIN_REQUEST) 
	HandleJoinRequest(from, (MsgJoinRequest *) msg);
    else if (t == MSG_JOIN_RESPONSE) 
	HandleJoinResponse(from, (MsgJoinResponse *) msg);
    else if (t == MSG_NOTIFY_SUCCESSOR) 
	HandleNotifySuccessor(from, (MsgNotifySuccessor *) msg);
    else if (t ==  MSG_GET_PRED) 
	HandleGetPred(from, (MsgGetPred *) msg);
    else if (t ==  MSG_PRED) 
	HandlePred(from, (MsgPred *) msg);
    else if (t ==  MSG_GET_SUCCLIST) 
	HandleGetSuccList(from, (MsgGetSuccessorList *) msg);
    else if (t ==  MSG_SUCCLIST) 
	HandleSuccList(from, (MsgSuccessorList *) msg);
    else if (t == MSG_NBR_REQ)
	HandleNeighborRequest (from, (MsgNeighborRequest *) msg);
    else if (t ==  MSG_NBR_RESP) 
	HandleNeighborResponse(from, (MsgNeighborResponse *) msg);
    else if (t == MSG_LEAVE_NOTIFICATION) 
	HandleLeaveNotification (from, (MsgLeaveNotification *) msg);
    else if (t == MSG_LINK_BREAK)
	HandleLinkBreak (from, (MsgLinkBreak *) msg);
    else 
	WARN << merc_va("LinkMaintainer:: received some idiotic message [%s]", msg->TypeString()) << endl;
}

void LinkMaintainer::Start()
{
    m_MercuryNode->GetApplication ()->JoinBegin (m_JoinRequestTimer->GetRepAddress ());

    m_Scheduler->RaiseEvent (m_JoinRequestTimer, m_Address, m_JoinRequestTimer->GetNextDelay ());
}

class NotifyContinuation : public SchedulerEvent {
    MemberHub  *h;
    IPEndPoint  succ;
public:
    NotifyContinuation (MemberHub *h, const IPEndPoint& succ) : h (h), succ (succ) {}

    void Execute (Node& node, TimeVal& timenow) {
	MsgNotifySuccessor *ns = new MsgNotifySuccessor (h->GetID (), node.GetAddress (), *h->GetRange ());
	node.GetNetwork ()->SendMessage (ns, &succ, PROTO_TCP);      // we dont want to lose this
	delete ns;
    }
};


bool LinkMaintainer::IsRequestDuplicate (IPEndPoint *from)
{
    bool found = false;
    TimeVal now = m_Scheduler->TimeNow ();

    // time-out as well as check for existence!
    for (JRMapIter it = m_JoinResponseSIDs.begin (); it != m_JoinResponseSIDs.end (); /* it++ */)
    {
	if (it->second <= now) {
	    JRMapIter oit (it);
	    ++oit;

	    m_JoinResponseSIDs.erase (it);
	    it = oit;
	}
	else {
	    if (it->first == *from)
		found = true;
	    ++it;
	}
    }

    return found;
}

////////////////////////////////////////////////////////////
// Handle an incoming join request from the guy who will be
// our new predecessor Send him a response if it's ok for
// him to join us. Give him half of our range.
//
// Update the successor of my erstwhile predecessor (which
// could be me myself, if I am the only person in this
// attribute hub)
////////////////////////////////////////////////////////////

void LinkMaintainer::HandleJoinRequest(IPEndPoint * from, MsgJoinRequest * jmsg)
{
    MDB (-5) << "received join request from " << from << endl;

    // a simple check to see if this node is our current predecessor
    // is not enough because in the meantime, somebody else could 
    // have joined in between! it's important that we decide whether
    // the request is a duplicate correctly because otherwise we end 
    // up splitting the range irrevocably! - Ashwin [02/21/2005]

    if (IsRequestDuplicate (from)) {
	MWARN << "dupe join request from " << from << "; ignoring... " << endl;
	return;
    }

    if (m_Hub->GetSuccessor () == NULL) {
	MWARN << " my successor is gone! not in a state to respond " << endl;
	return;
    }


    // XXX: these are blocking TCP messages; if we are not threaded, things can 
    // just block badly! We might want to think about a non-blocking stateful 
    // tcp-send layer underneath. :(  Ashwin [12/12/2004]
    //
    // for the join message, this is fine, since we are not going to be able to 
    // do much anyways :)       - Ashwin [01/04/2005]
    if (m_Hub->GetStatus() != ST_JOINED) {
	MsgJoinResponse *resp = new MsgJoinResponse(jmsg->hubID, m_Address, JOIN_AM_UNJOINED);
	m_Network->SendMessage(resp, from, PROTO_TCP);	// don't care if the msg didn't get delivered...
	delete resp;
	return;
    }

    // split range ; keep latter half; give early half to him
    Value min = m_Hub->GetRange()->GetMin();
    Value max = m_Hub->GetRange()->GetMax();    

    Value mid = min;
    Value span = m_Hub->GetRangeSpan ();
    span /= 2;
    
    mid += span;
    if (mid > m_Hub->GetAbsMax ()) {
	mid -= m_Hub->GetAbsMax ();
	mid += m_Hub->GetAbsMin ();
    }

    NodeRange *assigned = new NodeRange(m_Hub->GetID(), min, mid);
    MsgJoinResponse* resp = new MsgJoinResponse(m_Hub->GetID(), m_Address, JOIN_NO_ERROR, *assigned);

    // this boundary-condition annoys! 
    bool succIsOnlyNode = false;

    if (m_Hub->GetSuccessor ()->GetAddress () == m_Address) {
	succIsOnlyNode = true;

	// well, what if joinresponse fails to go, but this message goes? 
	// in reality, this will never happen, but the simulator makes it 
	// happen :) so, we set a flag in the joinresponse

	resp->SetSuccessorIsOnlyNode ();
	// ref<NotifyContinuation> cont = new refcounted<NotifyContinuation> (m_Hub, *from);
	// m_Scheduler->RaiseEvent (cont, m_Address, 50);
    }

    NodeRange newrange (m_Hub->GetID (), mid, max);

    // add successors to this response; the first one is me myself!
    resp->AddSuccessor(m_Address, newrange);
    PList *succlist = &m_Hub->GetSuccessorList();
    for (PeerListIter it = succlist->begin(); it != succlist->end(); it++)
    {
	ref<Peer>& p = *it;
	if (p->GetAddress () == m_Address)
	    continue;
	resp->AddSuccessor(p->GetAddress(), p->GetRange());
    }	

#ifdef LOADBAL_TEST
    float load = 0.0;
    if (g_Preferences.do_loadbal) {
	load = m_Hub->GetLoadBalancer ()->GetMyLoad ();
	if (jmsg->IsLeaveJoinRelated ()) { 
	    resp->SetLoad (load / 2.0);
	}
    }
#endif

    if (m_Network->SendMessage(resp, from, PROTO_TCP) < 0) {
	MWARN << "could not send join response to " << from << endl;
    }
    else {
	// do stuff only when you are sure the message at least left us!
	NodeRange oldrange = *m_Hub->GetRange ();

	m_Hub->GetPubsubRouter ()->UpdateRangeLoad (newrange);
	m_Hub->SetRange(&newrange);

	if (succIsOnlyNode) {
	    m_Hub->SetSuccessor (*from, *assigned);
	}

	m_Hub->SetPredecessor (*from, *assigned);

	// remember that we sent this JoinResponse 
	TimeVal expiry = m_Scheduler->TimeNow () + Parameters::TCPFailureTimeout;
	m_JoinResponseSIDs.insert (JRMap::value_type (*from, expiry));

	// FIXME: this is actually not the right place to handover
	// subscriptions - since ideally we should do it only after the
	// predecessor is established etc. otherwise matching guarantees
	// can't be given!
	//
	// we will live with that for now. one possible solution is the
	// following: - send the new predecessor its new range and the
	// subscriptions - but keep those subscriptions with us as well (for
	// some time) - during this time, we should not switch to our new
	// range...
	//
	// however, there may be complexities involved with concurrent joins
	// - Ashwin [01/04/2005]	

	vector<int> ss = m_Hub->GetPubsubRouter ()->HandoverSubscriptions(from);
	vector<int> sp = m_Hub->GetPubsubRouter ()->HandoverTriggers(from);
	if (g_Preferences.do_loadbal) {
	    LoadBalEntry ent (LoadBalEntry::JOIN, (int) ss[0] + sp[0], (int) ss[1] + sp[1], *from);
	    LOG(LoadBalanceLog, ent);
	}

#ifdef LOADBAL_TEST
	if (g_Preferences.do_loadbal) {
	    LoadMetric *lm = new LoadMetric (load / 2.0);
	    m_Hub->GetLoadSampler ()->SetLoad (lm);
	    delete lm;
	}
#endif
	m_MercuryNode->GetApplication ()->RangeContracted (oldrange, *m_Hub->GetRange ());
    }
    delete resp;	
    delete assigned;


    MDB(1) << "after responding to a join request:" << m_Hub << endl;
    TINFO << "current range=" << m_Hub->GetRange () << " succ=" << m_Hub->GetSuccessor () << endl;
}

void LinkMaintainer::HandleJoinResponse(IPEndPoint * from, MsgJoinResponse * msg)
{
    MemberHub *hub = m_Hub;

    switch (msg->GetError()) {
    case JOIN_NO_ERROR:
	break;

    case JOIN_AM_UNJOINED:
	MWARN << "successor is unjoined himself, cannot join!" << endl;
	hub->SetStatus(ST_UNJOINED);
	return;

    default:
	MWARN << "unknown join error encountered: " << msg->GetError() << endl;
	return;
    }

    MDB (1) << "received join response, range=" << msg->GetAssignedRange () << " from " << *from << endl;

    // this can happen if dupe responses arrive - this can happen
    // if i happen to send a dupe request due to delay in receiving 
    // the first response.
    if (hub->GetRange() != NULL) {	
	MWARN << "already had range! dupe response perhaps; ignoring..." << endl;
	return;
    }

    /// Cancel the timer
    m_JoinRequestTimer->Cancel();	

    NodeRange r (msg->GetAssignedRange ());
    hub->SetRange (&r);
    hub->SetSuccessorList(msg->GetPeerInfoList());

    if (msg->SuccessorIsOnlyNode ()) {
	Peer *succ = m_Hub->GetSuccessor ();
	MsgNotifySuccessor *ns = new MsgNotifySuccessor (m_Hub->GetID(), (IPEndPoint &) succ->GetAddress (), (NodeRange &) succ->GetRange ());
	HandleNotifySuccessor ((IPEndPoint *) &succ->GetAddress (), ns);
	delete ns;
    }
    // got a join response; so my join is complete! this will start successor maintenance
    DB_DO(-5) {
	INFO << "current range=" << m_Hub->GetRange () << " succ=" << m_Hub->GetSuccessor () << endl;
    }

#ifdef LOADBAL_TEST
    if (g_Preferences.do_loadbal) {
	MDB (-10) << " received join response setting load " << endl;
	LoadMetric *lm = new LoadMetric (msg->GetLoad ());
	m_Hub->GetLoadSampler ()->SetLoad (lm);
	delete lm;
    }
#endif

    hub->OnJoinComplete();
}

// Maintain the successor pointer more aggressively than the
// the successor list. The successor list timeout is a factor (2-3)
// times the successor timeout.
void LinkMaintainer::DoSuccListMaintenance()
{
    MDB (5) << "performing successor list maintenance" << endl;
    DBG_DO {
	m_Hub->PrintSuccessorList ();
    }

    // Maintain the successor pointer; send a GetPredecessor()
    Peer *succ = m_Hub->GetSuccessor();
    if (!succ) {
	MWARN << "no successor? " << endl;
	return;
    }

    // Why send request to oneself! :)
    if (succ->GetAddress () == m_Address)
	return;

    if (!g_Preferences.nosuccdebug) {
	FREQ(10, TINFO << "succ list maintainence. curr succ=" << succ << endl);
    }

    // If these requests are not answered, we will just assume that
    // succ will also stop responding to heartbeats; and will be removed.
    MsgGetPred *gp = new MsgGetPred(m_Hub->GetID(), m_Address);
    m_Network->SendMessage(gp, (IPEndPoint *) &succ->GetAddress(), Parameters::TransportProto);
    delete gp;

    // Maintain the successor list; send a GetSuccList()
    m_SLMaintInvocations++;
    //	if (m_SLMaintInvocations % 3 == 0) 
    {
	MsgGetSuccessorList *gsl = new MsgGetSuccessorList(m_Hub->GetID(), m_Address);
	m_Network->SendMessage(gsl, (IPEndPoint *) &succ->GetAddress(), Parameters::TransportProto);
	delete gsl;
    }
}

void LinkMaintainer::HandleGetPred(IPEndPoint *from, MsgGetPred *gp)
{
    if (m_Hub->GetStatus () != ST_JOINED)
	return;

    Peer *pred = m_Hub->GetPredecessor();	
    MsgPred *p;	
    if (pred == NULL) {
	DB_DO(1) { MWARN << " received GetPred from=" << from << " but pred=(null)?" << endl; }
	p = new MsgPred (m_Hub->GetID (), m_Address, SID_NONE, RANGE_NONE, RANGE_NONE);
    }
    else {
	p = new MsgPred (m_Hub->GetID(), m_Address, pred->GetAddress(), pred->GetRange(), *m_Hub->GetRange ());
    }

    m_Network->SendMessage(p, from, Parameters::TransportProto);
    delete p;
}

void LinkMaintainer::HandleGetSuccList(IPEndPoint *from, MsgGetSuccessorList *gsl)
{
    if (m_Hub->GetStatus () != ST_JOINED)
	return;

    Peer *pred = m_Hub->GetPredecessor();
    if (!pred || pred->GetAddress() != *from) {
	DBG_DO { 
	    MWARN << "received succlist request from unknown fellow: from=" 
		  << from << " pred=";
	    cerr << (pred ? pred->GetAddress ().ToString () : "(null)") << endl; 
	}
	return;
    }	

    if (!g_Preferences.nosuccdebug) {
	FREQ(10, {
	    TINFO << "hub " << (int) m_Hub->GetID () << ": succlist req from " 
		  << *from << endl;
	}
	    );
    }

    MsgSuccessorList *sl = new MsgSuccessorList(m_Hub->GetID(), m_Address);	
    PeerInfoList *l = sl->GetPeerInfoList();

    l->push_back(PeerInfo(m_Address, *(m_Hub->GetRange())));    // ooh, so many lispy braces!

    for (PeerListIter it = m_Hub->GetSuccessorList().begin(); it != m_Hub->GetSuccessorList().end(); it++)
    {
	ref<Peer>& p = *it;

	if (p->GetAddress () == m_Address)
	    continue;
	MDB (20) << "deciding to send " << p->GetAddress () << "," << p->GetRange () << " as successor " << endl;
	l->push_back(PeerInfo(p->GetAddress(), p->GetRange()));
    }
    MDB (20) << "------------------" << endl;

    m_Network->SendMessage(sl, from, Parameters::TransportProto);
    delete sl;
}

// NotifySuccessor() gets called here; however, this routine gets called
// only if our successor replies with the pred(). Therefore, it should be
// okay to not call NotifySuccessor() any place else.  - Ashwin [01/21/2005]

void LinkMaintainer::HandlePred(IPEndPoint *from, MsgPred *p)
{
    Peer *cursucc = m_Hub->GetSuccessor ();
    if (!cursucc) {
	DB_DO (-11) {
	    MTWARN << "received pred but have no successor!" << endl;
	}
	return;
    }
    if (cursucc->GetAddress () != *from) {
	DB_DO (-11) { 
	    MTWARN << "Huh? received pred response from somebody who's no longer my successor..." << endl;
	}
	return;
    }

    // notify if the guy is "predecessor-less" 

    if (p->GetPredAddress () == SID_NONE) {  
	MsgNotifySuccessor *ns = new MsgNotifySuccessor (m_Hub->GetID(), m_Address, *m_Hub->GetRange());
	m_Network->SendMessage (ns, from, Parameters::TransportProto);
	delete ns;
	return;
    }

    // make sure we do comparisons with the latest range of the successor!
    cursucc->SetRange (p->GetCurRange ());

    // everything's fine! my successor thinks i am its predecessor
    if (p->GetPredAddress() == m_Address)   {
	MDB (20) << ">>> succ's pred is me - great news!" << endl;

	// make sure my 'max' = succ's 'min'; also check circle wrapping
	NodeRange *currange = m_Hub->GetRange ();

	Value newmax = cursucc->GetRange ().GetMin ();
	if (newmax == m_Hub->GetAbsMin ())
	    newmax = m_Hub->GetAbsMax ();

	NodeRange newrange (currange->GetAttrIndex (), currange->GetMin (), newmax);
	if (newrange.GetMin () == newrange.GetMax ()) {
	    MWARN << "New range=" << newrange << " is of zero-width! disregarding this MsgPred " << endl;
	    return;
	}

	if (*m_Hub->GetRange () != newrange) 
	    m_MercuryNode->GetApplication ()->RangeExpanded (*m_Hub->GetRange (), newrange);

	m_Hub->SetRange (&newrange);
	return;     
    }

    MTDB (1) << "successor <<" << from << ">> thinks somebody (" << p->GetPredAddress () << "," << 
	p->GetPredRange () << ") else is his pred!" << endl;

    // oops; some new guy
    NodeRange *succs_preds_range = &p->GetPredRange();
    const NodeRange *curr_succs_range = &m_Hub->GetSuccessor()->GetRange();

    // new guy closer to us than the successor? set this guy as our successor!
    if (IsBetweenRightInclusive (succs_preds_range->GetMin (), m_Hub->GetRange ()->GetMin (), curr_succs_range->GetMin ())) {
	MTINFO << " (" << m_Scheduler->TimeNow() << ") info from " << from << " setting new guy " << m_Hub->GetSuccessor () << " as our successor - notifying!" << endl;

	m_Hub->MergeSuccessor (p->GetPredAddress (), p->GetPredRange ());

	if (!g_Preferences.nosuccdebug) {
	    m_Hub->PrintSuccessorList ();
	}

	MsgNotifySuccessor *ns = new MsgNotifySuccessor (m_Hub->GetID(), m_Address, *m_Hub->GetRange());
	m_Network->SendMessage (ns, (IPEndPoint *) &m_Hub->GetSuccessor ()->GetAddress(), PROTO_UDP);
	delete ns;
    }
    else {
	MWARN << "successor is confused; thinks some random dude is his predecessor - notifying!" << endl;
	// our successor looks confused; tell him that i am the predecessor
	MsgNotifySuccessor *ns = new MsgNotifySuccessor (m_Hub->GetID(), m_Address, *m_Hub->GetRange());
	m_Network->SendMessage (ns, from, Parameters::TransportProto);
	delete ns;
    }
}

void LinkMaintainer::HandleSuccList(IPEndPoint *from, MsgSuccessorList *slmsg)
{	
    Peer *cursucc = m_Hub->GetSuccessor ();
    MTDB(1) << "received succ list from " << *from << " cursucc=" << cursucc << endl;

    if (!cursucc) {
	MTWARN << " [BENIGN] received succ list but have no successor!" << endl;
	return;
    }

    if (cursucc->GetAddress () != *from) {
	MTWARN << " Huh? received successor list from somebody who's no longer my successor!" << endl;
	return;
    }

    PeerInfoList *newlist = slmsg->GetPeerInfoList();
    ASSERT(newlist->size() >= 1);

    PeerInfo first = newlist->front();
    ASSERT(first.GetAddress() == cursucc->GetAddress());

    MDB (20) << "------ new succlist from " << *from << " cursucc=" << cursucc << " -----" << endl;
    for (PeerInfoListIter it = newlist->begin(); it != newlist->end(); it++)
    {
	PeerInfo inf = *it;
	DB_DO (20) { cerr << " " << inf.addr.GetPort () << "," << inf.range << " "; }
    }
    MDB (20) << "------------------" << endl;
    m_Hub->MergeSuccessorList(newlist);	
}

#define VALUE_EPSILON 10
/**
 * check if @lesser is adjacent to @larger assuming
 * a wrapped circle. allow a small gap for equality
 *
 * XXX DOES NOT USE EPSILON YET.
 **/
bool IsAdjacent (const Value& lesser, const Value& larger, const Value& absmin, const Value& absmax)
{	
    if (lesser == absmax) 
	return larger == absmin;
    return lesser == larger;
}

void LinkMaintainer::HandleNotifySuccessor(IPEndPoint *from, MsgNotifySuccessor *ns)
{
    if (m_Hub->GetStatus() != ST_JOINED)
	return;

#if 0 
    // -- unnecessary - Ashwin [02/25/2005]
    // 
    /// XXX: make sure the pred ends up repeating his notify succ request. 
    /// as long as we reply to get_pred (), that should work...
    if (m_Hub->GetSuccessor() == NULL) {
	MWARN << "received notify successor while my own successor is null!" << endl;
	return;
    }
#endif

    MDB (1) << "::::: received notify successor from " << *from << endl;

    Peer newpred (*from, ns->GetRange(), m_MercuryNode, m_Hub);
    Peer *curpred = m_Hub->GetPredecessor();

    if (!curpred) {
	// Install this guy as the new predecessor
	m_Hub->SetPredecessor (*from, ns->GetRange ());
	return;
    }

    // If the predecessor matches, treat this like a heartbeat
    if (curpred && curpred->GetAddress() == *from) {
	curpred->SetRange(ns->GetRange());
	return;
    }

    // check new pred's range: does he look closer to use than our 
    // current predecessor? if so, install him else return!

    NodeRange npr = ns->GetRange ();
    NodeRange cpr = curpred->GetRange ();
    NodeRange myr = *m_Hub->GetRange ();

    bool closer = false;

    // if max's are equal, check for min, else check for max
    if ((RMAX(npr) == RMAX(cpr) && IsBetween (RMIN(npr), RMIN(cpr), RMIN(myr))) 
	|| IsBetween (RMAX(npr), RMAX(cpr), RMAX(myr))) 
    {
	closer = true;
    }

    if (!closer)
    {
	MWARN << "notify from=" << from << " new pred=" << newpred << " NOT closer." 
	      << " curpred=" << curpred << 
	    " me=" << m_Hub->GetRange () << endl;
	return;
    }

    // Install this guy as the new predecessor
    m_Hub->SetPredecessor (*from, ns->GetRange ());

    // check for a special case
    Peer *cursucc = m_Hub->GetSuccessor ();
    if (cursucc && cursucc->GetAddress () == *from) {
	m_Hub->SetSuccessor (*from, ns->GetRange ());
    }
}

extern bool loopingMessage;

void LinkMaintainer::HandleNeighborRequest (IPEndPoint *from, MsgNeighborRequest *req)
{
    if (m_Hub->GetStatus() != ST_JOINED)
	return;

    const Value& dest_val = req->GetValue ();
    Constraint c (m_Hub->GetID (), dest_val, dest_val);
    bool left, center, right;

    c.GetRouteDirections (*m_Hub->GetRange (), left, center, right, m_Hub->GetRange ()->GetMax () == m_Hub->GetAbsMax ());
    MTDB (10) << " received nbrreq " << m_Hub->GetRange () << " req=" << req << endl;

    if (center) {
	const IPEndPoint& requestor = req->GetCreator ();
	MDB (5) << "received long pointer neighbor request from " << requestor << endl;

	MsgNeighborResponse *resp = new MsgNeighborResponse (m_Hub->GetID (), m_Address, *m_Hub->GetRange(), req->GetEpoch(), req->GetNonce ());

	/// how to garbage collect this dude? must register pings
	/// for this guy

	m_Hub->AddReverseLongNeighbor (requestor, RANGE_NONE);

	m_Network->SendMessage(resp, (IPEndPoint *) &requestor, Parameters::TransportProto);
	delete resp;

	return;
    }

    // send to avoid loopiness 
    MsgLivenessPong *pong = new MsgLivenessPong (m_Hub->GetID (), m_Address, *m_Hub->GetRange (), 0xff);
    m_Network->SendMessage (pong, from, Parameters::TransportProto);
    delete pong;

    // check for loopiness
    DB_DO(10) { 
	if (req->hopCount >= Parameters::MaxMessageTTL - 5) { 
	    loopingMessage = true;
	    m_Hub->PrintSortedPeers ();
	}
    }
    if (req->hopCount >= Parameters::MaxMessageTTL) { 
	MTWARN << "loopy Neighbor request from " << from << " with dest val=" << dest_val << "(" << req->hopCount<< "). THROWING. " << endl;
	return;	
    }

    const IPEndPoint *next_hop = m_Hub->GetPubsubRouter ()->ComputeNextHop (dest_val);
    if (next_hop == NULL) {
	MTWARN << "nobody to route to for long neighbor request?!" << endl;
	return;
    }

    m_Network->SendMessage (req, (IPEndPoint *) next_hop, Parameters::TransportProto);
}

Value LinkMaintainer::GenerateHarmonicValue (int nodeCount)
{
    int dist = (int) exp(G_GetRandom() * log((float) (nodeCount - 1)));
    Value dest_val = 0;

    if (!m_Hub->m_HistogramMaintainer->GetValueAtDistance(dist, dest_val)) {
	MWARN << "HistogramMaintainer::GetValueAtDistance failed! Should this really happen?" << endl;
	return VALUE_NONE;
    }

    Constraint c (m_Hub->GetID (), dest_val, dest_val);
    bool left, center, right;

    c.GetRouteDirections (*m_Hub->GetRange (), left, center, right, m_Hub->GetRange ()->GetMax () == m_Hub->GetAbsMax ());

    if (center) {
	MDB(10) << "the dest_val falls in my range... so not sending a nbr_req" << endl;
	return VALUE_NONE;
    }	
    return dest_val;
}

void NeighborRequestTimer::OnTimeout () {
    // ok; looks like our request didn't get through! 
    // send another one!
#if 0
    {
	MercuryNode *m_MercuryNode = m_LM->m_MercuryNode;
	MWARN << " neighbor request TIMEOUT!! (time=" << m_LM->m_Scheduler->TimeNow () << ")" << endl;
    }
#endif
    m_LM->RepairPointer (m_Nonce, -1);
}

void LinkMaintainer::RepairPointer (uint32 oldnonce, int nodeCount)
{
    // -1 when it is called from the timer;
    if (nodeCount == -1) {
	m_NRTimers.erase (oldnonce);
	nodeCount = m_Hub->m_HistogramMaintainer->EstimateNodeCount();
    }

    // try to generate a sane value; i am not sure making many trials
    // here is a good idea.

    int attempts = 0;
    Value dest_val = VALUE_NONE;
    while (attempts < 3 && ((dest_val = GenerateHarmonicValue (nodeCount)) == VALUE_NONE))
	attempts++;

    if (attempts >= 3) {  // forget it 
	MWARN << " forgetting it! nodeCount=" << nodeCount << endl;
	return;
    }

    uint32 nonce = SendNeighborRequest (dest_val);
    MDB (15) << " repairing pointer: nonce=" << merc_va("%0x", nonce) << " destval=" << dest_val << endl;

    // associate a timer with this request.
    ref<NeighborRequestTimer> nrt = new refcounted<NeighborRequestTimer> (this, nonce);
    m_NRTimers.insert (NRTMap::value_type (nonce, nrt));

    m_Scheduler->RaiseEvent (nrt, m_Address, Parameters::LongNeighborResponseTimeout);
}

uint32 LinkMaintainer::SendNeighborRequest (Value& dest_val)
{
    uint32 nonce = CreateNonce ();
    const IPEndPoint *next_hop = m_Hub->GetPubsubRouter ()->ComputeNextHop (dest_val);
    MsgNeighborRequest *req;

    if (next_hop == NULL) {
	MWARN << "nobody to route to for long neighbor request?!" << endl;
	goto done;
    }

    req = new MsgNeighborRequest (m_Hub->GetID (), m_Address, m_Address, dest_val, m_Epoch, nonce);
    m_Network->SendMessage (req, (IPEndPoint *) next_hop, Parameters::TransportProto);
    delete req;

 done:
    return nonce;
}

void LinkMaintainer::RepairLongPointers()
{
    MemberHub *hub = m_Hub;
    HistogramMaintainer *hmaintainer = m_Hub->m_HistogramMaintainer;

    int nodeCount = hmaintainer->EstimateNodeCount();
    int n_long_pointers = (int) (log((float) nodeCount)/log((float) 2.0));
    if (n_long_pointers < 3) {
	MDB(10) << "Node count too small for long pointers... " << endl;
	return;
    }

    m_Epoch++;

    // get rid of all timers for the neighbor requests for this epoch!
    for (NRTMapIter it = m_NRTimers.begin (); it != m_NRTimers.end (); it++) {
	ref<NeighborRequestTimer> nrt = it->second;
	nrt->Cancel ();
    }
    m_NRTimers.clear (); 

    // ok, we gonna move on in life; do something for the old timer folks! i.e., kick them out. :-)
    m_Scheduler->RaiseEvent (new refcounted<KickOldPeersTimer>(m_Hub, Parameters::KickOldPeersTimeout),
			     m_Address, Parameters::KickOldPeersTimeout );

    MDB (10) << merc_va("Gonna setup %d long pointers for hub %d", n_long_pointers, hub->GetID()) << endl;

    for (int i = 0; i < n_long_pointers; i++) {
	RepairPointer (0, nodeCount);
    }
}

void LinkMaintainer::HandleNeighborResponse(IPEndPoint *from, MsgNeighborResponse *resp)
{
    MDB (20) << "received long pointer neighbor RESPONSE from " << from << endl;

    NRTMapIter it = m_NRTimers.find (resp->GetNonce ());
    if (it != m_NRTimers.end ()) {
	it->second->Cancel ();
	m_NRTimers.erase (it);
    }

    if (m_Epoch != resp->GetEpoch()) {
	MDB (20) << "-- response to old request; we have moved on in life... " 
		 << merc_va("old_epoch:new_epoch = %d:%d", resp->GetEpoch(), m_Epoch) << endl;

	return;
    }

    if (m_Hub->LookupLongNeighbor (*from) != NULL) {
	MDB (20) << "received dupe long ptr neighbor response. ignoring... " << endl;
	return;
    }

    m_Hub->AddLongNeighbor (*from, resp->GetRange ());
}

#include <util/stacktrace.h>

void LinkMaintainer::OnPeerDeath (ref<Peer> p)
{
    // debug
    if (!g_Preferences.nosuccdebug) {
	MTINFO << " Some peer died ... " << endl;
	m_Hub->PrintSuccessorList ();
    }

    Peer *succ = m_Hub->GetSuccessor ();
    if (!succ) {
	MTWARN << "all successors died. stopping now." << endl;
	m_MercuryNode->Stop ();
	/// XXX we can "rejoin" also, but we will see about that
	_exit (2);
    }

    if (p->IsLongNeighbor ())
    {
	// a long pointer is dead. when do we repair?
	// we do it right away. but this may not be good
	// in a congestion control sense.
	//
	// perhaps, Jinyang's Accordion like protocol
	// can be used here.

	MTDB (20) << " repairing long pointer " << endl;
	RepairPointer (0x0, -1);
    }
}

////////////////////// JoinRequestTimer /////////////////////////////////
//
JoinRequestTimer::JoinRequestTimer(MemberHub *hub, IPEndPoint repaddr, int timeout, bool lj) 
    : Timer(timeout), m_RepAddress (repaddr), m_LeaveJoin (lj)
{
    m_Hub = hub;
    m_Timeout = Parameters::JoinRequestTimeout;
}

void JoinRequestTimer::OnTimeout() 
{
    //	DB(1) << "join request timer for hub: " << (int) m_Hub->GetID() << " firing at " << m_Hub->GetScheduler ()->TimeNow () << endl;

    if (m_NTimeouts++ > Parameters::MaxJoinAttempts) {
	WARN << "** join timeout **" << endl;    
	Debug::die ("Could not join hub %s [exceeded %d attempts] possible successor was [%s] \n", 
		    m_Hub->GetName().c_str(), Parameters::MaxJoinAttempts, m_Hub->GetBootstrapRep ().ToString ());
    }

    m_Hub->ClearPredecessor ();

    // send a long neighbor pointer request. 
    // using this node as the starting point.

    MsgJoinRequest *join_req = new MsgJoinRequest(m_Hub->GetID(), m_Hub->GetAddress ()
#ifdef LOADBAL_TEST
						  , m_LeaveJoin
#endif
	);

    m_Hub->GetNetwork()->SendMessage(join_req, &m_RepAddress, PROTO_TCP);
    delete join_req;

    // exponential back-off! 1.2 ** maxjoinattempts = 38 => ~7.5 seconds wait in the worst case
    m_Timeout = (u_long) (m_Timeout * 1.2);
    _RescheduleTimer (m_Timeout);
}

/////////////////////////////   KickOldPeersTimer ////////////////////////
//
KickOldPeersTimer::KickOldPeersTimer(MemberHub *hub, int timeout)
    : Timer(timeout)
{
    m_Hub    = hub;
}

void KickOldPeersTimer::OnTimeout()
{
}

//////////////////////////////////////////////////////////////////////////////////
/// SuccListMaintenanceTimer
///
SuccListMaintenanceTimer::SuccListMaintenanceTimer(LinkMaintainer *lm, int timeout)
    : Timer(timeout)
{
    m_LinkMaintainer = lm;
}

void SuccListMaintenanceTimer::OnTimeout()
{
    m_LinkMaintainer->DoSuccListMaintenance();
    _RescheduleTimer(Parameters::SuccessorMaintenanceTimeout);
}

void LinkMaintainer::SendLeaveNotification (IPEndPoint addr, NodeRange range, double currentload, IPEndPoint newsucc)
{
    list<Interest *> subs;
    list<MsgPublication *> trigs;

    m_Hub->GetPubsubRouter ()->GetMatchingSubscriptions (range, subs);
    m_Hub->GetPubsubRouter ()->GetMatchingTriggers (range, trigs);

    INFO << " sending leave notification to (" << addr << ";" << range << ") newsuc=" << newsucc << endl;
    MsgLeaveNotification *ln = new MsgLeaveNotification (m_Hub->GetID (), m_Address, range, currentload);
    for (list<Interest *>::iterator it = subs.begin (); it != subs.end (); ++it) 
	ln->AddSubscription (*it);
    for (list<MsgPublication *>::iterator it = trigs.begin (); it != trigs.end (); ++it) 
	ln->AddTrigger (*it);

    ASSERT (g_Preferences.do_loadbal);
    LoadBalEntry ent (LoadBalEntry::LEAVE, (int) subs.size () + trigs.size (), (int) ln->GetLength (), newsucc);
    LOG(LoadBalanceLog, ent);

    m_Network->SendMessage (ln, &addr, PROTO_TCP);
    delete ln;
}

class LNContinuation : public SchedulerEvent {
    LinkMaintainer *m_LinkMaintainer;
    IPEndPoint m_NewSucc;
    IPEndPoint m_Address; 
    NodeRange  m_Range;
    double     m_CurrentLoad;
public:
    LNContinuation (LinkMaintainer *lm, IPEndPoint newsucc, IPEndPoint addr, NodeRange range, double currentload) :
	m_LinkMaintainer (lm), m_NewSucc (newsucc), m_Address (addr), m_Range (range), m_CurrentLoad (currentload) {}

    void Execute (Node& node, TimeVal& timenow) {
	m_LinkMaintainer->SendLeaveNotification (m_Address, m_Range, m_CurrentLoad, m_NewSucc);
	m_LinkMaintainer->EndLeave (m_NewSucc);
    }
};

/** 
 * Send messages to predecessor and successor about our departure
 * each message contains (a) new range, (b) triggerlist and (c)
 * sublist
 *
 * XXX: I think it's quite possible that data (subs+trigs) be lost if
 * succ/pred die in between the leave-join operations. but, we assume
 * that replication mechanisms should exist anyway for handling their
 * loss - those should take care of this event as well. - Ashwin
 * [04/29/2005]
 *
 **/

void LinkMaintainer::DoLeaveJoin (IPEndPoint *newsucc, double currentload)
{
    m_MercuryNode->GetApplication ()->LeaveBegin ();

    Value min = m_Hub->GetRange ()->GetMin ();
    Value max = m_Hub->GetRange ()->GetMax ();
    Value span = m_Hub->GetRangeSpan ();

    span /= 2;
    Value mid = min;
    mid += span;

    if (mid > m_Hub->GetAbsMax ()) {
	mid -= m_Hub->GetAbsMax ();
	mid += m_Hub->GetAbsMin ();
    }

    Peer *pred = m_Hub->GetPredecessor (), *succ = m_Hub->GetSuccessor ();
    NodeRange rpred (m_Hub->GetID (), pred->GetRange ().GetMin (), mid);
    NodeRange rsucc (m_Hub->GetID (), mid, succ->GetRange ().GetMax ());

    IPEndPoint predaddr = pred->GetAddress ();
    IPEndPoint succaddr = succ->GetAddress ();

    m_Hub->PrepareLeave ();

    // break link to successor first. this makes sure weird race 
    // conditions do not occur when we leave and our current pred
    // asks our current succ and gets US as its successor via 
    // MsgPred!! - Ashwin [07/18/2005]

    SendLeaveNotification (succaddr, rsucc, currentload / 2.0, *newsucc);

    // XXX really what should happen is that the receiving 
    // predecessor should SUSPEND its succlist maintenance 
    // processor for a bit after getting leave notification
    m_Scheduler->RaiseEvent (new refcounted<LNContinuation> (this, *newsucc, predaddr, rpred, currentload / 2.0), m_Address, Parameters::PeerPingInterval);
}

void LinkMaintainer::EndLeave (IPEndPoint newsucc)
{
    // now re-join at our new successor.

    m_MercuryNode->GetApplication ()->LeaveEnd ();

    // Start () will schedule the timer ...
    MTDB (-10) << " will soon join near " << newsucc << endl;
    m_JoinRequestTimer = new refcounted<JoinRequestTimer> (m_Hub, newsucc, 0, true /* this is a leave-join related join request */);
    Start ();      
}

class PurgeDataContinuation : public SchedulerEvent {
    MemberHub *m_Hub;
public:
    PurgeDataContinuation (MemberHub *hub) : m_Hub (hub) {}

    void Execute (Node& node, TimeVal& timenow) {
	m_Hub->GetPubsubRouter ()->PurgeOutofRangeData ();
    }
};

extern bool specialDebug;

void LinkMaintainer::HandleLeaveNotification (IPEndPoint *from, MsgLeaveNotification *lnmsg)
{
    Peer *p = m_Hub->LookupPeer (*from);
    Peer *pred = m_Hub->GetPredecessor ();
    Peer *succ = m_Hub->GetSuccessor ();

    MTINFO << "(" << m_Scheduler->TimeNow () << ") received *** leave notification *** from " << from << endl;

    if (!pred || !succ) {
	MTWARN << " received leave notification and i have no pred and succ " << endl;
	return;
    }

#if 0
    if (!p || (*from != pred->GetAddress () && *from != succ->GetAddress ())) {
	MTWARN << "received leave notification from unknown neighbor" << endl;
	return;
    }
#endif

    // must disable leave-joins for a bit
    if (g_Preferences.do_loadbal) 
	m_Hub->GetLoadBalancer ()->SetRingUnstable ((int) (1.5 * Parameters::SuccessorMaintenanceTimeout));

    m_Hub->GetPubsubRouter ()->UpdateRangeLoad (lnmsg->GetAssignedRange ());
    m_Hub->SetRange (lnmsg->GetAssignedRange ());

    for (list<Interest *>::iterator it = lnmsg->s_begin (); it != lnmsg->s_end (); ++it)
	m_Hub->GetPubsubRouter ()->AddNewInterest (*it);

    for (list<MsgPublication *>::iterator it = lnmsg->t_begin (); it != lnmsg->t_end (); ++it)
	m_Hub->GetPubsubRouter ()->AddNewTrigger (*it);

#ifdef LOADBAL_TEST
    if (g_Preferences.do_loadbal) {
	double myload = m_Hub->GetLoadBalancer ()->GetMyLoad () + lnmsg->GetAdditionalLoad ();    
	LoadMetric *lm = new LoadMetric (myload);
	m_Hub->GetLoadSampler ()->SetLoad (lm);
	delete lm;
    }
#endif

    // the ring reconfigures here...
    m_Hub->RemovePeer (*from);
}

void LinkMaintainer::HandleLinkBreak (IPEndPoint *from, MsgLinkBreak *lmsg)
{
    Peer *p = m_Hub->LookupPeer (*from);
    if (!p) {
	MWARN << "[BENIGN] received link break message from phantom " << from << endl;
	return;
    }

    if (p->IsLongNeighbor ()) {
	// a long pointer is dead. when do we repair?
	// we do it right away. but this may not be good
	// in a congestion control sense.
	//
	// perhaps, Jinyang's Accordion like protocol
	// can be used here.
	RepairPointer (0x0, -1);
    }
    m_Hub->RemovePeer (*from);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
