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
#include <mercury/Timer.h>
#include <mercury/Utils.h>
#include <mercury/Node.h>

void G_RegisterTimer (Node& node, ref<Timer> timer) {
    node.GetScheduler ()->RaiseEvent (timer, node.GetAddress (), timer->GetNextDelay ());
}

Timer::Timer (u_long timeout) : m_NTimeouts (0), 
				m_NextDelay (timeout), m_Cancelled (false)
{
}

/// THIS IS THE MOST FUCKED UP TIMER CLASS I COULD HAVE
/// EVER WRITTEN. PLEASE REDESIGN IT SOME DAY.

void Timer::Execute (Node& node, TimeVal& timenow)
{
    // just dont fire; the scheduler will get rid of us from the event queue
    if (m_Cancelled) 
	return;       

    m_NextDelay = 0;
    OnTimeout ();

    if (m_NextDelay > 0) 
	node.GetScheduler ()->RaiseEvent (mkref (this), node.GetAddress (), m_NextDelay);
}

void Timer::_RescheduleTimer (u_long timeout)
{
    m_NextDelay = timeout;
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
