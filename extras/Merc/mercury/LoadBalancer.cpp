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

// $Id: LoadBalancer.cpp 2472 2005-11-13 01:34:54Z ashu $

#include <mercury/Hub.h>
#include <mercury/MercuryNode.h>
#include <mercury/Scheduler.h>
#include <mercury/Timer.h>
#include <mercury/Message.h>
#include <mercury/Sampling.h>
#include <mercury/MetricInfo.h>
#include <mercury/LoadBalancer.h>
#include <mercury/PubsubRouter.h>
#include <mercury/Parameters.h>
#include <mercury/RoutingLogs.h>
#include <mercury/LinkMaintainer.h>

#define METRIC(sample) (dynamic_cast<const LoadMetric *>((sample)->GetMetric ()))
#define LOAD(sample)  (METRIC(sample)->GetLoad ())

#define PREDADDR()  ((IPEndPoint *) &m_Hub->GetPredecessor()->GetAddress())
#define SUCCADDR()  ((IPEndPoint *) &m_Hub->GetSuccessor()->GetAddress())

#define APP_LEAVE_JOIN_OK() (m_MercuryNode->GetApplication ()->IsLeaveJoinOK ())

static double LoadEps (Sample *sample) {
    double l = LOAD(sample);
    if (l < LoadBalancer::EPSILON)
	l = LoadBalancer::EPSILON;
    return l;
}

void MakeStableTimer::OnTimeout () 
{
    m_LB->m_RingState = LoadBalancer::STABLE;
}

void LoadBalanceTimer::OnTimeout ()
{
    m_LB->CheckLoadBalance ();
    _RescheduleTimer (m_Timeout);
}

LoadBalancer::LoadBalancer (MemberHub *hub) 
    : m_Hub (hub),
      m_MercuryNode (m_Hub->GetMercuryNode()), m_Network (m_Hub->GetNetwork ()),
      m_Address (m_Hub->GetAddress ()), m_Scheduler (m_Hub->GetScheduler ()),
      m_LoadBalanceTimer (new refcounted<LoadBalanceTimer> (this, Parameters::CheckLoadBalanceInterval)),
      m_MakeStableTimer (NULL),
      m_LeaveJoinLBReqTracker (NULL),
      m_LB_Requestor (SID_NONE)
{
    m_LocalLoad = EPSILON;

    MsgType msgs[] = { 
	MSG_LOCAL_LB_REQUEST, MSG_LOCAL_LB_RESPONSE, MSG_LEAVEJOIN_LB_REQUEST, MSG_LEAVEJOIN_DENIAL,
	MSG_LCHECK_REQUEST, MSG_LCHECK_RESPONSE
    };

    for (uint32 i = 0; i < sizeof(msgs) / sizeof(MsgType); i++)
	m_MercuryNode->RegisterMessageHandler(msgs[i], this);
}

void LoadBalancer::Start ()
{
    m_State = DOING_NOTHING;
    m_RingState = STABLE;

    // make a new pointer! otherwise, the "cancelled" flag is set, and it will 
    // never be invoked again
    m_LoadBalanceTimer = new refcounted<LoadBalanceTimer> (this, Parameters::CheckLoadBalanceInterval);
    m_Scheduler->RaiseEvent (m_LoadBalanceTimer, m_Address, 0);
}

void LoadBalancer::Pause ()
{
    m_LoadBalanceTimer->Cancel ();
}

void LoadBalancer::CheckLoadBalance ()
{
    MTDB (-5) << " checking load balance " << endl;
    if (!m_Hub->GetSuccessor () || !m_Hub->GetPredecessor () || !m_Hub->GetRange () || !m_Hub->GetLoadSampler ())
	return;

    MTDB (5) << " doing set local load  " << endl;
    // check if pred, succ have reported loads
    if (!SetLocalLoad ())
	return;

    MTDB (-5) << " doing local load balance " << endl;
    // check for local load balance
    if (DoLocalLoadBalance ())
	return;

    if (!AmHeavy ())
	return;

    DB_DO (-5) {
	MTDB (-5) << "<<HEAVY LOAD>> "; print_vars (cerr, m_LocalLoad, GetAverageLoad());
    }

    // wait for our immed neighbors if we think they are light enough
    float pred_load, succ_load;
    GetPredSuccLoad (&pred_load, &succ_load);

    if (pred_load <= EPSILON || succ_load <= EPSILON) {
	MTDB (-5) << "waiting for neighbors to send a local request ***" << endl;
	return;
    }

    if (pred_load * g_Preferences.loadbal_delta < m_LocalLoad || 
	succ_load * g_Preferences.loadbal_delta < m_LocalLoad) {
	MTDB (-5) << "waiting for neighbors to send a local request ***" << endl;
	return;
    }

    if (APP_LEAVE_JOIN_OK())
	DoRemoteLoadBalance ();
    else {
	MTDB (1) << "* application did not permit leave join load balance " << endl;
    }
}

