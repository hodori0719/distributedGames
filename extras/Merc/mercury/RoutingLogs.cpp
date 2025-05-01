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
/* -*- Mode:c++; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */

/**************************************************************************
  RoutingLogs.cpp

begin           : Nov 8, 2003
version         : $Id: RoutingLogs.cpp 2507 2005-12-21 22:05:36Z jeffpang $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#include <mercury/RoutingLogs.h>

    IMPLEMENT_EXPLOG(MessageLog, MessageEntry);
IMPLEMENT_EXPLOG(AggregateMessageLog, AggregateMessageEntry);
//IMPLEMENT_EXPLOG(PubSubHopsLog, HopsEntry);
//IMPLEMENT_EXPLOG(PubSubTestLog, PubTestEntry);
IMPLEMENT_EXPLOG(LoadBalanceLog, LoadBalEntry);

void InitRoutingLogs(IPEndPoint sid)
{
    ASSERT(g_MeasurementParams.enabled);
    ASSERT(sid != SID_NONE);

    if (!g_MessageLog) {
	INFO << sid << endl;
	INIT_EXPLOG (MessageLog, sid);
	INIT_EXPLOG (AggregateMessageLog, sid);
    }
    if (!g_LoadBalanceLog && g_Preferences.do_loadbal)
	INIT_EXPLOG (LoadBalanceLog, sid);

    /*
      if (!g_PubTestLog)
      g_PubTestLog    = new ExpLog<PubTestEntry>(g_LocalSID, "PubTestLog");
    */

    /*
      if (!g_PubSubHopsLog)
      g_PubSubHopsLog = new ExpLog<HopsEntry>(g_LocalSID, "PubSubHopsLog");
    */
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
