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

#ifndef __DELAYED_TRANSPORT__H
#define __DELAYED_TRANSPORT__H

#include <hash_map.h>
#include <wan-env/Transport.h>

typedef hash_map<SID, float, hash_SID, equal_SID> LatencyMap;
typedef LatencyMap::iterator LatencyMapIter;

/**
 * Transport wrapper that delays packets on the receiver end based on
 * an artificial latency graph.
 */
class DelayedTransport : public Transport {

    friend class DelayedConnection;

    Transport *m_Trans;

    static bool m_LatMapInited;
    static LatencyMap m_LatMap;

    DelayedConnection *GetMyConnection(SID *peer);

 public:

    static void  InitLatMap(const SID& me, const char *filename);
    static float GetLat(const SID& peer);

    DelayedTransport(Transport *t);
    virtual ~DelayedTransport();

    void  StartListening();
    void  StopListening();
    int   FillReadSet(fd_set *tofill);
    void  DoWork(fd_set *isset, u_long timeout_usecs);
    uint32 GetPriority() { return m_Trans->GetPriority(); }

    Connection *GetConnection(IPEndPoint *target);
    ConnStatusType GetReadyConnection(Connection **connp);
    void CloseConnection(IPEndPoint *target);
};

#endif // __UDP_TRANSPORT__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