void LoadBalancer::ProcessMessage (IPEndPoint *from, Message *msg)
{
    if (m_Hub->GetID () != msg->hubID)
	return;
    if (m_Hub->GetRange () == NULL)
	return;
    if (m_Hub->GetSuccessor () == NULL)
	return;

    MsgType t = msg->GetType ();
    if (t == MSG_LOCAL_LB_REQUEST)
	HandleLocalLBRequest (from, (MsgLocalLBRequest *) msg);
    else if (t == MSG_LOCAL_LB_RESPONSE)
	HandleLocalLBResponse (from, (MsgLocalLBResponse *) msg);
    else if (t == MSG_LEAVEJOIN_LB_REQUEST)
	HandleLeaveJoinLBRequest (from, (MsgLeaveJoinLBRequest *) msg);
    else if (t == MSG_LEAVEJOIN_DENIAL)
	HandleLeaveJoinDenial (from, (MsgLeaveJoinDenial *) msg);
    else if (t == MSG_LCHECK_REQUEST)
	HandleLeaveCheckRequest (from, (MsgLeaveCheckRequest *) msg);
    else if (t == MSG_LCHECK_RESPONSE)
	HandleLeaveCheckResponse (from, (MsgLeaveCheckResponse *) msg);
    else 
	MWARN << " loadbalancer received some idiotic message " << msg->TypeString () << endl;
}

class ResetLocalStateContinuation : public SchedulerEvent {
    LoadBalancer *m_LB;
public:
    ResetLocalStateContinuation (LoadBalancer *lb) : m_LB (lb) {}

    void Execute (Node& node, TimeVal& timenow) {
	// cerr << node.GetAddress () << ": executing continuation at " << timenow << endl;
	m_LB->m_State = LoadBalancer::DOING_NOTHING;
	m_LB->m_SentLocalReqTo = NULL;
    }
};

void LoadBalancer::GetPredSuccLoad (float *pred, float *succ) 
{
    *pred = *succ = EPSILON;

    MetricInfo *metric_info = m_Hub->GetRegisteredMetric (m_Hub->GetLoadSampler ());
    Sample *predLoad = metric_info->GetNeighborHoodEstimate (-1);
    Sample *succLoad = metric_info->GetNeighborHoodEstimate (1);

    if (predLoad) 
	*pred = LoadEps (predLoad);
    if (succLoad)
	*succ = LoadEps (succLoad);
}

/**
 * returns true if either:
 *  - load balancing (local or leave-join) is going on
 *  - local load is imbalanced (in which case, it also starts 
 *            performing local load balance.)
 **/
bool LoadBalancer::DoLocalLoadBalance ()
{
    double my = GetMyLoad ();

    if (IsLoadBalanceOngoing ())
	return true;

    float pred, succ;
    GetPredSuccLoad (&pred, &succ);

    if (my <= EPSILON)
	return false;

    Peer *sendto = NULL;

    // lighter node starts a local load balance
    if ((succ / my) > g_Preferences.loadbal_delta) 
	sendto = m_Hub->GetSuccessor ();
    else if ((pred / my) > g_Preferences.loadbal_delta)
	sendto = m_Hub->GetPredecessor ();

    DB_DO(10) {
	MTDB (10) << "";
	print_vars (cerr, my, succ, pred, sendto);
    }
    if (sendto == NULL) {
	MTDB (10) << "[local load balanced] " << endl;
	return false;
    }

    MsgLocalLBRequest *lbrq = new MsgLocalLBRequest (m_Hub->GetID (), m_Address, *m_Hub->GetRange (), my);    
    m_Network->SendMessage (lbrq, (IPEndPoint *) &sendto->GetAddress (), Parameters::TransportProto);
    delete lbrq;

    MTDB (-10) << " sending local lb-request to " << sendto->GetAddress () << endl;
    m_State = SENT_REQ_LOCAL_NBR;
    m_SentLocalReqTo = new refcounted<IPEndPoint> (sendto->GetAddress ());

    // make sure we re-set our state after a bit
    m_Scheduler->RaiseEvent (new refcounted<ResetLocalStateContinuation> (this), m_Address, Parameters::TCPFailureTimeout);
    return true;
}

