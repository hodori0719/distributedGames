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

/***************************************************************************
  MercuryNode.h 

begin          : Nov 2, 2002
copyright      : (C) 2002-2003 Ashwin R. Bharambe( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz( jweisz@cs.cmu.edu )

***************************************************************************/


#ifndef __MERCURYNODE__H
#define __MERCURYNODE__H

#include <mercury/common.h>
#include <mercury/Router.h>
#include <mercury/NetworkLayer.h>
#include <mercury/Scheduler.h>
#include <mercury/Message.h>
#include <mercury/Timer.h>
#include <mercury/Node.h>
#include <mercury/Application.h>
#include <list>
#include <hash_map.h>

    static const uint32 PEER_HBEAT_INTVL     = 500;   // milliseconds
static const uint32 BOOTSTRAP_HBEAT_INTVL= 2000;  // don't trouble the bootstrap server too much. (perhaps some randomization should be done)
static const uint32 WAIT_ITERATIONS      = 10;    // how many heartbeats to miss before u start panicking

static const uint32 JOIN_REQ_INTERVAL    = 500;   // Wait for 500 milliseconds before sending out new joinrequest.
static const uint32 MAX_JOIN_TRIES       = 10;    // #tries before you give up!

class Cache;

// Declarations; we don't need the actual includes here!
class BufferManager;
class MercuryNode;
class MemberHub;
class Peer;
class HubManager;
class Sampler;
class LoadSampler;

typedef hash_map<MsgType, vector<MessageHandler *> > HandlerMap;

// figure out how to turn these off if it's running in a WAN environment
// these macros have been written mostly for use from the simulator environ

#define MINFO   m_MercuryNode->croak (DBG_MODE_INFO, 0, __FILE__, __FUNCTION__, __LINE__)
#define MWARN   m_MercuryNode->croak (DBG_MODE_WARN, 0, __FILE__, __FUNCTION__, __LINE__)

#define MTINFO  MINFO << "[" << Debug::GetFormattedTime () << "]"
#define MTWARN  MWARN << "[" << Debug::GetFormattedTime () << "]"

#ifdef DEBUG
#define MDB(lvl) if (g_VerbosityLevel >= lvl) m_MercuryNode->croak (DBG_MODE_DEBUG, lvl, __FILE__, __FUNCTION__, __LINE__)
#else
#define MDB(lvl) 1 ? cerr : cerr
#endif

#define MDBG MDB(5)

#define MTDB(lvl)  MDB(lvl) << "[" << Debug::GetFormattedTime() << "]"

class MercuryNode : public Node, public Router, public MessageHandler {
    friend class MercuryNodePeriodicTimer;

 private:
    BufferManager         *m_BufferManager;
    HubManager            *m_HubManager;
    bool                   m_AllJoined;        // bootstrap tells us when everyone is joined

    int                    m_Epoch;            // repair epoch
    HandlerMap             m_HandlerMap;

    MercuryNode           *m_MercuryNode;      // hack for debug
    Application           *m_Application;
    TimeVal                m_StartTime;
 protected:
    bool                   m_Simulating; 
 public:
    MercuryNode(NetworkLayer *network, Scheduler *scheduler, IPEndPoint& addr);
    ~MercuryNode();

    bool IsJoined();
    bool AllJoined() { return m_AllJoined; }
    uint32 GetIP() { return m_Address.GetIP(); }
    uint16 GetPort() { return m_Address.GetPort(); }


    /**
     * Interface offered to the applications.
     * Perhaps, there should be some other class
     * encapsulating this!
     **/

    /**
     * Start the mercury node. This call starts the mercury
     * bootstrap process.
     **/
    void Start ();

    /**
     * Stop the mercury node. This stops all the network 
     * processing done by mercury. Messages are not accepted
     * any longer.
     **/
    void Stop ();

    /**
     * Send a publication through Mercury. Mercury does _not_ 
     * own the 'pub' -- it's the applications responsibility
     * to delete it. (Mercury makes an internal copy.)
     **/
    void SendEvent (Event *pub);

