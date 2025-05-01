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

#ifndef __CBR_TRANSPORT__H
#define __CBR_TRANSPORT__H

#include <wan-env/UDPTransport.h>
#include <wan-env/CBRConnection.h>

/**
 * A unreliable constant bitrate transport over UDP. It supports
 * aggregate outbound bitrate and per-flow bitrate limits.
 *
 * Uses the queuing discipline of the underlying UDP transport.
 */
class CBRTransport : public UDPTransport {

    friend class CBRConnection;

 protected:

    TimeVal        m_SendTime;   // min time to send next packet
    ConnectionList m_SenderList; // round-robin queue for senders

    // Overloaded to add/remove connections from sender queue also
    void _ClearConnections();
    void _CleanupConnections();
    UDPConnection *CreateConnection(Socket sock, IPEndPoint *otherEnd);
    void AddConnection(Connection *conn);
    void RemoveConnection(Connection *conn);

 public:

    // XXX Make these configurable

    // Max bitrate total allowed outbound on this transport (bytes/sec)
    static const int MAX_AGGREGATE_RATE = 1875000; // ~ 1.5Mbps
    // Max bitrate total allowed outbound on each flow (bytes/sec)
    static const int MAX_PERFLOW_RATE   =   48000; // ~ 384kbps
    // The length of the burst to send each time we service connections (ms)
    static const int MAX_SERVICE_BURST  =  1; // ms

    // queuing discipline for send queue
    static const QueuingDiscType SEND_QUEUE_DISCIPLINE = Q_DROPHEAD;
    // max size for send queue (pkts)
    static const int             SEND_QUEUE_SIZE       = 1024;

    CBRTransport() : m_SendTime(TIME_NONE) {}
    virtual ~CBRTransport() {}

    void  DoWork(fd_set *isset, u_long timeout_usecs);
};

#endif // __CBR_TRANSPORT__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