struct less_sample_t {
    bool operator () (const Sample *sa, const Sample *sb) const {
	return LOAD (sa) < LOAD (sb);
    }
};

/**
 * requiring delta * l < avg is too restrictive. such nodes may
 * not exist despite load imbalance. 
 *
 * instead, we use Markov's inequality, and do load exchanges 
 * with nodes such that delta * l < myload
 *
 **/
void LoadBalancer::GetLightSamples (vector<Sample *>& samples)
{
    // double avg = GetAverageLoad ();
    // if (avg < EPSILON)
    //     return;

    double my = GetMyLoad ();

    vector<Sample *> all_samples;
    m_Hub->GetSamples (m_Hub->GetLoadSampler (), &all_samples);

    for (vector<Sample *>::iterator it = all_samples.begin (); it != all_samples.end (); ++it)
    {
	Sample *s = *it;
	double ls = LOAD (s);

	if (g_Preferences.loadbal_delta * ls > my)
	    continue;

	samples.push_back (s);
    }

    sort (samples.begin (), samples.end (), less_sample_t ());
    return;
}

class LeaveJoinLBReqTracker : public Timer {
    LoadBalancer *lb;
    IPEndPoint    candidate;
public:
    LeaveJoinLBReqTracker (LoadBalancer *lb, const IPEndPoint& candidate) : 
	Timer (0), lb (lb), candidate (candidate) {}

    void OnTimeout () { 
	if (lb->GetState () == LoadBalancer::WAITING_FOR_LEAVE_JOIN_RESPONSE) 
	    lb->SetState (LoadBalancer::DOING_NOTHING);
	//	lb->CheckLoadBalance ();
    }    
};

// - Ashwin [06/21/2005] 
//
// Here is how the whole remote load balance thing should be
// operate.  Heavy load sends request for leave-join. starts a
// timer. when timer fires, it just resets it's state to
// "DOING_NOTHING" -- so the next load-bal loop iteration "does"
// something.
// 
// lighter nodes receive leave-join request (it does not do any
// check as to the "non-maliciousness" of the request). if it is
// doing sth or thinks it aint light, it sends a denial. on denial
// receipt, heavy node repeats process with some other node. 
//
// otherwise, light node leaves the place; it needs to gracefully
// transfer its state to its neighbors (if this is not required,
// then it can just leave and the fail-over mechanism will patch the
// ring; although pred will get more load then.) then light node
// sends a join request to heavy node and joins. 
//
// a successful join near heavy node resets the load-measuring
// counter to zero and so the heavier node does not re-request
// another leave-join as long as the load does not increase too much
// once again. this sort of builds in some amount of dampening into
// the system.
//
// NOTE: assumption(s) hidden here. By design, Leave-join requests
// always time-out. When they timeout, we reset our state to
// "doing_nothing". this means that the next iteration of
// DoLoadBalance () will work starting from scratch. However, if the
// leave-join request really succeeded, it would have reset our
// recent-load-in-window to zero so we would not be heavy anymore.
// on the other hand, if our request was denied (and denial didn't
// reach us), we expect that we get a better candidate by our
// sampling this time. TODO: find a better way to locate light nodes
// rather than just "pin-point" them using samples. (some local
// flooding might be suitable.)
// 

