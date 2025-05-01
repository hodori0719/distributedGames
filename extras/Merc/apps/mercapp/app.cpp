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

using namespace std;

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Mercury.h>
#include <wan-env/RealNet.h>
#include <mercury/ObjectLogs.h>
#include <mercury/RoutingLogs.h>
#include <wan-env/WANMercuryNode.h>
#include <mercury/Hub.h>
#include <mercury/HubManager.h>
#include <mercury/Sampling.h>
#include <util/OS.h>
#include <mercury/Event.h>

static int g_NumPubs;
static int g_RunningTime;
static int g_IdleTime;
static float g_InitialLoad;

static OptionType g_Options[] =
    {
	{ '#', "npubs", OPT_INT, "number of publications to send", 
	  &g_NumPubs, "10", NULL },
	{ '#', "time", OPT_INT, "run for this much time (seconds)",
	  &g_RunningTime, "60", NULL },
	{ '#', "idletime", OPT_INT, "time to idle for (to account for skew in start times)",
	  &g_IdleTime, "120", NULL },
	{ '#', "load", OPT_FLT, "initial value of load",
	  &g_InitialLoad, "100.0", NULL },
	{ 0, 0, 0, 0, 0, 0, 0 }
    };

class TestSampler : public LoadSampler {
    float m_Value;

public:	    
    TestSampler (MemberHub *hub, float val) : LoadSampler (hub), m_Value (val) {}
    virtual ~TestSampler () {}

    const float GetValue () const { return m_Value; }
    virtual Metric *GetPointEstimate () { 
	ASSERT (m_Hub != NULL);
	if (m_Hub->GetRange () == NULL)
	    return NULL;

	// cerr << " returning value " << m_Value << " for load sampling " << endl;
	return new LoadMetric (m_Value);
    }

    void SetLoad (Metric *l) {
	MercuryNode *m_MercuryNode = m_Hub->GetMercuryNode ();
	m_Value = (float) (dynamic_cast<LoadMetric *> (l)->GetLoad ());
	MDB (-5) << "new load=" << m_Value << endl;
    }
};

class TimepassTimer : public Timer { 
    int m_Timeout;
public:
    TimepassTimer (int timeout) : Timer (timeout), m_Timeout (timeout) {}
    void OnTimeout () { 
	cerr << " waiting to get joined ... " << endl;
	_RescheduleTimer (m_Timeout);
    }
};

bool finished = false;

class FinishEvent : public SchedulerEvent {
public:
    void Execute (Node& node, TimeVal& timenow) {
	cerr << " finishing at " << timenow << endl;
	finished = true;
    }
};

void SendPublication (WANMercuryNode *router, Constraint& cst)
{
    int min = cst.GetMin ().getui (), max = cst.GetMax ().getui ();
    int v = min + (int) (drand48 () * (max - min));

    Constraint c (0, v, v);
    MercuryEvent *ev = new MercuryEvent ();
    ev->AddConstraint (c);

    router->SendEvent (ev);
    delete ev;
}

void SendSubscription (WANMercuryNode *router, Constraint& cst)
{
    int min = cst.GetMin ().getui (), max = cst.GetMax ().getui ();
    int v1 = min + (int) (drand48 () * 0.5 * (max - min));
    int v2 = (int) (drand48 () * 0.2 * (max - min));

    Constraint c (0, v1, v2);
    Interest *in = new Interest ();
    in->AddConstraint (c);

    router->RegisterInterest (in);
    delete in;
}

class SendPubsubTimer : public Timer {
    WANMercuryNode *router;
    int nsent;
    int timeout;
    vector<Constraint> csts;
public:
    SendPubsubTimer (WANMercuryNode *router, int timeout) : Timer (timeout), router (router), timeout (timeout) {
	nsent = 0;
	csts = router->GetHubConstraints ();
	ASSERT (csts.size () > 0);
    }

    void OnTimeout () {
	if (nsent > g_NumPubs)
	    return;

	SendPublication (router, csts[0]);
	SendSubscription (router, csts[0]);
	nsent++;

	_RescheduleTimer (timeout);
    }
};

