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

#ifndef __UDP_TRANSPORT__H
#define __UDP_TRANSPORT__H

#include <wan-env/Transport.h>
#include <wan-env/UDPConnection.h>

typedef enum { Q_DROPTAIL, Q_DROPHEAD } QueuingDiscType;

/**
 * Basic interface to kernel level TCP Transport.
 */
class UDPTransport : public Transport {

    friend class UDPConnection;

 protected:

    Socket m_ListenSocket;

    int  _Connect_UDP(Socket *pSock, IPEndPoint *otherEnd);
    int  _Send_UDP(IPEndPoint *toWhom, byte* buffer, uint32 length);
    int _ServiceOnce(TimeVal *tv);

    /**
     * Construct an appropriate UDP connection (or subclass) for this 
     * transport.
     */
    virtual UDPConnection *CreateConnection(Socket sock, IPEndPoint *otherEnd);

    /**
     * Add a new connection to all the connection data structures.
     */
    virtual void AddConnection(Connection *conn);

    /**
     * Remove a stale connection from all the connection data structures.
     */
    virtual void RemoveConnection(Connection *conn);

 public:

    static const QueuingDiscType QUEUING_DISCIPLINE = Q_DROPHEAD;
    /** Max datagram size; set by OS */
    static       int MAX_UDP_MSGSIZE;
    /** Message queue size (per connection), # packets */
    static const int APP_QUEUE_SIZE                 = 1024;
    /** Max number of packets to dequeue from kernel at a time in DoWork() */
    static const int MAX_PKTS_SERVICE               = 1280000;

    UDPTransport() {}
    virtual ~UDPTransport() {}

    void  StartListening();
    void  StopListening();
    int   FillReadSet(fd_set *tofill);
    void  DoWork(fd_set *isset, u_long timeout_usecs);
    uint32 GetPriority() { return 10 /* m_ReadPackets */; }	

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
