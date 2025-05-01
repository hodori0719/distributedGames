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
#ifndef __SIMULATOR__H
#define __SIMULATOR__H   

#include <mercury/NetworkLayer.h>
#include <mercury/Scheduler.h>
#include <util/TimeVal.h>

typedef u_long (*LatencyFunc) (IPEndPoint& start, IPEndPoint& end);

class Node;
class EventQueue;

class Simulator : public NetworkLayer, public Scheduler {

    EventQueue    *m_Queue;
    TimeVal        m_CurrentTime;
    LatencyFunc    m_LatencyFunc;
 public:
    Simulator ();
    virtual ~Simulator ();

    static const int NODE_TO_NODE_LATENCY = 50;     // 50 milliseconds

    void AddNode (Node& node);
    void RemoveNode (Node& node);

    void SetLatencyFunc (LatencyFunc func) { m_LatencyFunc = func; }

    //============================================================================
    /////// Networklayer 
    virtual int SendMessage(Message *msg, IPEndPoint *toWhom, TransportType proto);

    // This is irrelevant for the simulator
    virtual void StartListening (TransportType proto) {}
    virtual void StopListening () {}
    virtual ConnStatusType GetNextMessage (IPEndPoint *ref_fromWhom, Message **ref_msg) { 
	return CONN_NOMSG; 
    }
    virtual void CloseConnection(IPEndPoint *otherEnd, TransportType proto) {}

    virtual void FreeMessage(Message *msg);

    //============================================================================
    //////// Scheduler functionality

    virtual void RaiseEvent (ref<SchedulerEvent> event, IPEndPoint& address, u_long millis);

    /// XXX not implemented. dont use this. just use a "cancelled" flag in the event 
    /// and dont fire the event when the scheduler calls Execute
    virtual void CancelEvent (ref<SchedulerEvent> event);
    virtual void ProcessTill (TimeVal& limit);
    virtual void ProcessFor (u_long millis) {
	TimeVal t = TimeNow () + millis;
	ProcessTill (t);
    }

    virtual void Reset () {}
    virtual TimeVal& TimeNow () { return m_CurrentTime; }
};

#endif /* __SIMULATOR__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
