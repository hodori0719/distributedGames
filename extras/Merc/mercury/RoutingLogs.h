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
  RoutingLogs.h

begin           : Nov 8, 2003
version         : $Id: RoutingLogs.h 2472 2005-11-13 01:34:54Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Ashwin Bharambe (ashu@cs.cmu.edu)

***************************************************************************/

#ifndef __ROUTING_LOGS_H__
#define __ROUTING_LOGS_H__

#include <mercury/IPEndPoint.h>
#include <mercury/Message.h>
#include <util/ExpLog.h>
#include <map>

    struct AggMeasurement {
	int  size;
	byte hopcount;
	int  nsamples;
    };

struct AggMeasurementEntry {
    byte key;
    AggMeasurement val;
};

typedef map<byte, AggMeasurement> MeasurementMap;

// maximum number of msg types we will log every aggregation interval (+1)
// better to overestimate (but we won't crash if it won't fit -- we'll
// print a warning and throw in the rest in an 'invalid' 255 bin)
#define MAX_MSG_TYPES 16 // if > 255, make sure to change the nxxx size below

struct AggregateMessageEntry : public ExpLogEntry 
{
    enum { INBOUND = 0,   OUTBOUND = 1 };
    enum { PROTO_TCP = 0, PROTO_UDP = 1 };

    byte ninbound, noutbound;
    AggMeasurementEntry inbound[MAX_MSG_TYPES];
    AggMeasurementEntry outbound[MAX_MSG_TYPES];

    void glob(AggMeasurement& orig, const AggMeasurement& add)
	{
	    orig.size     += add.size;
	    orig.hopcount += add.hopcount; // this allows us to still get avg
	    orig.nsamples += add.nsamples;
	}

    AggregateMessageEntry () {}
    AggregateMessageEntry (MeasurementMap& inbound, MeasurementMap& outbound)
	{
	    ninbound  = 0;
	    noutbound = 0;

	    AggMeasurementEntry *e = &this->inbound[0];
	    for (MeasurementMap::iterator it = inbound.begin (); 
		 it != inbound.end (); ++it, ++e) {
		if (it->second.nsamples <= 0)
		    continue;

		if (e < &this->inbound[MAX_MSG_TYPES]) {
		    e->key = it->first;
		    e->val = it->second;
		    ninbound++;
		} else {
		    --e;
		    // couldn't fit! glob all the rest together
		    e->key = MSG_INVALID;
		    glob(e->val, it->second);
		}
	    }

	    e = &this->outbound[0];
	    for (MeasurementMap::iterator it = outbound.begin (); 
		 it != outbound.end (); ++it, ++e) {
		if (it->second.nsamples <= 0)
		    continue;

		if (e < &this->outbound[MAX_MSG_TYPES]) {
		    e->key = it->first;
		    e->val = it->second;
		    noutbound++;
		} else {
		    --e;
		    // couldn't fit! glob all the rest together
		    e->key = MSG_INVALID;
		    glob(e->val, it->second);
		}
	    }
	}

    uint32 Dump (FILE *fp) {
	uint32 ret = 0;

	for (AggMeasurementEntry *e = &inbound[0]; 
	     e < &inbound[ninbound]; e++) {
	    byte type = e->key;
	    const AggMeasurement& msr = e->val;

	    ASSERT(msr.nsamples > 0);

	    ret += fprintf(fp, "%13.3f\t%d\t%d\t%d\t%d\t%8.2f\n",
			   (double)time.tv_sec + 
			   (double)time.tv_usec/USEC_IN_SEC,		    
			   INBOUND, 
			   (int) type, msr.size, msr.nsamples, 
			   ((float)msr.hopcount / (float)msr.nsamples) );
	}

	for (AggMeasurementEntry *e = &outbound[0]; 
	     e < &outbound[noutbound]; e++) {
	    byte type = e->key;
	    AggMeasurement msr = e->val;

	    ASSERT(msr.nsamples > 0);

	    ret += fprintf(fp, "%13.3f\t%d\t%d\t%d\t%d\t%8.2f\n",
			   (double)time.tv_sec + 
			   (double)time.tv_usec/USEC_IN_SEC,		    
			   OUTBOUND, 
			   (int) type, msr.size, msr.nsamples, 
			   ((float)msr.hopcount / (float)msr.nsamples));
	}
	return ret;
    }
};

