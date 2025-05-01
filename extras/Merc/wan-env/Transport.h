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

#ifndef __TRANSPORT__H
#define __TRANSPORT__H

#include <mercury/ID.h>
#include <mercury/common.h>
#include <mercury/Scheduler.h>
#include <wan-env/RealNet.h>

class Connection;

///////////////////////////////////////////////////////////////////////////////

typedef map<IPEndPoint, Connection *, less_SID> _conn_hash_t;
typedef _conn_hash_t::iterator _conn_hash_iter;

class ConnectionHash : public _conn_hash_t {
 public:
    Connection *Lookup(IPEndPoint *otherEnd) {
	_conn_hash_iter it = find(*otherEnd);
	if (it == end())
	    return 0;
	else
	    return (*it).second;
    }
    void Insert(IPEndPoint *otherEnd, Connection *conn) {
	insert(value_type(*otherEnd, conn));
    }

    void Flush(IPEndPoint *otherEnd) {
	_conn_hash_iter it = find(*otherEnd);
	if (it != end())
	    erase(it);
    }
};

typedef ConnectionHash::iterator ConnectionHashIter;

typedef list<Connection *> ConnectionList;
typedef ConnectionList::iterator ConnectionListIter;

///////////////////////////////////////////////////////////////////////////////

// Optional options to pass to a transport layer
//typedef map<string, Value, less<string> > TransportOptions;

///////////////////////////////////////////////////////////////////////////////

/**
 * Base class for various transport protocols.
 */
class Transport {
    friend class ResetReadPktsTimer;
 protected:

    RealNet       *m_Network;
    IPEndPoint     m_ID;
    TransportType  m_Proto;

    // These are for convinience... subclasses need not actually use them
    uint32         m_ReadPackets;    // #packets read 
    ConnectionHash m_AppConnHash;    // unique appid -> connection map
    ConnectionList m_ConnectionList; // used as a round-robin recv queue
    Mutex          m_ConnLock;       // lock on connection data structures

    inline void Lock() { 
#ifdef ENABLE_REALNET_THREAD 
	m_ConnLock.Acquire();
#endif 
    }
    inline void Unlock() { 
#ifdef ENABLE_REALNET_THREAD 
	m_ConnLock.Release(); 
#endif
    }

    /**
     * Close and remove all connections from data structures AND
     * delete the connection data structures.
     */
    virtual void _ClearConnections();

    /**
     * Remove all closed connections from data structures AND
     * delete the connection data structures.
     */
    virtual void _CleanupConnections();

    /**
     * Print the connection service queue.
     */
    void _PrintConnections();

    /**
     * Print the connection map.
     */
    void _PrintConnHash();

    Transport();

 public:
    /**
     * All Transport instances must be created from this funcion.
     */
    static Transport *Create(RealNet *net, TransportType protocol, IPEndPoint id);

    /**
     * Lookup a protocol ID by some variant of its string name.
     */
    static TransportType GetProtoByName(const char *proto_name);

    virtual ~Transport() {}

    /**
     * Get the creating network layer.
     */
    RealNet *GetNetwork() { return m_Network; }
    TimeVal& TimeNow (); 

    /**
     * Get the the protocol
     */
    TransportType GetProtocol() { return m_Proto; }

    /**
     * Get this end's app id
     */
    IPEndPoint GetAppID() { return m_ID; }

    /**
     * Increment the number of packets read 
     **/
    void IncrReadPackets() { m_ReadPackets++; }
    uint32 GetReadPackets() { return m_ReadPackets; }

    /** 
     * Begin the listening for the protocol
     */
    virtual void  StartListening() = 0;

    /** 
     * Stop listening 
     */
    virtual void  StopListening() = 0;

    /** 
     * Fill in the fd_set with all the socket descriptors to select on
     *
     * @return the max file descriptor set
     */
    virtual int  FillReadSet(fd_set *tofill)                      = 0;

    /** 
     * Periodically called function in a separate thread. Implementators
     * are responsible for locking. DoWork() should try to respect the 
     * timeout parameter.
     */
    virtual void  DoWork(fd_set *isset, u_long timeoutUsecs)      = 0;

    /**
     * Returns processing priority of the transport. This is used by 
     * RealNet to allocate DoWork() timeslices to each transport. One
     * way to assign priority is to use #packets processed during a 
     * window of time.
     **/
    virtual uint32 GetPriority()                                  = 0;

    /** 
     * Get a (possibly new) connection to a target endpoint. 
     * Returns NULL if we can't connect to the target.
     */
    virtual Connection *GetConnection(IPEndPoint *target)         = 0;

    /** 
     * Get the next connection which has data on it ready to read
     * or other status value (CLOSE, ERROR).
     * 
     * Semantics: If the return value is:
     *
     * CONN_NEWINCOMING - same as CONN_OK, but the connp is new
     * CONN_OK          - a new data packet was read on connp
     * CONN_CLOSED      - connp was closed. You will only get this
     *                    once for each remotely closed connection.
     *                    The transport layer still owns the connection
     *                    and is responsible for garbage collecting it.
     * CONN_ERROR       - connp had an error. The transport layer keeps
     *                    the connection and it remains in ERROR state
     *                    until the caller explicitly closes it.
     * CONN_NOMSG       - no connection is ready. *connp == NULL.
     * CONN_INVALID     - should never happen.
     */
    virtual ConnStatusType GetReadyConnection(Connection **connp) = 0;

    /** 
     * Close a connection. This explicit close will NOT generate
     * a CONN_CLOSED event via GetReadyConnection. This call signals
     * that the connection can be garbage collected.
     */
    virtual void CloseConnection(IPEndPoint *target)              = 0;
};

#endif // __TRANSPORT__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
