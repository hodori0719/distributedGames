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
#ifndef TIMER__H
#define TIMER__H

#include <mercury/common.h>
#include <mercury/Scheduler.h>
#include <list>

class Timer : public SchedulerEvent {
 protected:
    int         m_NTimeouts;             // how many times has the timer been fired. OnTimeout() would increment it if needed.
    u_long      m_NextDelay; 
    bool        m_Cancelled;

 public:
    Timer(u_long timeout);

    void Cancel()       { m_Cancelled = true; }
    bool IsCancelled() const { return m_Cancelled; }
    u_long GetNextDelay () const { return m_NextDelay; }

    void Execute (Node& node, TimeVal& timenow);
    virtual void OnTimeout() = 0;

 protected:
    void _RescheduleTimer (u_long timeout);
};

void G_RegisterTimer (Node& node, ref<Timer> timer);

#endif // TIMER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