struct MessageEntry : public ExpLogEntry
{
    enum { INBOUND = 0,   OUTBOUND = 1 };
    enum { PROTO_TCP = 0, PROTO_UDP = 1 };

    byte dir;
    byte proto;
    int type;
    uint32 size;
    uint32 nonce;
    uint16 hopcount;       // ashwin

    MessageEntry() {}
    MessageEntry(int dir, int proto, int nonce, int type, uint32 size, uint16 hopcount) :
	dir((byte)dir), proto((byte)proto),
    type(type), nonce(nonce), size(size), 
    hopcount(hopcount) {}

    uint32 Dump(FILE *fp) {
	if (!g_MeasurementParams.binaryLog) 
	{
	    return fprintf(fp, "%08X\t%13.3f\t%d\t%d\t%d\t%d\t%d\n",
			   nonce, 
			   (double)time.tv_sec + 
			   (double)time.tv_usec/USEC_IN_SEC,		    
			   (int)dir, (int)proto, type, (int)size, (int) hopcount);
	}
	else 
	{
	    fwrite((void *) &nonce, sizeof(uint32), 1, fp);
	    double t = (double)time.tv_sec + 
		(double)time.tv_usec/USEC_IN_SEC;
	    fwrite((void *) &t, sizeof(double), 1, fp);
	    fwrite((void *) &dir, sizeof(byte), 1, fp);
	    fwrite((void *) &proto, sizeof(byte), 1, fp);
	    fwrite((void *) &type, sizeof(int), 1, fp);
	    fwrite((void *) &size, sizeof(uint32), 1, fp);
	    fwrite((void *) &hopcount, sizeof(uint16), 1, fp);

	    // Assume we wrote all successfully :)
	    return sizeof(uint32) + sizeof(double) + sizeof(byte) + sizeof(byte)
		+ sizeof(int) + sizeof(uint32) + sizeof(uint16);
	}
    }
};

struct LoadBalEntry : public ExpLogEntry
{
    static const char LEAVE = 'l';
    static const char JOIN = 'j';
    static const char PREDADJ = 'p';
    static const char SUCCADJ = 's';
    static const char LOADREP = '#';

    char evtype;
    char addr[80];
    union {
	struct {
	    float load; float avg;
	};
	struct {
	    int nitems; int size;
	};
    };

    LoadBalEntry() {}
    LoadBalEntry(char evtype, int nitems, int size, const IPEndPoint& ip) : 
	evtype (evtype), nitems (nitems), size (size) 
	{
	    ip.ToString (addr);
	}
    LoadBalEntry(char evtype, float load, float avg) : 
	evtype (evtype), load (load), avg (avg) 
	{
	    strcpy (addr, "NA");
	}

    uint32 Dump(FILE *fp) {
	uint32 c = 0;
	c += fprintf(fp, "%13.3f\t%c\t",
		     (double)time.tv_sec + 
		     (double)time.tv_usec/USEC_IN_SEC,
		     evtype);
	if (evtype == LOADREP) 
	    c += fprintf(fp, "%.3f\t%.3f\t%s\n", load, avg, addr);
	else
	    c += fprintf(fp, "%d\t%d\t%s\n", nitems, size, addr);
	return c;
    }
};

///////////////////////////////////////////////////////////////////////////////

DECLARE_EXPLOG(MessageLog, MessageEntry);
DECLARE_EXPLOG(AggregateMessageLog, AggregateMessageEntry);
DECLARE_EXPLOG(LoadBalanceLog, LoadBalEntry);

extern void InitRoutingLogs(IPEndPoint sid);

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
