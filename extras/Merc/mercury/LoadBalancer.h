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
#ifndef __LOADBALANCING__H
#define __LOADBALANCING__H   

// $Id: LoadBalancer.h 2382 2005-11-03 22:54:59Z ashu $

#include <mercury/IPEndPoint.h>
#include <mercury/ID.h>
#include <vector>

class MemberHub;
class MercuryNode;
class NetworkLayer;
class Scheduler;
class Sample;

struct MsgLocalLBRequest;
struct MsgLocalLBResponse;
struct MsgLeaveJoinLBRequest;
struct MsgLeaveJoinDenial;

template<class K, class comp>
class TimerRegistry {
    typedef map<K, ref<Timer>&, comp> _TimerMap;

    _TimerMap m_Map;
public:
    void RegisterTimer (K key, ref<Timer>& timer) { 
	m_Map.erase (key);
	m_Map.insert (_TimerMap::value_type (key, timer));
    }

    void UnregisterTimer (K key) {
	typename _TimerMap::iterator it = m_Map.find (key);
	if (it == m_Map.end ()) 
	    return;

	it->second->Cancel ();
	m_Map.erase (it);
    }
};

class LoadBalancer;

class LoadBalanceTimer : public Timer {
    LoadBalancer *m_LB;
    u_long m_Timeout;
public:
    LoadBalanceTimer (LoadBalancer *lb, u_long timeout) 
	: Timer (timeout), m_LB (lb), m_Timeout (timeout) {}

    void OnTimeout ();
    const u_long GetTimeout () const { return m_Timeout; }
};

class MakeStableTimer : public Timer {
    LoadBalancer *m_LB;
public:
    MakeStableTimer (LoadBalancer *lb) : Timer (0), m_LB (lb) {}

    void OnTimeout ();
};
class LoadBalancer : public MessageHandler {
    friend class LoadBalanceTimer;
    friend class LeaveJoinLBReqTracker;
    friend class ResetLocalStateContinuation;
    friend class MakeStableTimer;

    MemberHub      *m_Hub;
    MercuryNode    *m_MercuryNode;
    NetworkLayer   *m_Network;
    IPEndPoint      m_Address;
    Scheduler      *m_Scheduler;

    byte            m_State;
    byte            m_RingState;
    double          m_LocalLoad;

    SIDTimeValMap   m_LocalLBReqSIDs;
    ptr<IPEndPoint> m_SentLocalReqTo;
    ref<LoadBalanceTimer> m_LoadBalanceTimer;

    ptr<MakeStableTimer> m_MakeStableTimer;
    ptr<LeaveJoinLBReqTracker> m_LeaveJoinLBReqTracker;
    SID             m_LB_Requestor;
    double          m_EarlierLoad;
public:
    static const float EPSILON = 1.0e-5;

    enum { DOING_NOTHING, SENT_REQ_LOCAL_NBR, STARTED_LEAVE_JOIN, WAITING_FOR_LEAVE_JOIN_RESPONSE, CHECKING_SUCC };
    enum { UNSTABLE, STABLE };

    LoadBalancer (MemberHub *hub);
    virtual ~LoadBalancer () {}
    void Start ();
    void Pause ();
    void SetRingUnstable (int timeout);
    void LeaveCheckExpired (IPEndPoint reqtor);

    void ProcessMessage(IPEndPoint *from, Message *msg);

    double GetMyLoad ();
    double GetAverageLoad ();
private:
    void CheckLoadBalance ();
    bool SetLocalLoad ();
    void GetPredSuccLoad (float *pred, float *succ);

    bool DoingLocalOps ();
    bool DoingRemoteOps ();
    bool IsLoadBalanceOngoing ();

    bool CheckLocalOK (IPEndPoint *from);
    bool AmHeavy ();
    bool DoLocalLoadBalance ();
    void DoRemoteLoadBalance ();
    bool IsRequestDuplicate (IPEndPoint *from);
    void SendDenial (IPEndPoint *from);

    IPEndPoint ChooseLightCandidate (vector<Sample *>& lightsamples);

    void HandleLocalLBRequest (IPEndPoint *from, MsgLocalLBRequest *llb);
    void HandleLocalLBResponse (IPEndPoint *from, MsgLocalLBResponse *llb);
    void HandleLeaveJoinLBRequest (IPEndPoint *from, MsgLeaveJoinLBRequest *ljr);
    void HandleLeaveJoinDenial (IPEndPoint *from, MsgLeaveJoinDenial *den);
    void HandleLeaveCheckResponse (IPEndPoint *from, MsgLeaveCheckResponse *resp);
    void HandleLeaveCheckRequest (IPEndPoint *from, MsgLeaveCheckRequest *req);

    void SetState (int state) { m_State = state; }
    int GetState () { return m_State; }

    void GetLightSamples (vector<Sample *>& samples);
};

#endif /* __LOADBALANCING__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