void LoadBalancer::DoRemoteLoadBalance ()
{
    if (IsLoadBalanceOngoing ()) {
	MTDB (-5) << " remote thing going on " << endl;
	return;
    }

    MTDB (-5) << " trying to do remote load balance " << endl;

    vector<Sample *> lightsamples;
    GetLightSamples (lightsamples);

    if (lightsamples.size () == 0) {
	MTDB (-10) <<  " HEAVY, but no light node known" << endl;
	return;
    }

    // choose the lightest for now
    IPEndPoint candidate = ChooseLightCandidate (lightsamples);

    MTDB (-10) << " sending remote-req to " << candidate << endl;
    MsgLeaveJoinLBRequest *req = new MsgLeaveJoinLBRequest (m_Hub->GetID (), m_Address, *m_Hub->GetRange (), GetMyLoad ());
    m_Network->SendMessage (req, (IPEndPoint *) &candidate, Parameters::TransportProto);        

    m_State = WAITING_FOR_LEAVE_JOIN_RESPONSE;

    if (m_LeaveJoinLBReqTracker != NULL) 
	m_LeaveJoinLBReqTracker->Cancel ();

    m_LeaveJoinLBReqTracker = new refcounted<LeaveJoinLBReqTracker> (this, candidate);
    m_Scheduler->RaiseEvent (m_LeaveJoinLBReqTracker, m_Address, Parameters::LeaveJoinResponseTimeout);
}

IPEndPoint LoadBalancer::ChooseLightCandidate (vector<Sample *>& lightsamples)
{
    int length = lightsamples.size ();
    int index = length;

    /*
      for (int i = 1; i < length; i++) { 
      // print_vars (cerr, i, LOAD(lightsamples[i]), LOAD(lightsamples[0]));
      if (fabs (LOAD (lightsamples[i]) - LOAD (lightsamples[0])) > EPSILON) {
      index = i;
      break;
      }
      }
    */

    /// select randomly from 0 -> index - 1
    Sample *s =  lightsamples[ (int) (drand48 () * index) ];
    MTDB (10) << "index=" << index << " chose light node=" << s->GetSender () << " with load=" << LOAD(s) << endl;
    IPEndPoint sender = s->GetSender ();

    MetricInfo *metric_info = m_Hub->GetRegisteredMetric (m_Hub->GetLoadSampler ());
    metric_info->RemoveSample (s->GetSender ());

    return sender;
}

/**
 * send another request, if we are still heavy 
 **/
void LoadBalancer::HandleLeaveJoinDenial (IPEndPoint *from, MsgLeaveJoinDenial *denial)
{
    MTDB (-5) << " DENIAL from " << from << endl;
    if (m_LeaveJoinLBReqTracker != NULL)
	m_LeaveJoinLBReqTracker->Cancel ();

    if (!AmHeavy ()) {
	m_State = DOING_NOTHING;
	return;
    }

    vector<Sample *> lightsamples;
    GetLightSamples (lightsamples);

    if (lightsamples.size () == 0) {
	MTDB (-10) <<  " HEAVY, but no light node known" << endl;
	return;
    }

    for (vector<Sample *>::iterator it = lightsamples.begin (); it != lightsamples.end (); ++it) {
	Sample *s = *it;

	if (s->GetSender () == *from) {
	    lightsamples.erase (it);
	    break;
	}
    }

    IPEndPoint candidate = ChooseLightCandidate (lightsamples);
    MsgLeaveJoinLBRequest *req = new MsgLeaveJoinLBRequest (m_Hub->GetID (), m_Address, *m_Hub->GetRange (), GetMyLoad ());
    m_Network->SendMessage (req, (IPEndPoint *) &candidate, PROTO_UDP);        

    m_LeaveJoinLBReqTracker = new refcounted<LeaveJoinLBReqTracker> (this, candidate);
    m_Scheduler->RaiseEvent (m_LeaveJoinLBReqTracker, m_Address, Parameters::LeaveJoinResponseTimeout);
}

void LoadBalancer::SetRingUnstable (int timeout)
{
    m_RingState = LoadBalancer::UNSTABLE;

    // after SuccessorMaintenanceTimeout, one should have 
    // the pred+succ pointers repaired, hopefully.

    if (m_MakeStableTimer != NULL) 
	m_MakeStableTimer->Cancel ();

    m_MakeStableTimer = new refcounted<MakeStableTimer> (this);
    m_Scheduler->RaiseEvent (m_MakeStableTimer, m_Address, timeout);
}

void LoadBalancer::SendDenial (IPEndPoint *sendto)
{
    MsgLeaveJoinDenial *denial = new MsgLeaveJoinDenial (m_Hub->GetID (), m_Address);
    m_Network->SendMessage (denial, sendto, Parameters::TransportProto);
    delete denial;
}