class PrintLoadTimer : public Timer {
    WANMercuryNode *router;
    TestSampler *sampler;
    int timeout;
public:
    PrintLoadTimer (WANMercuryNode *router, TestSampler *sampler, int timeout) : 
	Timer (timeout), router (router), sampler (sampler), timeout (timeout) 
    {
    }

    void OnTimeout () {
	cerr << "[" << Debug::GetFormattedTime() << "]" << " load=" << sampler->GetValue () << endl;
	_RescheduleTimer (timeout);
    }
};

int main(int argc, char **argv)
{
    InitializeMercury (&argc, argv, g_Options, true);
    DBG_INIT (NULL);
    
    IPEndPoint sid (g_Preferences.hostname, g_Preferences.port);

    InitObjectLogs (sid);
    InitRoutingLogs (sid);

    WANMercuryNode *m_Router = WANMercuryNode::GetInstance (g_Preferences.port);

    m_Router->FireUp ();    

    ref<TimepassTimer> m_Timepass = new refcounted<TimepassTimer> (30000);
    m_Router->GetScheduler ()->RaiseEvent (m_Timepass, m_Router->GetAddress (), 0);

    cerr << " starting up with initial load=" << g_InitialLoad << endl;

    int i = 0;
    /// we use "AllJoined" as a barrier (and not for ensuring "good"
    /// behavior, mercury seems robust to routing when some people
    /// havent joined)
    while (!m_Router->AllJoined ()) {
	// this timeout specifies how much time you can sleep if 
	// there's nothing to do. also, it specifies the maximum 
	// amount of time you should spend getting packets from 
	// the kernel
	RealNet::DoWork (100);

	// this timeout says how much time you should process the 
	// packets dequed from the kernel.
	m_Router->DoWork (200);
    }

    m_Timepass->Cancel ();
    cerr << "JOINED!" << endl;

    /// XXX this only works for the first hub. Ding! - Ashwin [08/05/2005]
    MemberHub *mhub = (MemberHub *) m_Router->GetHubManager ()->GetHubByID (0); 
    TestSampler *sampler = new TestSampler (mhub, g_InitialLoad);
    m_Router->RegisterLoadSampler (0, sampler);

    m_Router->GetScheduler ()->RaiseEvent (new refcounted<FinishEvent> (), m_Router->GetAddress (), g_RunningTime * 1000);
    m_Router->GetScheduler ()->RaiseEvent (new refcounted<SendPubsubTimer> (m_Router, 200), m_Router->GetAddress (), 0);
    m_Router->GetScheduler ()->RaiseEvent (new refcounted<PrintLoadTimer> (m_Router, sampler, 1000), m_Router->GetAddress (), 0);

    cerr << " registering finish event at " << m_Router->GetScheduler ()->TimeNow () << " to be fired after " << g_RunningTime << " seconds"  << endl;

    while (!finished) {
	RealNet::DoWork (50);
	m_Router->DoWork (50);
    }

    cerr << "* hit timelimit ! going home soon!!" << endl;

    // idle for a time proportional to vservers; 
    // let's say it takes 500 ms for the bootstrap to establish
    // a TCP connection and send the MsgAllJoined message;
    // so, people can be off by g_VServers * 500 ms. we put in
    // a factor of 4 slack, just to be safe.
    //
    // the running scripts pass this option now, instead of 
    // calculating it here.
    finished = false;

    // stop all messages
    g_VerbosityLevel = -100;   
    cerr << "* idling for " << g_IdleTime << " seconds " << endl;
    m_Router->GetScheduler ()->RaiseEvent (new refcounted<FinishEvent> (), m_Router->GetAddress (), g_IdleTime * 1000);
    while (!finished) { 
	RealNet::DoWork (100);  // sleep long if there's nothing to do!
	m_Router->DoWork (200);
    }

    cerr << "exiting gracefully" << endl;
    m_Router->Shutdown ();
    return 0;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
