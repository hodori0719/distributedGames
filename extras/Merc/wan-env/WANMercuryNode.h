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
#ifndef __WANMERCURYNODE__H
#define __WANMERCURYNODE__H   

#include <mercury/MercuryNode.h>
#include <mercury/Scheduler.h>
#include <util/TimeVal.h>
#include <pthread.h>

class WANMercuryNode : public MercuryNode {
    pthread_t  m_Thread;

    WANMercuryNode (NetworkLayer *network, Scheduler *sched, IPEndPoint &addr);
 public:
    static WANMercuryNode *GetInstance (int port);

    virtual void DoWork (u_long timeout);        // the appliation is supposed to call this periodically if HAVE_THREADS is disabled
    void StartThread();

    void FireUp();
    void Shutdown();

    void Send (u_long timeout);
    void Recv (u_long timeout);
    void Maintenance (u_long timeout);

 private:
    // this has to be in some way related to UDPs queuesize;
    // we sort of ignore TCP messages here, which aint too good,
    // but we expect UDP to be the predominant protocol.
    static const int MAX_PACKETS_TO_PROCESS = 1024;

    // inline this one.... called _very_ frequently...
    bool ProcessOnePacket () {
	IPEndPoint from = SID_NONE;
	Message *msg = NULL;
	bool nomsg = false; 

	ConnStatusType status = m_Network->GetNextMessage (&from, &msg);

	switch (status) {
	case CONN_NEWINCOMING:
	case CONN_OK:
	    {
		MercuryNode::ReceiveMessage (&from, msg);
	    }
	    break;
	case CONN_CLOSED:
	case CONN_ERROR:
	    HandleConnectionError(status, &from);
	    break;

	case CONN_NOMSG:
	    nomsg = true;
	    break;
	default:
	    WARN << "BUGBUG: Hmm... got weird connection status." << endl;
	    break;
	}

	return nomsg ? false : true;
    }

    void HandleConnectionError(ConnStatusType type, IPEndPoint *from);
};

#endif /* __WANMERCURYNODE__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
