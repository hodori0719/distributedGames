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
#include <sstream>
#include <Mercury.h>
#include <util/TimeVal.h>
#include <mercury/Message.h>
#include <wan-env/RealNet.h>
#include <util/debug.h>
#include <util/Benchmark.h>
#include <wan-env/WANScheduler.h>

bool ReadMessage(IPEndPoint *from, Message **msg, RealNet *net) {
    ConnStatusType status;

    do {
	status = net->GetNextMessage(from, msg);

	switch (status) {
	case CONN_NEWINCOMING:
	case CONN_OK:
	    break;
	case CONN_CLOSED:
	    DBG << "connection closed from: " << from->ToString() << endl;
	    return false;
	case CONN_ERROR:
	    DBG << "connection error from: " << from->ToString() << endl;
	    return false;
	case CONN_NOMSG:
	    return false;
	default:
	    Debug::die("Hmm... got weird connection status.");
	    break;
	}

    } while (*msg == NULL);

    ASSERT(*msg != NULL);
    return true;
}

int main(int argc, char **argv)
{
    InitializeMercury(&argc, argv, NULL, false);
    if (argc != 2) {
	cerr << "usage: server <self_ip:port>" << endl;
	exit(1);
    }

    IPEndPoint self(argv[1]);

    WANScheduler *sched = new WANScheduler ();
    RealNet *net = new RealNet(sched, self);
    net->StartListening (PROTO_UDP);

    //////////////////////////////////////////////////////////////////////////

    IPEndPoint from;
    Message *msg = NULL;

    while (true) {
	RealNet::DoWork();
	msg = NULL;
	if (ReadMessage(&from, &msg, net) && msg->GetType () == MSG_PING) {
	    MsgPing pong;
	    pong.pingNonce = ((MsgPing *)msg)->pingNonce;
	    gettimeofday(&pong.time, NULL);
	    net->SendMessage(&pong, &from, PROTO_UDP);
	    net->FreeMessage(msg);
	}
    }
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
