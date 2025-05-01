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
#ifndef __SIMMERCURYNODE__H
#define __SIMMERCURYNODE__H   

// $Id: SimMercuryNode.h 2382 2005-11-03 22:54:59Z ashu $

#include <mercury/MercuryNode.h>
#include <mercury/Scheduler.h>

class MercuryNodePeriodicTimer : public Timer {

    MercuryNode *m_MercuryNode;	
 public:
    MercuryNodePeriodicTimer (MercuryNode *node) : Timer (0), m_MercuryNode (node) {} 

    void OnTimeout () {
	// m_MercuryNode->DoPeriodic ();
	m_MercuryNode->SendApplicationPackets ();
	_RescheduleTimer (20);          // reschedule ourselves very frequently. depending on the scheduler granularity, we may not get called this often though.
    }
};

class SimMercuryNode : public MercuryNode {

 public:
    SimMercuryNode (NetworkLayer *network, Scheduler *sched, IPEndPoint &addr) : MercuryNode (network, sched, addr) 
	{
	    m_Simulating = true;
	}

    void StartUp () {	
	m_Scheduler->RaiseEvent (new refcounted<MercuryNodePeriodicTimer> (this), m_Address, 0);
	MercuryNode::Start ();
    }
};

#endif /* __SIMMERCURYNODE__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
