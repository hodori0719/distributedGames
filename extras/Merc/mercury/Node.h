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
#ifndef __NODE__H
#define __NODE__H   

#include <mercury/common.h>
#include <mercury/IPEndPoint.h>
#include <mercury/Scheduler.h>
#include <mercury/Message.h>
#include <mercury/NetworkLayer.h>

class Node {
 protected:
    NetworkLayer  *m_Network;
    Scheduler     *m_Scheduler;
    IPEndPoint     m_Address;
 public:
    Node (NetworkLayer *network, Scheduler *scheduler, IPEndPoint& addr) :
	m_Network (network), m_Scheduler (scheduler), m_Address (addr)  
	{}
    virtual ~Node () {}

    NetworkLayer *GetNetwork () { return m_Network; }
    Scheduler *GetScheduler () { return m_Scheduler; }
    IPEndPoint& GetAddress () { return m_Address; }

    virtual void ReceiveMessage (IPEndPoint *from, Message *msg) = 0;
};

class DummyNode : public Node {

 public:
    DummyNode (NetworkLayer *network, Scheduler *scheduler, IPEndPoint &addr) : Node (network, scheduler, addr) {}
    // this is not intended to receive messages. just a placeholder
    virtual void ReceiveMessage (IPEndPoint *from, Message *msg) { ASSERT (false); }
};

#endif /* __NODE__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
