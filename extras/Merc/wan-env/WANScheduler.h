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
#ifndef __WANSCHEDULER__H
#define __WANSCHEDULER__H   

#include <util/TimeVal.h>
#include <mercury/Scheduler.h>

class EventQueue;
class WANMercuryNode;

class WANScheduler : public Scheduler {
    TimeVal m_Now;

    Node *m_Node;        // The node I am attached to...
    EventQueue *m_Queue;
    static bool m_InitedCPUMHz;
 public:
    WANScheduler ();
    ~WANScheduler ();

    void SetNode (Node *node) { m_Node = node; }

    virtual void RaiseEvent (ref<SchedulerEvent> event, IPEndPoint& address, u_long millis);
    virtual void CancelEvent (ref<SchedulerEvent> event);
    virtual void Reset ();
    virtual TimeVal& TimeNow ();

    virtual void ProcessTill (TimeVal& limit);    
    virtual void ProcessFor (u_long millis) {
	TimeVal t = TimeNow () + millis;
	ProcessTill (t);
    }
};

#endif /* __WANSCHEDULER__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
