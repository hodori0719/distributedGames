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

#ifndef __NETWORKLAYER__H
#define __NETWORKLAYER__H

#include "common.h"

class Message;

// XXX: In an ideal world, I would have liked to have an abstract 
// EID class, and IPEndPoint would be valid only in a real 'wan' 
// implementation. However, doing so creates a whole lot of other
// problems which I want to avoid at this point. - Ashwin [02/14/2005]

// Encapsulates the functionality that any network layer would provide.
//
//  Basically, the application takes care of messages ONLY - alongwith some other
//  notifications such as whether a message was received from a newly established
//  connection or not. (see: ConnStatusType enum in Connection class)
//
//  The advantage of building this layer is that we can switch between simulations
//  and real network testing easily. 

// static const char *g_ProtoStrings[] = { "PROTO_UNRELIABLE", "PROTO_RELIABLE" };

typedef enum {
    PROTO_INVALID, PROTO_UDP, PROTO_TCP, PROTO_TCP_PASSIVE, PROTO_CBR
} TransportType;

static const char * g_TransportProtoStrings[] = {
    "PROTO_INVALID", "PROTO_UDP", "PROTO_TCP", "PROTO_TCP_PASSIVE", "PROTO_CBR" 
};

typedef enum { CONN_OK, CONN_NEWINCOMING, CONN_CLOSING, CONN_CLOSED, 
	       CONN_ERROR, CONN_NOMSG, CONN_INVALID } ConnStatusType;

static const char *g_ConnStatusStrings[] = {
    "CONN_OK", "CONN_NEWINCOMING", "CONN_CLOSING", "CONN_CLOSED", "CONN_ERROR",
    "CONN_NOMSG", "CONN_INVALID"
};

/**
 * Interface for network layer implementations.
 *
 * Implementations of the NetworkLayer should have some notion of an appID
 * IPEndPoint associated with them. That is, sending a message via that 
 * NetworkLayer will be "from" that appID. This should be assigned in the
 * constructor and not changed.
 *
 * XXX TODO: well actually there should be a difference between the appID
 * and the implementation endpoint, otherwise a single network layer can't
 * have more than one TCP or UDP IP::Port. Work on this later... 
 *
 * XXX TODO: There should be a way to expose the underlying connections
 * to the application, not just messages in a random order. Right now
 * the NetworkLayer looks like the thin end of a funnel -- we demux
 * everything into separate Transports, and then merge them all again
 * when passing them up to the application...
 */
class NetworkLayer {
 public:
    virtual ~NetworkLayer () {}

    // XXX TODO: These aren't really used for anything useful... eventually
    // get rid of these and fix TCPConnection so that it doesn't need them

    enum { READ_CLOSE, READ_ERROR, READ_INCOMPLETE, READ_COMPLETE, READ_INVALID };

    static int ReportReadError(int retcode) {
	if (retcode == 0)
	    return READ_CLOSE;
	else
	    return READ_ERROR;
    }

    /**
     * Starts listening for connections for a particular underlying transport 
     * protocol. The protocol will start running on this NetworkLayer's 
     * internal appID.
     *
     * @param proto the Transport protocol to use. This should be one of the
     *              Transport::PROTO_* constants. DO NOT USE PROTO_UNRELIABLE
     *              or PROTO_RELIABLE in NetworkLayer. Although they will map
     *              to their correct counterparts in Transport::PROTO_*, this
     *              is a legacy hack.
     */ 
    virtual void StartListening(TransportType proto) = 0;

    /**
     * Stop listening on all transport protocols.
     */
    virtual void StopListening() = 0;

    /**
     * Sends a message to 'toWhom' using the transport protocol 'protocol'.
     * 'toWhom' represents an application endpoint. The "sender" of the
     * message will be the appID associated with this NetworkLayer.
     *
     * @param msg the message to send
     * @param toWhom the endpoint to send it to.
     * @param proto  the Transport::PROTO_* to use as the transport protocol.
     *               You must call StartListening() with that protocol before
     *               using it to send messages.
     * @return error status (0 == OK, -1 == ERROR)
     */ 
    virtual int SendMessage(Message *msg, IPEndPoint *toWhom, TransportType proto) = 0;

    /**
     * Receives the next message from the network. the network allocates the 
     * message pointer for you. ref_fromWhom should be allocated by the 
     * application.
     * 
     * @param ref_fromWhom the sender of the message, filled in
     * @param ref_msg the message received, filled in
     * @return the status of the connection. The semantics are:
     *
     * CONN_NEWINCOMING - same as CONN_OK, but the connp is new
     * CONN_OK          - ref_msg and ref_fromWhom will be filled in
     *                    with the new message
     * CONN_CLOSED      - the connection associated with ref_fromWhom was
     *                    remotely closed. You will get this message only once.
     * CONN_ERROR       - the connection associated with ref_fromWhom had 
     *                    an error. The network layer keeps
     *                    the connection and it remains in ERROR state
     *                    until the caller explicitly closes it.
     * CONN_NOMSG       - no new messages
     * CONN_INVALID     - should never happen.
     *
     * XXX TODO: there is no way for the caller to know the transport
     *           protocol the message arrived under?
     */ 
    virtual ConnStatusType GetNextMessage(IPEndPoint *ref_fromWhom, 
					  Message **ref_msg) = 0;

    /**
     * Release a message received via GetNextMessage(...).
     *
     * @param msg a message originally received from GetNextMessage()
     */
    virtual void FreeMessage(Message *msg) = 0;

    /**
     * Explicitly close a "connection" to the endpoint (e.g., if it had
     * an error).
     *
     * @param otherEnd the target of the connection
     * @param proto the transport protocol (Transport::PROTO_*) that
     *              the connection is using.
     */
    virtual void CloseConnection(IPEndPoint *otherEnd, TransportType proto) = 0;

};

#endif // __NETWORKLAYER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
