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

#ifndef __CONNECTION__H
#define __CONNECTION__H

#include <mercury/common.h>
#include <mercury/NetworkLayer.h>
#include <mercury/IPEndPoint.h>

class Transport;
class Message;
class RealNet;
class Packet;

/**
 * Auxiliary information returned with each packet.
 */
struct PacketAuxInfo
{
    TimeVal timestamp;
};

/**
 * Base class for Transport connections.
 */
class Connection {
    friend class DelayedTransport;
    friend ostream& operator<<(ostream& out, Connection *con);

    Transport     *m_Transport;              // owning transport

    Socket         m_Socket;                 // Socket for this connection
    IPEndPoint     m_SocketPeerAddress;      // 'getpeername' 
    IPEndPoint     m_AppPeerAddress;         // "App level" ID of the other end

    ConnStatusType m_Status;                 // My status: closed/error/etc...

 protected:

    Connection(Transport *t, Socket sock, IPEndPoint *otherEnd);

    /**
     * This is called to send a data packet filled by the application.
     * The connection becomes the owner of the packet.
     *
     * @return error status
     */
    virtual int Send(Packet *tosend) = 0;

    /**
     * This is called to get the next message when a READ_COMPLETE is
     * returned from Read() (or anytime after). NULL if none.
     *
     * @param aux packet auxiliary information (filled in)
     */
    virtual Packet *GetNextPacket(PacketAuxInfo *aux) = 0;

    /**
     * Free a packet returned by GetNextPacket();
     */
    virtual void FreePacket(Packet *pkt) = 0;

    virtual void SetSocketPeerAddress();
    virtual void SetSocketPeerAddress(IPEndPoint *addr) { 
	m_SocketPeerAddress = *addr;
    }

 public:

    virtual ~Connection();

    /**
     * Get the creating transport protocol.
     */
    Transport *GetTransport() {
	return m_Transport;
    }

    /**
     * Get the appID of the remote endpoint of this connection. This is
     * the IPEndPoint that should be used to communicate to this end point.
     */
    IPEndPoint *GetAppPeerAddress() {
	return &m_AppPeerAddress;
    }

    /**
     * Get the actual IP:Port endpoint of the remote end of this connection.
     * This may be different from the appID (e.g., may be just a random
     * port assigned via the kernel).
     */
    IPEndPoint *GetSocketPeerAddress() {
	return &m_SocketPeerAddress;
    }

    /**
     * Set the appID of the remote endpoint of this connection
     */
    void SetAppPeerAddress(IPEndPoint *otherEnd) {
	m_AppPeerAddress = *otherEnd;
    }

    /**
     * Get the associated socket.
     */
    Socket GetSocket() {
	return m_Socket;
    }

    /**
     * Get the associated transport protocol.
     */
    int GetProtocol();

    /**
     * Get the status of this connection.
     */
    ConnStatusType GetStatus() {
	return m_Status;
    }

    /**
     * Set the status of this connection.
     */
    void SetStatus(ConnStatusType status) {
	m_Status = status;
    }

    /**
     * Public interface to send packets via this connection.
     */
    int SendMessage(Packet *tosend);

    /**
     * If this connection is ready (e.g., the transport returned it
     * as the next ready connection), then call this to retrieve the
     * latest message received. Do NOT call it otherwise, as it may
     * block.
     */
    Message *GetLatestMessage();

    void Print(FILE* stream);
};

ostream& operator<<(ostream& out, Connection *con);

#endif // __CONNECTION__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
