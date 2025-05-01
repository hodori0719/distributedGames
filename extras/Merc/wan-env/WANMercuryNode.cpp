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

// if somebody can come up with a better name than this one
// they are welcome. this app is the interface mercury offers 
// a "real" application. 

#include <mercury/Parameters.h>
#include <mercury/IPEndPoint.h>
#include <mercury/NetworkLayer.h>
#include <mercury/Scheduler.h>
#include <wan-env/WANParameters.h>
#include <wan-env/WANScheduler.h>
#include <wan-env/WANMercuryNode.h>
#include <wan-env/RealNet.h>
#include <signal.h>

extern bool g_WANRecvSleeps;
#undef HAVE_THREADS

#ifdef ENABLE_ALARM
static bool sg_GotAlarm = false;
static struct itimerval sg_Itimerval;

void SET_ALARM (u_long timeout) 
{
    g_GotAlarm = false;

#ifdef USE_UALARM
    ualarm (timeout * USEC_IN_MSEC, 0); 
#else
    memset (&g_Itimerval, sizeof (struct itimerval), 0);
    g_Itimerval.it_value.tv_sec = timeout / MSEC_IN_SEC;

    timeout %= MSEC_IN_SEC;
    g_Itimerval.it_value.tv_usec = timeout * USEC_IN_MSEC;

    if (setitimer (ITIMER_REAL, &g_Itimerval, NULL) < 0) {
	perror ("setitimer");
	Debug::die (" error setting timer\n");
    }
#endif
}

inline bool ALARMED () {
    return g_GotAlarm;
}

void handle_alarm (int signal) {
    g_GotAlarm = true;
}
#endif

WANMercuryNode *WANMercuryNode::GetInstance (int port)
{
    IPEndPoint address (g_Preferences.hostname, port);
    WANScheduler *sched = new WANScheduler ();
    RealNet *net = new RealNet (sched, address);

    WANMercuryNode *node = new WANMercuryNode (net, sched, address);
    sched->SetNode (node);
    return node;
}

WANMercuryNode::WANMercuryNode (NetworkLayer *network, Scheduler *sched, IPEndPoint& addr) 
    : MercuryNode (network, sched, addr), m_Thread (0)
{
    if (g_MeasurementParams.enabled)
	((RealNet *) m_Network)->EnableLog ();
}

extern bool doPrint;
bool doPrint = false;

#define INIT_STOP_TIME()  unsigned long long stoptime; \
if (timeout == 0) stoptime = CurrentTimeUsec () + (unsigned long long) 100000 * USEC_IN_MSEC;  \
    else stoptime = CurrentTimeUsec () + timeout * USEC_IN_MSEC; 

#define SMALL_TIMEOUT_USECS 250


void WANMercuryNode::Recv (u_long timeout)
{
    bool processed;
    INIT_STOP_TIME ();

    NOTE(WANRECV:TIMEOUT, timeout);
    m_Scheduler->ProcessTill (m_Scheduler->TimeNow());

    // while (CurrentTimeUsec () < stoptime) {
    int i = 0;
    for (i = 0; i < MAX_PACKETS_TO_PROCESS; i++) {
	// receive and process one packet; right now, dont 
	// give to app, will think about it later

	processed = ProcessOnePacket ();
	if (!processed) 
	    break;
    }

    NOTE(WANRECV::processed, i);

    if (CurrentTimeUsec () < stoptime)
	m_Scheduler->ProcessTill (m_Scheduler->TimeNow ());

    /// XXX argh; we have gone back and forth on this so many times!! 
    /// try to sleep for some time...
    if (g_WANRecvSleeps) { 
	unsigned long long cur = CurrentTimeUsec ();
	if (cur < stoptime && timeout != 0) {
	    // u_long msec = (stoptime - cur) / USEC_IN_MSEC;
	    // NOTE (WAN:RECVSLEEP, msec);
	    u_long t = stoptime - cur;

	    /* 60 seconds is the max timeout specified anywhere */
	    ASSERTDO (t <= 60 * USEC_IN_SEC, { 
		WARN << " ** "; 
		print_vars(cerr, timeout, cur, stoptime, t, processed);
	    });
	    RealNet::DoWorkUsec (stoptime - cur);
	}
    }
}