class LeaveCheckTimer : public Timer {
    LoadBalancer *m_LoadBalancer;
    IPEndPoint m_LB_Requestor;
public:
    LeaveCheckTimer (LoadBalancer *lb, IPEndPoint rtor) : Timer (0), m_LoadBalancer (lb), m_LB_Requestor (rtor) {}
    void OnTimeout () {
	m_LoadBalancer->LeaveCheckExpired (m_LB_Requestor);
    }
};

void LoadBalancer::LeaveCheckExpired (IPEndPoint lb_reqtor)
{
    // check if state got cleared!
    if (m_State != CHECKING_SUCC) 
	return; 

    MTDB (-10) << "denying leave-join request coz neighbors did not respond to leave-check " << endl;
    SendDenial (&lb_reqtor);
}

void LoadBalancer::HandleLeaveJoinLBRequest (IPEndPoint *from, MsgLeaveJoinLBRequest *ljr)
{
    if (IsLoadBalanceOngoing () || !APP_LEAVE_JOIN_OK()) {
	MTDB (-10) << "denying leave-join request coz busy " << endl;

	SendDenial (from);	
	return;
    }

    if (m_RingState == UNSTABLE || !m_Hub->GetPredecessor () || !m_Hub->GetSuccessor () || !m_Hub->GetRange () || !m_Hub->GetLoadSampler ()) {
	MTDB (-10) << "denying leave-join request coz ring is unstable " << endl;
	SendDenial (from);	
	return;
    }

    // see note near GetLightSamples ()  - Ashwin [06/26/2005]
    double my  = GetMyLoad ();

    if (g_Preferences.loadbal_delta * my > ljr->GetLoad ()) {
	DB_DO(-10) {
	    MTDB (-10) << "denying leave-join request from " << *from << " coz NOT relatively LIGHT" ; print_vars (cerr, my, ljr->GetLoad());
	}

	SendDenial (from);
	return;
    }

    double avg = GetAverageLoad (); 
    float pred, succ;
    GetPredSuccLoad (&pred, &succ);

    pred += my / 2.0;
    succ += my / 2.0;

    if (pred > avg * g_Preferences.loadbal_delta || succ > avg * g_Preferences.loadbal_delta) {
	MTDB (-10) << " denying leave-join request from " << *from << " coz neighbors are probably heavy; will wait" << endl;

	SendDenial (from);
	return;
    }

    // everything seems okay, now verify that my neighbors 
    // have not accepted a leave-join request in the meantime.

    MTDB (-10) << " sending leave-check-request to " << SUCCADDR() << endl;
    MsgLeaveCheckRequest lcr (m_Hub->GetID (), m_Address);
    m_Network->SendMessage (&lcr, SUCCADDR(), Parameters::TransportProto);

    m_State = CHECKING_SUCC;

    m_LB_Requestor = *from;
    m_EarlierLoad = my;
    m_Scheduler->RaiseEvent (new refcounted<LeaveCheckTimer> (this, *from), m_Address, Parameters::JoinRequestTimeout);

    // (2) on receving a response, check you had a request active, and make note. if you hear both
    // responses, cancel timer and perform LeaveJoin depending on the response. else, send denial. 
    // 
    // (3) wait for a certain time. if you dont hear from people, ignore the lb request, erase all
    // state. perhaps send a denial...
    // 
    // XXX: assume that the requestor is genuinely heavy. 

}

#define IS_PRED(from) (m_Hub->GetPredecessor ()->GetAddress () == *from)
#define IS_SUCC(from) (m_Hub->GetSuccessor ()->GetAddress () == *from)

void LoadBalancer::HandleLeaveCheckResponse (IPEndPoint *from, MsgLeaveCheckResponse *lcr)
{
    if (m_State != CHECKING_SUCC) {
	MWARN << "not expecting a leave check response" << endl;
	return;
    }

    ASSERT (m_LB_Requestor != SID_NONE);
    if (lcr->IsOK ()) {
	MINFO << " (" << m_Scheduler->TimeNow () << ") performing a leave-join towards " << m_LB_Requestor << endl;
	// OK now
	LoadMetric *lm = new LoadMetric (0.0);
	m_Hub->GetLoadSampler ()->SetLoad (lm);
	delete lm;

	m_Hub->GetLinkMaintainer ()->DoLeaveJoin (&m_LB_Requestor, m_EarlierLoad);

	m_State = STARTED_LEAVE_JOIN;
    }
    else {
	MTDB (-10) << "OOPS: our successor instructed us not to leave" << endl;
	SendDenial (&m_LB_Requestor);
	m_State = DOING_NOTHING;
    }

    m_LB_Requestor = SID_NONE;
}

