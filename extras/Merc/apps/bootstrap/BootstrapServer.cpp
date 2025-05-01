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
#include <mercury/BootstrapNode.h>
#include <Mercury.h>
#include <wan-env/WANScheduler.h>
#include <wan-env/RealNet.h>
#include <util/debug.h>
#include <util/TimeVal.h>
#include <util/OS.h>

static WANScheduler *s_Scheduler;
static RealNet *s_Network;
static IPEndPoint s_BootstrapIP;

static void DoWork (BootstrapNode *node)
{
    s_Network->StartListening (PROTO_TCP);
    s_Network->StartListening (PROTO_UDP);

    IPEndPoint from((uint32)0, 0);
    Message *msg = 0;

    while (true) {
#ifndef ENABLE_REALNET_THREAD
	// Do periodic Network processing
	RealNet::DoWork();
#endif
	s_Scheduler->ProcessTill (s_Scheduler->TimeNow ());

	ConnStatusType status = s_Network->GetNextMessage(&from, &msg);

	switch (status) {
	case CONN_NEWINCOMING:
	    // HandleNewConnection(&from, msg);
	    // fall thru'...            
	case CONN_OK:
	    node->ReceiveMessage (&from, msg);
	    break;
	case CONN_CLOSED:
	case CONN_ERROR:
	    //HandleConnectionError(status, &from);
	    break;

	case CONN_NOMSG:
	    break;
	default:
	    Debug::warn ("BUGBUG: Hmm... got weird connection status.\n");
	    break;
	}
	// OS::SleepMillis(10);
    }
}

int main(int argc, char **argv)
{
    strcpy (g_ProgramName, *argv);

    // most preferences (-v, -D, --hostname, etc.) are stored inside
    // libmercury! gah! otherwise, a bootstrapserver uses only 
    // BootstrapNode class from libmercury. - Ashwin [02/18/2005]
    InitializeMercury (&argc, argv, g_BootstrapOptions);

    if (!g_BootstrapPreferences.schemaFile[0]) {
	PrintUsage(g_BootstrapOptions);
	return 2;
    }
    srand(42);

    IPEndPoint s_BootstrapIP (g_Preferences.hostname, BootstrapNode::DEFAULT_BOOTSTRAP_PORT);
    s_Scheduler = new WANScheduler ();
    s_Network = new RealNet (s_Scheduler, s_BootstrapIP);

    BootstrapNode *node = new BootstrapNode(s_Network, s_Scheduler, s_BootstrapIP, g_BootstrapPreferences.schemaFile);
    s_Scheduler->SetNode (node);

    node->Start ();
    DBG_INIT(&s_BootstrapIP);

    DoWork (node);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
