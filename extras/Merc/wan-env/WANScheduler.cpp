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

#include <util/TimeVal.h>
#include <mercury/EventQueue.h>
#include <wan-env/WANScheduler.h>
#include <wan-env/WANMercuryNode.h>
#include <util/OS.h>
#include <list>

bool WANScheduler::m_InitedCPUMHz = false;

WANScheduler::WANScheduler () : m_Node (NULL)
{
    m_Queue = new EventQueue ();
    if (!m_InitedCPUMHz)  {
	InitCPUMHz ();
	m_InitedCPUMHz = true;
    }
}

WANScheduler::~WANScheduler () 
{
    delete m_Queue;
}

void WANScheduler::Reset ()
{
    m_Queue->Clear ();
}


TimeVal& WANScheduler::TimeNow ()
{
    OS::GetCurrentTime (&m_Now);
    return m_Now;
}

void WANScheduler::ProcessTill (TimeVal& limit)
{
    while (!m_Queue->Empty ())
	{
	    const SEvent *ev = m_Queue->Top ();
	    if (ev->firetime > limit) 
		break;

	    ev->ev->Execute (*m_Node, TimeNow ());
	    m_Queue->Pop ();
	}
}

void WANScheduler::RaiseEvent (ref<SchedulerEvent> event, IPEndPoint& address /* ignored */, u_long millis)
{
    m_Queue->Insert (event, *m_Node, TimeNow () + millis);
}

// NOBODY uses it for now... properway to implement: keep 
// another hashtable to lookup event - find the SEvent pointer
// remove it, or set a "cancel" flag....
void WANScheduler::CancelEvent (ref<SchedulerEvent> event) 
{
    ASSERT (false); 
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