void LoadBalancer::HandleLeaveCheckRequest (IPEndPoint *from, MsgLeaveCheckRequest *lcr)
{
    MTDB (-10) << " receive leave-check-request from " << from << endl;
    if (m_Hub->GetPredecessor () == NULL) {
	MWARN << "leave check request: predecessor does not exist?" << endl;
	return;
    }

    if (!IS_PRED(from)) { 
	MWARN << "leave check guy is NOT predecessor. HUH?" << endl;
	return;
    }

    MsgLeaveCheckResponse resp (m_Hub->GetID (), m_Address);
    if (IsLoadBalanceOngoing () || !APP_LEAVE_JOIN_OK() || m_RingState == UNSTABLE) {
	MTDB (-10) << " REFUSING NEIGHBOR TO LEAVE " << endl;
	resp.SetOK (false);
	m_Network->SendMessage (&resp, from, Parameters::TransportProto);
	return;
    }

    m_Network->SendMessage (&resp, from, Parameters::TransportProto);
    SetRingUnstable (Parameters::JoinRequestTimeout);
}

void LoadBalancer::HandleLocalLBRequest (IPEndPoint *from, MsgLocalLBRequest *llb)
{
    if (IsLoadBalanceOngoing ()) {
	DB_DO (-10) { MWARN << " received loadbal-req while performing load-bal! " << endl; }
	return;
    }

    if (!CheckLocalOK (from))
	return;

    /// XXX we must send response reliably!
    if (IsRequestDuplicate (from)) {	
	MTDB (-10) << "another request from this guy? reject it!" << endl;
	return;
    }

    double my = GetMyLoad ();
    double nbrload = llb->GetLoad ();
    if ((my / nbrload) < g_Preferences.loadbal_delta) {
	DB_DO (-10) { MWARN << "thanks for the offer, but load is not imbalanced" << endl; }
	return;
    }

    // ok: additional load to offload = (my - nbr) / 2
    // for now, we assume that my load is spread across uniformly
    // 
    // range to give to other guy: [(my - nbr) / (2 * my)] * myrange

    Peer *peer =m_Hub->LookupPeer (*from);
    Peer *succ = m_Hub->GetSuccessor ();
    ASSERT (peer != NULL);

    peer->SetRange (llb->GetRange ());

    double decr_fraction = ((my - nbrload) / (2 * my));

    DB_DO (5) {
	MTDB (-5) << "";
	print_vars (cerr, nbrload, my, decr_fraction, m_Hub->GetRange());
    }

    Value span_change = m_Hub->GetRangeSpan ();
    span_change.double_mul (decr_fraction);

    MTDB (-5) << "current span=" << m_Hub->GetRangeSpan () << " span-change=" << span_change << endl;

    Value curmin = m_Hub->GetRange ()->GetMin ();
    Value curmax = m_Hub->GetRange ()->GetMax ();

    NodeRange *peer_range = 0;

    if (peer == succ) {  // succ is lighter; so i decrease my max
	curmax -= span_change;
	if (curmax < m_Hub->GetAbsMin ()) { 
	    curmax += m_Hub->GetAbsMax ();
	    curmax -= m_Hub->GetAbsMin ();
	}

	peer_range = new NodeRange (m_Hub->GetID (), curmax, peer->GetRange ().GetMax ());
    }
    else {              // pred is lighter; so i increase my min.
	curmin += span_change;
	if (curmin > m_Hub->GetAbsMax ()) {
	    curmin -= m_Hub->GetAbsMax ();
	    curmin += m_Hub->GetAbsMin ();
	}

	peer_range = new NodeRange (m_Hub->GetID (), peer->GetRange ().GetMin (), curmin);
    }

    NodeRange my_new_range (m_Hub->GetID (), curmin, curmax);    
    MsgLocalLBResponse *lresp = new MsgLocalLBResponse (m_Hub->GetID (), m_Address, *peer_range, my_new_range, (my + nbrload) / 2);

    if (m_Network->SendMessage (lresp, from, PROTO_TCP) < 0) {
	MWARN << " could not send local load balance response to " << from << endl;
    }
    else {
	INFO << " (" << m_Scheduler->TimeNow () << ") range contracting due to nbr-load-adjustment, nbr=" << peer << endl;
	m_MercuryNode->GetApplication ()->RangeContracted (*m_Hub->GetRange (), my_new_range);

	m_Hub->GetPubsubRouter ()->UpdateRangeLoad (my_new_range);
	m_Hub->SetRange (&my_new_range);
	peer->SetRange (*peer_range);

	// ok: this guy has accepted a larger range, so send him triggers and subscriptions
	vector<int> ss = m_Hub->GetPubsubRouter ()->HandoverSubscriptions (from);
	vector<int> sp = m_Hub->GetPubsubRouter ()->HandoverTriggers (from);

	LoadBalEntry ent ((peer->IsSuccessor () ? LoadBalEntry::SUCCADJ : LoadBalEntry::PREDADJ),
			  ss[0] + sp[0], ss[1] + sp[1],
			  peer->GetAddress ());
	LOG(LoadBalanceLog, ent);

	/// THIS IS FOR DRIVING TESTS ONLY
	LoadMetric *lm = new LoadMetric ((my + nbrload) / 2);
	m_Hub->GetLoadSampler ()->SetLoad (lm);
	delete lm;
	/// END THIS IS FOR DRIVING TESTS ONLY

	TimeVal expiry = m_Scheduler->TimeNow () + Parameters::TCPFailureTimeout;
	m_LocalLBReqSIDs.insert (SIDTimeValMap::value_type (*from, expiry));
    }

    delete peer_range;
    delete lresp;
}

