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
#ifndef __SCHEDULER__H
#define __SCHEDULER__H   

#include <mercury/common.h>
#include <mercury/IPEndPoint.h>
#include <util/refcnt.h>

class Node;

class SchedulerEvent : public virtual refcount /* allow mkref's */ {
 public:
    virtual ~SchedulerEvent () {}

    virtual void Execute (Node& node, TimeVal& timenow) = 0;
};

class Scheduler {
 public:
    virtual ~Scheduler () {}

    virtual void RaiseEvent (ref<SchedulerEvent> event, IPEndPoint& dest, u_long millis) = 0;
    virtual void CancelEvent (ref<SchedulerEvent> event) = 0;
    virtual void ProcessTill (TimeVal& limit) = 0;
    virtual void ProcessFor (u_long millis) = 0;
    virtual void Reset () = 0;
    virtual TimeVal& TimeNow () = 0;
};

#endif /* __SCHEDULER__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
