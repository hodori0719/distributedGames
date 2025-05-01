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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

#ifndef __OBJECT_LOGS_H__
#define __OBJECT_LOGS_H__

#include <mercury/IPEndPoint.h>
#include <mercury/ID.h>
#include <util/types.h>
#include <util/ExpLog.h>

    // This must be enabled for the DiscoveryLatLog
#define ENABLE_OBJECT_LOGS 1

    ///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_OBJECT_LOGS
#define OBJECT_LOG_DO(blurb) do { blurb; } while (0);
#else 
#define OBJECT_LOG_DO(blurb)
#endif

/**
 * This log is used to track the time it takes to discover a new object
 * we are interested in. Since a sub can result in multiple pubs, once
 * we get the pub, we also track the GUID. (GUID == GUID_NONE in subs)
 * For pubs and subs also track the number of hops traversed. The record
 * is taken when we receive the message. (INIT is when we submit the sub)
 *
 * Starting from a Publication
 * ===========================
 * (1)    PUB_INIT ... PUB_SEND ALIAS* =>
 * (R)    PUB_STORE ... 
 *
 * Starting from a Subscription
 * ============================
 * (1)    SUB_INIT ... SUB_SEND =>
 * (R) => SUB_STORE ... 
 *
 * ...
 *
 * (R)    MATCH_SEND ... ALIAS* =>
 * (1) => PUB_RECV ... REG_SEND =>
 * (2) => REG_RECV ... UPDATE_SEND =>
 * (1) => UPDATE_RECV ... UPDATE_DONE
 *
 * (1) = submitter node
 * (R) = redezvous point
 * (2) = primary node
 *
 * ALIAS (using the second constructor) matches multiple SUB_STORE entries
 * with multiple PUB_STORE entries, because a pub can be matched to multiple
 * subs (and vis versa) on the same node. ALIAS is also used to match the
 * initial PUB_SEND to the multiple pubs that get sent to each hub.
 */
struct DiscoveryLatEntry : public ExpLogEntry
{
    enum { SUB_INIT, SUB_SEND, SUB_STORE, 
	   PUB_INIT, PUB_SEND, PUB_STORE,
	   ALIAS, MATCH_SEND, PUB_RECV, 
	   REG_SEND, REG_RECV, 
	   UPDATE_SEND, UPDATE_RECV, UPDATE_DONE, 
	   PUB_ROUTE_RECV, PUB_ROUTE_SEND,
	   SUB_ROUTE_RECV, SUB_ROUTE_SEND
    };

    byte   type;
    int    hops;
    uint32 nonce;
    uint32 alias;

    DiscoveryLatEntry() {};
    DiscoveryLatEntry(int type, int hops, uint32 nonce) : 
	type((byte)type), hops(hops), nonce(nonce), alias(0) {}
    /**
     * @param nonce the original sub nonce
     * @param alias the new nonce to track it with
     */
    DiscoveryLatEntry(int type, int hops, uint32 nonce, uint32 alias) : 
	type((byte)type), hops(hops), nonce(nonce), alias(alias) {}
    virtual ~DiscoveryLatEntry() {};

    /**
     * Override this to mean: sample for a window of <size> seconds every
     * floor(1/<rate>) windows.
     */
    bool Sample(float rate, float size) {
	if (rate >= 1.0)
	    return true;

	int windex = (int)(time.tv_sec/(double)size);
	int modulo = (int)(1.0/rate);
	return (windex % modulo) == 0;
    }

    uint32 Dump(FILE *fp) {
	if (!g_MeasurementParams.binaryLog) 
	{
	    if (alias == 0) {
		return fprintf(fp, "%13.3f\t%d\t%d\t%08X\n",
			       (double)time.tv_sec + 
			       (double)time.tv_usec/USEC_IN_SEC,
			       (int)type, hops, nonce);
	    } else {
		return fprintf(fp, "%13.3f\t%d\t%d\t%08X\t%08X\n",
			       (double)time.tv_sec + 
			       (double)time.tv_usec/USEC_IN_SEC,
			       (int)type, hops, nonce, alias);
	    }
	}
	else 
	{
	    double t = (double)time.tv_sec + 
		(double)time.tv_usec/USEC_IN_SEC;

	    fwrite((void *) &t, sizeof(double), 1, fp);
	    fwrite((void *) &type, sizeof(byte), 1, fp);
	    fwrite((void *) &hops, sizeof(int), 1, fp);
	    fwrite((void *) &nonce, sizeof(uint32), 1, fp);
	    fwrite((void *) &alias, sizeof(uint32), 1, fp);

	    // Assume we wrote all successfully :)
	    return sizeof(double) + sizeof(byte) + sizeof(int)
		+ sizeof(uint32) + sizeof(uint32);
	}
    }
};

struct ReplicaLifetimeEntry : public ExpLogEntry
{
    enum { OBJINFO  =  'i', // when we construct the GObjectInfo
	   MCREATE  =  'm', // replica created via migrating a prim
	   DCREATE  =  'c', // replica created via delta
	   PCREATE  =  'p', // replica created via pub
	   FCREATE  =  'f', // replica created via fetch 
	   STORE    =  's', // replica entered object store
	   MDESTROY =  'y', // replica destroyed by migrating a prim
	   TDESTROY =  'd', // replica destroyed by timeout
	   EDESTROY =  'e', // replica destroyed by explicit delta
    };

    char type;
    GUID guid;

    ReplicaLifetimeEntry() {};
    virtual ~ReplicaLifetimeEntry() {}

    uint32 Dump(FILE *fp) {
	return fprintf(fp, "%13.3f\t%s\t%c\n",
		       (double)time.tv_sec + 
		       (double)time.tv_usec/USEC_IN_SEC,
		       guid.ToString(), type);
    }
};

#ifdef PUBSUB_DEBUG
#include <sstream>
#include <mercury/Interest.h>

struct DebugEntry : public ExpLogEntry {
    char msg[1024];

    DebugEntry() {}
    DebugEntry(bool trigger, Interest *in, Event *ev) {
	stringstream s;
	s << "is_trigger=" << trigger;
	if (in)
	    s << " {interest}=> " << in;
	s << endl;
	if (ev)
	    s << "\t{event}=> " << ev << endl;
	s << "----" ;

	strncpy(msg, s.str().c_str(), 1023);
    }
    virtual ~DebugEntry() {}

    uint32 Dump(FILE *fp) {
	return fprintf(fp, "%13.3f\t%s\n", 
		       (double)time.tv_sec + (double)time.tv_usec/USEC_IN_SEC,
		       msg);
    }
};
#endif

///////////////////////////////////////////////////////////////////////////////

DECLARE_EXPLOG(DiscoveryLatLog, DiscoveryLatEntry);
DECLARE_EXPLOG(ReplicaLifetimeLog, ReplicaLifetimeEntry);
#ifdef PUBSUB_DEBUG
DECLARE_EXPLOG(DebugLog, DebugEntry);
#endif

extern void InitObjectLogs(IPEndPoint sid);

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
