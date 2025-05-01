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
/* -*- Mode:c; c-basic-offset:4; tab-width:4; indent-tabs-mode:nil -*- */

/***************************************************************************
  BootstrapNode.h

begin                : March 11, 2003
copyright            : (C) 2003-2005 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2005 Jeff Pang (jeffpang@cs.cmu.edu )

**************************************************************************/

#ifndef __BOOTSTRAPNODE__H
#define __BOOTSTRAPNODE__H

#include <list>
#include <vector>
#include <mercury/common.h>
#include <mercury/ID.h>
#include <mercury/Constraint.h>
#include <mercury/Message.h>
#include <mercury/NetworkLayer.h>
#include <mercury/Scheduler.h>
#include <mercury/Node.h>
#include <util/Options.h>

    struct NodeState {
	int         m_Level;
	IPEndPoint  m_Addr;
	NodeRange  *m_Range;
	TimeVal     m_tLastHeartBeat;
	// a node is joined after bootstrap and after integrating into the hub
	bool        m_IsJoined;

	NodeState() {}
	~NodeState () {
	    if (m_Range) 
		delete m_Range;
	}

	NodeState(IPEndPoint addr, TimeVal t) : 
	    m_Addr(addr), m_Range (0), m_tLastHeartBeat(t),  m_IsJoined(false), m_Level (0)
	    {}

	NodeState (const NodeState& os) 
	    : m_Addr (os.GetAddress ()), m_tLastHeartBeat (os.m_tLastHeartBeat), m_IsJoined (os.IsJoined ()), m_Level (os.GetLevel ())
	    {
		if (os.GetRange () != NULL)
		    m_Range = new NodeRange (*os.GetRange ());
		else
		    m_Range = NULL;
	    }

	NodeState& operator= (const NodeState& os) {
	    if (this == &os) return *this;

	    m_Addr = os.GetAddress ();
	    m_tLastHeartBeat = os.m_tLastHeartBeat;
	    m_IsJoined = os.IsJoined ();
	    m_Level = os.GetLevel ();

	    if (os.GetRange () != NULL)
		m_Range = new NodeRange (*os.GetRange ());
	    else
		m_Range = NULL;

	    return *this;
	}    

	NodeRange *GetRange () const { return m_Range; }
	void SetRange(NodeRange *r) { 
	    if (m_Range)
		delete m_Range;
	    m_Range = new NodeRange (*r); 
	}

	const IPEndPoint& GetAddress () const { return m_Addr; }
	int GetLevel () const { return m_Level; }
	void SetLevel (int level) { m_Level = level; }

	void SetIsJoined() { m_IsJoined = true; }
	bool IsJoined() const { return m_IsJoined; }
    };

typedef list<NodeState> NodeList;
typedef NodeList::iterator NodeListIter;

class Histogram;

struct BootstrapHubInfo {
    string       m_Name;
    Value        m_AbsMin;
    Value        m_AbsMax;
    NodeList     m_Nodelist;

    Histogram   *m_Histogram;

    BootstrapHubInfo(const string& name, Value& abs_min, Value& abs_max) :
	m_Name(name), m_AbsMin(abs_min), m_AbsMax(abs_max)
	{}
};

typedef vector<BootstrapHubInfo *> BootstrapHubInfoVec;
typedef BootstrapHubInfoVec::iterator BootstrapHubInfoVecIter;

static const uint32 HUB_SPARSENESS_THRESHOLD = 20000;       // if a hub does not contain these many people, we REQUIRE
// an incoming node to accept responsibility for the hub.
struct _bootstrap_prefs_t {
    bool haveHistograms;
    char schemaFile[1024];
    int  nBuckets;
    int  nServers;
    bool oneNodePerHub;
    char choosingPolicy[80];
    char identMapFile[80];
};

extern struct _bootstrap_prefs_t g_BootstrapPreferences;
extern OptionType g_BootstrapOptions[];

class BootstrapNode : public Node {
    friend class PeriodicTimer;

    BootstrapHubInfoVec   m_HubInfoVec;
    int m_NumAssigned;
    set<IPEndPoint, less_SID> m_BootedServers;

    gmp_randstate_t m_GMPRandState;        // for large random numbers
    bool m_AllJoined;

    // Jeff: added this data structure to allow the bootstrap to
    // force nodes to join at particular locations in the ring
    // The ident-map policy enforces a static-join and requires
    // the "all-joined" signal to be broadcast to all nodes...
    typedef map<IPEndPoint, MercuryID, less_SID> ident_map_t;
    map<string, ident_map_t *> m_IdentMap;
    uint32 m_NumIdentMapNodes;
    typedef map<IPEndPoint, vector<HubInitInfo *>, less_SID> static_map_t;
    static_map_t m_StaticInitInfo;

 public:
    static const int DEFAULT_BOOTSTRAP_PORT = 15000;

    BootstrapNode (NetworkLayer *network, Scheduler *scheduler, IPEndPoint& addr, char *schema_file);
    virtual ~BootstrapNode () {}

    void Start ();
    void Stop () {}

    void UpdateHistograms();
    bool CheckAllJoined();
    void CheckRingSanity ();

    virtual void ReceiveMessage (IPEndPoint *from, Message *msg);

 private:
    BootstrapHubInfo* ParseLine(char *line, int curr_line);
    bool ReadSchema(FILE* fp);

    bool ReadIdentMap(const char *file);

    //bool ChooseIdent(MercuryID& ret, IPEndPoint *from, BootstrapHubInfo *hinfo);
    IPEndPoint ChooseRepresentative(IPEndPoint *from, BootstrapHubInfo *hinfo);
    Value GetRandom (const Value& max);

    //void StartProcessingBootReqs();
    //void ProcessNextBootReq();

    BootstrapHubInfo *GetHubInfoByID(byte hubID);
    void UpdateHubHistogram(BootstrapHubInfo *hinfo);

    void HandleBootstrapRequestStatic(IPEndPoint *from, MsgBootstrapRequest *msg);
    void HandleBootstrapRequest(IPEndPoint *from, MsgBootstrapRequest *msg);
    void HandleBarrierPing (IPEndPoint *from, MsgPing *msg);
    void HandleHeartbeat(IPEndPoint *from, MsgHeartBeat *hmsg);
    void HandleEstimateRequest(IPEndPoint *from, MsgCB_EstimateReq *req);

    IPEndPoint ChooseSimulateDistributedJoin (BootstrapHubInfo *hinfo);
    IPEndPoint ChooseRandom (BootstrapHubInfo *hinfo);
    IPEndPoint ChooseRoundRobin (BootstrapHubInfo *hinfo);
    //IPEndPoint ChooseFromIdentMap (IPEndPoint *from, BootstrapHubInfo *hinfo);

    //void FixKnownRanges(NodeState& state, HubInitInfo& newinfo, BootstrapHubInfo *info);

    void CleanupState();
    void DumpState();
};

#endif // __BOOTSTRAPNODE__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