bool LoadBalancer::CheckLocalOK (IPEndPoint *from)
{
    if (m_Hub->GetPredecessor () == NULL) {
	DB_DO(-10) { MWARN << " no pred while local loadbal-op! requestor will try... " << endl; }
	return false;
    }

    Peer *p = m_Hub->LookupPeer (*from);
    if (!p) {
	DB_DO(-10) { 
	    MWARN << " received local loadbal-op from unknown fella!" << endl;
	}
	return false;
    }

    Peer *succ = m_Hub->GetSuccessor ();
    Peer *pred = m_Hub->GetPredecessor ();

    if (*from != pred->GetAddress () && *from != succ->GetAddress ()) {
	DB_DO(-10) {
	    MWARN << " received loadbal-op from random d00d!" << endl;
	}
	return false;
    }

    return true;
}

bool LoadBalancer::IsRequestDuplicate (IPEndPoint *from)
{
    bool found = false;
    TimeVal now = m_Scheduler->TimeNow ();

    // time-out as well as check for existence!
    for (SIDTimeValMapIter it = m_LocalLBReqSIDs.begin (); it != m_LocalLBReqSIDs.end (); /* it++ */)
    {
	if (it->second <= now) {
	    SIDTimeValMapIter oit (it);
	    ++oit;

	    m_LocalLBReqSIDs.erase (it);
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

void LoadBalancer::HandleLocalLBResponse (IPEndPoint *from, MsgLocalLBResponse *llb)
{
    if (!DoingLocalOps ()) {
	MWARN << "received local lb-response without requesting" << endl;
	return;
    }

    Peer *peer = m_Hub->LookupPeer (*from);
    if (!peer) {
	DB_DO(-10) {
	    MWARN << "local lb-response from unknown person" << endl;
	}
	return;
    }

    Peer *succ = m_Hub->GetSuccessor ();
    Peer *pred = m_Hub->GetPredecessor ();

    if (succ->GetAddress () != *from && pred->GetAddress () != *from) {
	DB_DO (-5) {
	    MWARN << "succ=" << succ->GetAddress () << "; pred=" << pred->GetAddress () << " local-lb-resp from=" << peer->GetAddress () << endl;
	}
	return;
    }

    if (m_SentLocalReqTo == NULL || *m_SentLocalReqTo != *from) {
	DB_DO(-10) { 
	    MWARN << "i did not send local lb-request to this guy (sent to:";
	    if (m_SentLocalReqTo != NULL) 
		cerr << *m_SentLocalReqTo;
	    else 
		cerr << "(null)";
	    cerr << ") from=" << *from << endl;
	}
	return;
    }

    peer->SetRange (llb->GetPeerNewRange ());

    // tell application we are changing the range
    INFO << " (" << m_Scheduler->TimeNow () << ") range expanding due to nbr-load-adjustment, nbr=" << peer << endl;
    m_MercuryNode->GetApplication ()->RangeExpanded (*m_Hub->GetRange (), llb->GetAssignedRange ());

    m_Hub->GetPubsubRouter ()->UpdateRangeLoad (llb->GetAssignedRange ());
    m_Hub->SetRange ((NodeRange *) &llb->GetAssignedRange ());

    /// THIS IS FOR DRIVING TESTS ONLY
    LoadMetric *lm = new LoadMetric (llb->GetNewLoad ());
    m_Hub->GetLoadSampler ()->SetLoad (lm);
    delete lm;
    /// END THIS IS FOR DRIVING TESTS ONLY

    m_SentLocalReqTo = NULL;
    m_State = DOING_NOTHING;
}

bool LoadBalancer::AmHeavy ()
{
    double avg = GetAverageLoad ();

    MTDB (-5) << " localload=" << m_LocalLoad << " avg=" << avg << endl;
    if (m_LocalLoad < EPSILON || avg < EPSILON)
	return false;

    if ((m_LocalLoad / avg) < g_Preferences.loadbal_delta) { 
	MTDB (10) << " NOT heavily loaded ... " << endl;
	return false;
    }

    return true;
}

bool LoadBalancer::SetLocalLoad ()
{
    m_LocalLoad = EPSILON;

    // check with our immediate neighbors.
    MetricInfo *metric_info = m_Hub->GetRegisteredMetric (m_Hub->GetLoadSampler ());

    Sample *predLoad = metric_info->GetNeighborHoodEstimate (-1);
    Sample *succLoad = metric_info->GetNeighborHoodEstimate (1);

    // if they havent told us about anything
    // chances are this is a very volatile nbrhood
    // so let things settle for a bit

    if (!predLoad || !succLoad) 
	return false;

    // m_LocalLoad = (LoadEps (predLoad) + LoadEps (succLoad) + GetMyLoad ()) / 3;

    m_LocalLoad = GetMyLoad ();
    MTDB (10) << "pred=" << LoadEps (predLoad) << " succ=" << LoadEps (succLoad) 
	      << " mine=" << GetMyLoad () << " localload=" << m_LocalLoad << endl;

    return true;
}

inline bool LoadBalancer::IsLoadBalanceOngoing ()
{
    return DoingLocalOps () || DoingRemoteOps ();	
}

inline bool LoadBalancer::DoingLocalOps ()
{
    return m_State == SENT_REQ_LOCAL_NBR;
}

inline bool LoadBalancer::DoingRemoteOps ()
{
    return m_State == STARTED_LEAVE_JOIN || m_State == WAITING_FOR_LEAVE_JOIN_RESPONSE || m_State == CHECKING_SUCC;
}

double LoadBalancer::GetAverageLoad ()
{
    vector<Sample *> loadsamples;
    m_Hub->GetSamples (m_Hub->GetLoadSampler (), &loadsamples);

    if (loadsamples.size () == 0) {	
	DB_DO (5) {
	    MTINFO << " lnbrs=" ;
	    m_Hub->PrintLongNeighborList ();
	}
	return EPSILON / 2;
    }

    double total = 0;
    for (int i = 0, len = loadsamples.size (); i < len; i++) {
	total += LOAD(loadsamples[0]);
    }

    total /= loadsamples.size ();
    return total;
}

double LoadBalancer::GetMyLoad ()
{
    if (m_Hub->GetLoadSampler () == NULL)
	return EPSILON;

    LoadMetric *mine = (LoadMetric *) m_Hub->GetLoadSampler ()->GetPointEstimate ();
    if (mine == NULL)
	return EPSILON;

    double ret = (double) mine->GetLoad ();
    if (ret < EPSILON)
	ret = EPSILON;

    delete mine;
    return ret;
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
