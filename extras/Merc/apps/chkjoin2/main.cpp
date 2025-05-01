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
#include <wan-env/WANScheduler.h>
#include <util/OS.h>
#include <mercury/Event.h>

char g_BsAddr[80]; 
IPEndPoint g_Bootstrap = SID_NONE;
int g_MaxWait;

bool finished = false;
RealNet *g_Network = NULL;
WANScheduler *g_Scheduler = NULL;

static OptionType g_Options[] =
    {
	{ '#', "bsaddr", OPT_STR, "addr:port of the bootstrap server", 
	  g_BsAddr, "localhost:20000", NULL },
	{ '#', "maxtime", OPT_INT, "max time (seconds) to wait",
	  &g_MaxWait, "300", NULL },
	{ 0, 0, 0, 0, 0, 0, 0 }
    };

void SendPing ();

class CheckTimer : public Timer { 
    int m_Timeout;
public:
    CheckTimer (int timeout) : Timer (timeout), m_Timeout (timeout) {}
    void OnTimeout () { 
	SendPing ();
	_RescheduleTimer (m_Timeout);
    }
};

void Exit (int status) {
    g_Network->StopListening ();
    g_Scheduler->Reset ();
    exit (status);
}


class FinishEvent : public SchedulerEvent {
public:
    void Execute (Node& node, TimeVal& timenow) {
	cout << "giving up" << endl;
	finished = true;
	Exit (1);
    }
};

class DNode : public Node {
public:
    DNode (NetworkLayer *n, Scheduler *s, IPEndPoint& a) : Node (n, s, a) {}
    void ReceiveMessage (IPEndPoint *from, Message *msg) {
	ASSERT (false);
    }
};

void ProcessMessage (IPEndPoint *from, Message *msg) {
    ASSERT (msg->GetType () == MSG_PING);

    MsgPing *ping = (MsgPing *) msg;
    if (ping->pingNonce == 0x1) {
	cout << "all joined" << endl;
    }
    else {
	cout << "not joined" << endl;
    }
    finished = true;
    Exit (0);
}

void SendPing () {
    MsgPing *ping = new MsgPing ();

    cerr << " requested info ... " << endl;
    g_Network->SendMessage (ping, &g_Bootstrap, PROTO_UDP);
    delete ping;
}

int main(int argc, char **argv)
{
    InitializeMercury (&argc, argv, g_Options, false /* print options */);
    DBG_INIT (NULL);

    if (g_BsAddr[0] == '\0') {
	PrintUsage (g_Options);
	exit (2);
    }    
    g_Bootstrap = IPEndPoint (g_BsAddr);
    INFO << "bootstrap server at " << g_Bootstrap << endl;


    /// network initialization
    g_Scheduler = new WANScheduler ();
    IPEndPoint localEnd ("localhost:60000");
    g_Network = new RealNet (g_Scheduler, localEnd);

    /// memory "leak" here!
    DNode *dummy = new DNode (g_Network, g_Scheduler, localEnd);
    g_Scheduler->SetNode (dummy);

    // wait for a small time
    g_Scheduler->RaiseEvent (new refcounted<FinishEvent> (), localEnd, 10000);
    g_Network->StartListening (PROTO_UDP);     // realnet is weird, wants to listen even when all you want to do is send. duh!

    /// into the loop
    IPEndPoint from ((uint32) 0, 0);
    Message *msg = 0;

    // contact bootstrap
    SendPing ();

    // wait for response
    while (!finished) {
	RealNet::DoWork ();
	g_Scheduler->ProcessTill (g_Scheduler->TimeNow ());

	ConnStatusType status = g_Network->GetNextMessage(&from, &msg);

	switch (status) {
	case CONN_NEWINCOMING:
	case CONN_OK:
	    ProcessMessage (&from, msg);
	    g_Network->FreeMessage (msg);
	    break;
	case CONN_CLOSED:
	case CONN_ERROR:
	    WARN << " Connection ERROR!" << endl;
	    break;
	case CONN_NOMSG:
	    break;
	default:
	    Debug::warn ("BUGBUG: Hmm... got weird connection status.\n");
	    break;
	}
	// OS::SleepMillis(10);
    }

    // control wont ever come here
    Exit (1);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
