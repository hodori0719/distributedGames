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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:nil -*- */

#include <wan-env/TCPPassiveTransport.h>
#include <util/Utils.h>

void TCPPassiveTransport::StartListening()
{
    // Do nothing.
}

void TCPPassiveTransport::StopListening()
{
    _ClearConnections();
}

int TCPPassiveTransport::FillReadSet(fd_set *tofill)
{
    int maxfd = 0;

    Lock();

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); iter++) {
	Connection *connection = (*iter);
	ASSERT(connection != 0);

	if (connection->GetStatus() == CONN_ERROR || 
	    connection->GetStatus() == CONN_CLOSED)
	    continue;

	FD_SET(connection->GetSocket(), tofill);
	if (connection->GetSocket() > maxfd) {
	    maxfd = connection->GetSocket();
	}
    }

    Unlock();

    return maxfd;
}

void  TCPPassiveTransport::DoWork(fd_set *isset)
{
    // periodically clean up closed connections

    TimeVal now = TimeNow ();
    PERIODIC2(1000, now, _CleanupConnections() );
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
