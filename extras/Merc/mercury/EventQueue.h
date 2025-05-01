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
#ifndef __EVENTQUEUE__H
#define __EVENTQUEUE__H   

#include <queue>
#include <vector>
#include <util/TimeVal.h>
#include <mercury/Node.h>
#include <mercury/Scheduler.h>

struct SEvent { 
    ref<SchedulerEvent> ev;
    Node& node;
    TimeVal firetime;

    SEvent (ref<SchedulerEvent> e, Node& n, TimeVal f) : ev (e), node (n), firetime (f) {}
};

struct less_SEvent {
    /* the prio queue keeps the largest on top, by default, since
     * we want the smallest to be at the top, we revert comparison */
    bool operator () (const SEvent* a, const SEvent* b) const {
	return a->firetime > b->firetime; 
    }
};

typedef priority_queue <SEvent *, vector <SEvent *>, less_SEvent> EventHeap;

class EventQueue {
    EventHeap m_Heap;
 public:
    void Insert (ref<SchedulerEvent> ev, Node& n, TimeVal t) {
	m_Heap.push (new SEvent (ev, n, t));
    }
    const SEvent *Top () {
	return m_Heap.top ();
    }
    void Pop () {
	SEvent *e = m_Heap.top ();
	m_Heap.pop ();
	delete e;
    }
    bool Empty () {
	return m_Heap.empty ();
    }    
    // inefficient, but who cares
    void Clear () {
	while (!Empty ()) {
	    Pop ();
	}
    }
};

#endif /* __EVENTQUEUE__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