    /**
     * Register a subscription through Mercury. It is the 
     * application's responsibility to delete the 'sub'. 
     **/
    void RegisterInterest (Interest *sub);

    /**
     * Read the next "matched" event (i.e., event in the 
     * system which has matched our registered subscription(s))
     * Returns NULL if no event is present.
     **/
    Event* ReadEvent ();

    /**
     * Register an application to handle callbacks 
     *
     **/
    void RegisterApplication (Application *app);

    /**
     * returns a vector of constraints - each one is the triple
     *    [index,  absmin,  absmax]
     **/    
    vector<Constraint> GetHubConstraints ();

    /**
     * returns a vector of the hub names - each one is the pair
     * [index, name]
     **/
    vector< pair<int,string> > GetHubNames ();

    /**
     * returns a vector of current ranges for each of the hubs 
     * in the schema. a triple (index, -1, -1) reflects that 
     * this node is not a member of the corresponding hub. a 
     * triple (index, 0, 0) indicates that this node is a member,
     * but is currently not part of the routing circle at the 
     * moment.
     **/
    vector<Constraint> GetHubRanges ();

    /**
     * Returns a vector of successors for the hub @hubid. If 
     * nothing is returned => we are not joined in the hub at
     * this point.
     **/
    vector<Neighbor> GetSuccessors (int hubid);

    /**
     * Returns predecessors. As of now, it can return a zero length
     * vector (meaning there's no predecessor known) or a 1-length one.
     **/
    vector<Neighbor> GetPredecessors (int hubid);

    /**
     * Returns a vector of `long neighbors' for the hub @hubid.
     **/
    vector<Neighbor> GetLongNeighbors (int hubid);

    /**
     * (Un)Register a metric sampler class. Mercury will perform 
     * random-walk based sampling for the application. Returns
     * 0 on success, -1 on error. XXX: do better error notification.
     **/
    int RegisterSampler (int hubid, Sampler *s);
    int UnRegisterSampler (int hubid, Sampler *s);


    /**
     * Change mercury's default load sampler (the default load 
     * sampler uses PubsubRouter::GetRoutingLoad () as its definition
     * of load.
     **/
    int RegisterLoadSampler (int hubid, LoadSampler *s);
    int UnRegisterLoadSampler (int hubid, LoadSampler *s);

    /**
     * Returns samples collected by Mercury for the metric
     * indicated by 'Sampler *s'. The memory for the samples 
     * is owned by Mercury, and the application should make
     * copies, if necessary. Returns 0 on success, -1 on 
     * error.
     **/
    int GetSamples (int hubid, Sampler *s, vector<Sample *> *ret);

    void LockBuffer() { ASSERT(0); }
    void UnlockBuffer() { ASSERT(0); }

    void Print(FILE *stream);

    virtual void DoWork (u_long timeout) {}      

    void PrintSubscriptionList (ostream& stream);
    void PrintPublicationList (ostream& stream);

    void RegisterMessageHandler(MsgType type, MessageHandler *handler);

    virtual void ReceiveMessage (IPEndPoint *from, Message *msg);
    void ProcessMessage (IPEndPoint *from, Message *msg);
    HubManager *GetHubManager() { return m_HubManager; }
    Application *GetApplication () { return m_Application; }

    ostream& croak (int mode, int lvl = 0, const char *file = NULL, const char *func = NULL, int line = 0);
    void SendApplicationPackets( void );

    void SetStartTime (TimeVal& t) { m_StartTime = t; }
    TimeVal& GetStartTime () { return m_StartTime; }
    float GetRelativeTime () { 
	if (m_StartTime == TIME_NONE)
	    return 0.0;
	return (float) (m_Scheduler->TimeNow () - m_StartTime) / (float) (MSEC_IN_SEC); 
    }

    void DoPeriodic ();
 protected:
    bool SendPacket ();
 private:

    MemberHub *GetHub (int hubid);
    void HandleAllJoined(IPEndPoint *from, MsgCB_AllJoined *msg);
    void HandlePublication (IPEndPoint *from, MsgPublication *msg);
    void PrintPeerList(FILE *stream);
};


#endif // __MERCURYROUTER__H

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
