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

#ifndef __REALNET__H
#define __REALNET__H

typedef float bwidth_t;    // XXX HACK: - Ashwin [03/06/2006]
#include <util/ExpLog.h>
#include <mercury/ID.h>
#include <util/TimeVal.h>
#include <mercury/NetworkLayer.h>
#include <mercury/IPEndPoint.h>
#include <map>
#include <list>
#include <wan-env/Connection.h>
#include <wan-env/Transport.h>
#include <mercury/RoutingLogs.h>
#include <sys/poll.h>

//#define ENABLE_TEST 1

//
// If defined, then we use a separate thread to periodically call DoWork()
// to process pending RealNet events. If not defined, the application is
// responsible for periodically calling DoWork().
//
//#define ENABLE_REALNET_THREAD 1
#undef ENABLE_REALNET_THREAD

///////////////////////////////////////////////////////////////////////////////
//////// For estimating bandwidth usage. *Very* simple moving window based
//////// approach right now.

// how large each time bucket will be in msec
// (aggregate all packets into bucket this size)
#define BUCKET_SIZE 100

typedef struct {
    TimeVal time;
    uint32  size;
} Measurement;

typedef list<Measurement> Window;
typedef Window::iterator WindowIter;

///////////////////////////////////////////////////////////////////////////////
// Implements a sockets based network layer
///////////////////////////////////////////////////////////////////////////////

struct ProtoID {
    TransportType proto;
    IPEndPoint id;

    ProtoID(IPEndPoint id, TransportType proto) : id(id), proto(proto) {}
};

struct less_ProtoID {
    less_SID sid_cmp;

    bool operator() (const ProtoID& a, const ProtoID& b) const {
	return a.proto < b.proto ||
	    ( a.proto == b.proto && sid_cmp(a.id, b.id) );
    }
};

typedef map<ProtoID, Transport *, less_ProtoID> TransportMap;
typedef TransportMap::iterator TransportMapIter;

typedef set<ProtoID, less_ProtoID> ProtoIDSet;
typedef ProtoIDSet::iterator ProtoIDSetIter;

///////////////////////////////////////////////////////////////////////////////

class RealNetWorker;
class Scheduler;

#include <util/debug.h>
#define MAX_FILE_DESC 40960      // 40K file descriptors!

class RealNet : public NetworkLayer {
    friend class Connection;
    friend class Transport;
    friend class RealNetWorker;

    ///// GLOBAL VARIABLES /////

    //
    // The timeout on each call to select (in usec). This is the maximum time 
    // the scheduler thread can block when there is potentially work to do
    // on each transport (e.g., if a transport is ready to send new messages).
    // A transport can manually interrupt a blocking select call with the
    // InterruptWorker() method.
    //
    // The purpose of this timeout is to prevent the scheduling thread from
    // consuming too much CPU by spinning in an idle DoWork() loop.
    //
    static const uint32          SELECT_TIMEOUT = 10; // msec
    static const uint32          POLL_TIMEOUT_MILLIS = 10; // msec

    static RealNetWorker        *m_WorkerThread;
    static Mutex                 m_Lock;
    static TransportMap          m_Transports;
    static fd_set                m_ReadFileDescs;      // for select
    static struct pollfd         m_PollFileDescs [MAX_FILE_DESC];      // for poll

    //
    // Start the singleton worker thread
    //
    static void InitWorker();

    inline static void Lock() { 
#ifdef ENABLE_REALNET_THREAD
	m_Lock.Acquire(); 
#endif
    }
    inline static void Unlock() {
#ifdef ENABLE_REALNET_THREAD 
	m_Lock.Release(); 
#endif
    }

    static Transport *_LookupTransport(const ProtoID& proto);
    static void       _DoSelect(TimeVal timeout);

#ifndef ENABLE_REALNET_THREAD
 public:
#endif
    //
    // Periodically called in worker thread 
    //
    static void DoWork (u_long timeout = 10 /* millis */);
    static void DoWorkUsec (u_long usecs);
    static Socket FillReadSet(fd_set *tofill);

 public:

    //
    // When a new socket is added that we might receive data on, we may have
    // to interrupt the current select call
    //
    static void InterruptWorker();

 private:

    ///////////////////////////////////////////////////////////////////////////

    Scheduler            *m_Scheduler;
    IPEndPoint            m_AppID;  // ID for this RealNet instance
    ProtoIDSet            m_Protos; // the set of transports used

    ///////////////////////////////////////////////////////////////////////////

    bool m_RecordBandwidthUsage;

    const uint32 m_WindowSize;    // the size of the bandwidth window in msec

    Window m_InboundWindow;       // the bandwidth usage tracking windows
    Window m_OutboundWindow;

    bool m_EnableMessageLog;

    MeasurementMap m_InboundAggregates, m_OutboundAggregates;
    uint32 m_SentMessages, m_RecvMessages;
    TimeVal m_StartTime;

    Scheduler *GetScheduler () { return m_Scheduler; }
 public:

    /**
     * @param the application-level ID to associate with this RealNet instance.
     *
     * @param recordBwidthUsage track bandwidth usage over the last
     * windowSize milliseconds.
     *
     * @param windowSize the window to track bandwidth usage over (ms).
     */
    RealNet(Scheduler *sched, IPEndPoint appID, bool recordBwidthUsage = false, 
	    uint32 windowSize = 1000);
    virtual ~RealNet();

    IPEndPoint GetAppID() {
	return m_AppID;
    }

    //
    // Implement the NetworkLayer methods
    //
    void StartListening(TransportType proto);
    void StopListening();

    ConnStatusType GetNextMessage(IPEndPoint *ref_fromWhom, Message **ref_msg);
    void FreeMessage(Message *msg);

    void CloseConnection(IPEndPoint *otherEnd, TransportType proto);

    int SendMessage(Message *msg, IPEndPoint *toWhom, TransportType proto);

    bwidth_t GetOutboundUsage(TimeVal& now); /* in bytes/sec */
    bwidth_t GetInboundUsage(TimeVal& now);  /* in bytes/sec */

    /// Log messages sent and received by this network layer
    void EnableLog() { 
	ASSERT(g_MeasurementParams.enabled);
	m_EnableMessageLog = true; 
    }
    void DoAggregateLogging ();
    void UpdateSendRecvStats ();

    //
    // Bunch of utilities
    //
    static int    ReadDatagram (Socket sock, IPEndPoint *fromWhom,
				byte *buffer, int length);
    static int    ReadDatagramTime (Socket sock, IPEndPoint *fromWhom,
				    byte *buffer, int length, TimeVal *tv);
    static int    WriteDatagram (Socket sock, IPEndPoint *toWhom,
				 byte *buffer, int length);
    static int    WriteBlock   (Socket sock, byte *buffer, int length);
    static int    ReadNoBlock  (Socket sock, byte *buffer, int length);
    static int    ReadBlock    (Socket sock, byte *buffer, int length);
    static bool   IsDataWaiting(Socket sock);
    static bool   WaitForWritable(Socket sock, TimeVal *t);

#ifdef ENABLE_TEST
    static void Test(int argc, char **argv);
#endif

 private:

    int    _SendMessage(Message *msg, Connection *connection);

    void   RecordOutbound(uint32 size, TimeVal& now);
    void   RecordInbound(uint32 size, TimeVal& now);
};

// Run a separate (global) thread for periodic processing
class RealNetWorker : public Thread
{
 public:
    RealNetWorker() {}
    virtual ~RealNetWorker() {}

    void Run() {
	// This is an independent thread; sleep as much as is possible!
	while (true)
	    RealNet::DoWork (100);
    }
};

#endif // __REALNET__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