void WANMercuryNode::Send (u_long timeout)
{
    INIT_STOP_TIME ();

    int n = 0;
    // while (CurrentTimeUsec () < stoptime) 
    while (true)
    {
	if (!MercuryNode::SendPacket ()) 
	    break;
	n++;
    }

    NOTE (WAN:SENDPKT, n);
}

void WANMercuryNode::Maintenance (u_long timeout)
{
    INIT_STOP_TIME ();

    if (CurrentTimeUsec () < stoptime)
	m_Scheduler->ProcessTill (m_Scheduler->TimeNow ());
}

/* Dont fret about this too much; not called at important places */
void WANMercuryNode::DoWork (u_long timeout)
{
    IPEndPoint from;
    Message *msg = NULL;
    bool processed, sent;

    NOTE(WAN_DOWORK:TIMEOUT, timeout);

    START(WANMercury::DoWork);

    INIT_STOP_TIME ();

    m_Scheduler->ProcessTill (m_Scheduler->TimeNow());

    while (CurrentTimeUsec () < stoptime)
	{	
	    sent = MercuryNode::SendPacket ();
	    processed = ProcessOnePacket ();

	    if (!processed && !sent) 
		break;
	}

    if (CurrentTimeUsec () < stoptime) 
	m_Scheduler->ProcessTill (m_Scheduler->TimeNow ());

    STOP(WANMercury::DoWork);
}

void *ThreadCaller(void *arg)
{
    WANMercuryNode *rtr = (WANMercuryNode *) arg;
    rtr->StartThread();
    return NULL;
}

void WANMercuryNode::FireUp()
{
#ifdef HAVE_THREADS	
    int retval;

    retval = pthread_create(&m_Thread, 0, &ThreadCaller, this);
    if (retval != 0) {
	Debug::die 
	    ("MercuryNode::MercuryNode (port): Could not create thread!  Exiting.\n");
    }
    //pthread_detach(m_Thread);
#else
    StartThread();
#endif
}

void mainthread_signal_handler(int signal)
{
#ifdef HAVE_THREADS
    msg ("*** thread signal handler: received signal %d\n", signal);
    pthread_exit (0);
#else
    // die causes abort
    Debug::die_no_abort("death brought to you by signal: %d!\n", signal);
#endif
}

void WANMercuryNode::StartThread()
{
    if (signal(SIGINT, mainthread_signal_handler) == SIG_ERR)
	Debug::die("could not install thread handler for main thread\n");

#ifdef ENABLE_ALARM
    if (signal(SIGALRM, handle_alarm) == SIG_ERR)
	Debug::die("could not install thread handler for SIGALRM\n");
    if (signal(SIGVTALRM, handle_alarm) == SIG_ERR)
	Debug::die("could not install thread handler for SIGVTALRM\n");
#endif

    // start the network
    m_Network->StartListening(PROTO_TCP);
    m_Network->StartListening(Parameters::TransportProto);

    // start mercury
    MercuryNode::Start(); 

#ifdef HAVE_THREADS
    while (true) {
	DoWork(90);
	Sleep(10);
    }
#endif
}

void WANMercuryNode::Shutdown ()
{
    MercuryNode::Stop ();

    m_Scheduler->Reset ();
    m_Network->StopListening ();
}

// Some connection was closed or has an error on it. Figure out who it corresponds to
// Perform reparative action, if needed. 

void WANMercuryNode::HandleConnectionError(ConnStatusType status, IPEndPoint * from)
{
    WARN << "Reporting connection closed or error from " << *from << endl;
    // m_HubManager->OnPeerDeath(from);
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
