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
#ifndef __HUB__H
#define __HUB__H

#include <vector>
#include <list>
#include <mercury/common.h>
#include <mercury/Peer.h>
#include <mercury/IPEndPoint.h>
#include <mercury/ID.h>
#include <map>
#include <util/refcnt.h>

//
// status of a node in the attribute hub
//  (most of these will now be unneeded because of the
//   timer implementation - Ashwin [05/10])
//
typedef enum status_t {
    ST_UNJOINED,
    ST_JOINING,
    ST_SUCC_RESPONSE_WAIT,
    ST_CHANGING_SUCCESSOR_WAIT,
    ST_GOT_CHANGING_SUCCESSOR,
    ST_JOINED
} ProtoStatusType;

static const char* g_ProtoStatusStrings[] = {
    "Unjoined",
    "Joining",
    "Waiting response from successor",
    "Waiting for 'ChangingSuccessor'",
    "Got 'ChangingSuccessor'",
    "Joined"
};

class Timer;

struct HubInitInfo : public Serializable {
    byte        ID;               // identifier of the hub
    string      name;             // name!
    Value       absmin;           // absolute min value
    Value       absmax;           // absolute max value
    bool        isMember;         // Am I a member of this hub?
    IPEndPoint  rep;              // Address of a representative of this hub  

    bool staticjoin;
    NodeRange range;
    PeerInfoList succs;
    PeerInfoList preds;

    HubInitInfo();
    HubInitInfo(const HubInitInfo& h);
    HubInitInfo(Packet *pkt);
    ~HubInitInfo();

    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print (FILE *stream);
};

ostream& operator<< (ostream& out, HubInitInfo *info);
ostream& operator<< (ostream& out, HubInitInfo &info);

class Histogram;
class Cache;
class NetworkLayer;
class MercuryNode;

struct MsgAck;
struct MsgPublication;

class Hub : public MessageHandler {
    friend class HubManager;
    friend ostream& operator<<(ostream& out, Hub *hub);

 protected:
    HubInitInfo     m_Initinfo;         // Information got from the bootstrap server
    MercuryNode    *m_MercuryNode;
    Cache          *m_Cache;

 public:

    Hub(MercuryNode *mnode, HubInitInfo &info);
    virtual ~Hub() {} 

    MercuryNode *GetMercuryNode () { return m_MercuryNode; }

    //////////////////////////////////////////////////////////////////////////
    // Accessors
    byte        GetID()           {   return m_Initinfo.ID;    }
    string&     GetName()         {   return m_Initinfo.name;    }
    IPEndPoint& GetBootstrapRep() {   return m_Initinfo.rep;    }
    const Value& GetAbsMin() const {   return m_Initinfo.absmin;    }
    const Value& GetAbsMax() const {   return m_Initinfo.absmax;    }
    bool        IsMine()          {   return m_Initinfo.isMember;    }
    bool        StaticJoin() const { return m_Initinfo.staticjoin; }

    Cache        *GetCache()     {   return m_Cache; }
    void         HandleAck(IPEndPoint *from, MsgAck *ack);

    virtual void ProcessMessage(IPEndPoint *from, Message *msg) = 0;
    virtual void Print(FILE *stream);
};

class PubsubRouter;
class LinkMaintainer;
class HistogramMaintainer;
class BufferManager;
class Scheduler;
class TmpBootstrapHbeater;
class SuccListMaintenanceTimer;

class Sampler;
class LoadSampler;
class MetricInfo;
class Histogram;
typedef map<uint32, MetricInfo *> SamplerMap;
typedef SamplerMap::iterator SamplerMapIter;

struct MsgLivenessPing;
struct MsgLivenessPong;
struct MsgPublication;
struct MsgSubscription;

struct MsgPointEstimateRequest;
struct MsgPointEstimateResponse;
struct MsgSampleRequest;
struct MsgSampleResponse;

class Sample;
class LoadSampler;
class NodeCountSampler;
class NCMetric;
class LoadBalancer;

struct less_value_ptr {
    bool operator () (const Value *a, const Value *b) const {
	return *a < *b;
    }
};

class MemberHub;

// what we want is a registry of peers by their addresses
// successorlist, pred, long neighbors POINT to this list

typedef map<SID, ref<Peer>, less_SID> SIDPeerMap;
typedef SIDPeerMap::iterator SIDPeerMapIter;

typedef list<ref<Peer> > PeerList;
typedef PeerList::iterator PeerListIter;

typedef map<Value *, ref<Peer>, less_value_ptr> PeerMap;
typedef PeerMap::iterator PeerMapIter;

typedef vector<Sample* > PSVec;
typedef PSVec::iterator PSVecIter;

typedef enum { PLIST_FRONT, PLIST_END } listpos_t;

class PList {
 private:
    void remove_peer (ref<Peer> p);
 protected:
    MemberHub*   m_Hub;             
    SIDPeerMap&  m_PMap;            // the underlying hash-table where we record all peers in this list
    PeerType     m_PeerType;        // type of peers in this list
    PeerList     m_List;

    ref<Peer> Register (const IPEndPoint& addr, const NodeRange& range);
    void UnRegister (const IPEndPoint& addr);

 public:
    PList (MemberHub *hub, SIDPeerMap& pmap, PeerType t) : m_Hub (hub), m_PMap (pmap), m_PeerType (t) {}

    void Add (const IPEndPoint& addr, const NodeRange& range, listpos_t where = PLIST_END);
    void Remove (const IPEndPoint& addr);
    template<typename _StrictWeakOrdering>
	void Sort (_StrictWeakOrdering comp) {
	m_List.sort (comp);
    }

    ref<Peer> Front () { return m_List.front (); }
    bool Empty () { return m_List.empty (); }	
    void Clear ();
    const int Size () const { return (int) m_List.size (); }
    Peer* Lookup (const IPEndPoint& addr);

    PeerListIter begin () { return m_List.begin (); }
    PeerListIter end () { return m_List.end (); } 

    void Print (ostream& os);
};

//////////////////////////////////////////////////////////////////////////
// Information about a hub of which this node is a member...
class MemberHub : public Hub {
    friend ostream& operator<<(ostream& out, MemberHub *hub);

    /// friends!
    friend class HubManager;
    friend class LinkMaintainer;
    friend class HistogramMaintainer;
    friend class PubsubRouter;
    friend class MercuryNode;
    friend class NCHistogramMaker;
    friend class LoadBalancer;
    friend class KickOldPeersTimer;
    friend class NbrPrinter;

    ProtoStatusType      m_Status;                // "mostly" obsolete...

    NodeRange           *m_Range;                 // range i am responsible for

    SIDPeerMap           m_PeersByAddress;        // unique peers indexed by their addressesA
    // NOTE: IMP: these are the peers we ping and keep
    // track of for liveness.
    /*	
      SIDPeerMap           m_ReversePeersByAddress; // registry of peers who point to us. this is 
      // so we dont respond to random pings. responding to 
      // random pings => "link breaks" when nodes change
      // ring positions are not detected.
      */												  
    PList                m_SuccessorList;         // successor + backup(s)
    PList                m_PredecessorList;       // predecessors -- information obtained using 
    // special successor pings
    PList                m_LongNeighborsList;     // long pointers obtained using sampling
    PList                m_ReverseLongNeighborsList;    // "back pointers" - useful for responding to pings

    PeerMap              m_SortedPeers;           // sorted according to ranges

    PubsubRouter        *m_PubsubRouter;
    LinkMaintainer      *m_LinkMaintainer;
    HistogramMaintainer *m_HistogramMaintainer;   // this class is soon going to be useless or else, we need to refactor stuff somehow. task for some other time.

    // convenience
    Scheduler           *m_Scheduler;
    NetworkLayer        *m_Network;
    IPEndPoint           m_Address, m_BootstrapIP;

    BufferManager       *m_BufferManager;

    /*
      histogram statistics data per hub. depending on whether a hub is mine,
      i maintain it, otherwise i just request data from the representative i know.
      XXX: make sure cross-hub links are maintained properly.
    */
    SamplerMap           m_SamplerRegistry;
    NodeCountSampler    *m_NCSampler;
    Histogram           *m_NCHistogram; 
    LoadSampler         *m_LoadSampler;
    LoadBalancer        *m_LoadBalancer;
    ptr<TmpBootstrapHbeater> m_BootstrapHeartbeatTimer;
    ptr<SuccListMaintenanceTimer> m_SuccListTimer; 
    ref<Timer>           m_PeerHeartbeatTimer, m_CheckPeerHeartbeatTimer;

 public:

    MemberHub(MercuryNode *mnode, IPEndPoint& bootstrap, 
	      BufferManager *bufferManager, HubInitInfo &info);
    virtual ~MemberHub();

    ProtoStatusType GetStatus() { return m_Status;}
    void SetStatus(ProtoStatusType status) {  m_Status = status;  }

    NodeRange *GetRange() { return m_Range; }
    void SetRange(NodeRange *range);
    void SetRange (const NodeRange &range) {
	SetRange ((NodeRange *) &range);
    }

    Value GetRangeSpan () { 
	if (m_Range == NULL)
	    return VALUE_NONE;
	else 
	    // Jeff: is this code sane?!
	    return m_Range->GetSpan (GetAbsMin (), GetAbsMax () - MercuryID(1));
    }
    bool AmRightMost () {
	if (m_Range == NULL)
	    return true; // doesn't make sense one way or other!
	else 
	    return m_Range->GetMax () == GetAbsMax ();
    }		

    BufferManager *GetBufferManager() { return m_BufferManager; }
    LinkMaintainer *GetLinkMaintainer() { return m_LinkMaintainer; }
    HistogramMaintainer *GetHistogramMaintainer () { return m_HistogramMaintainer; }
    PubsubRouter *GetPubsubRouter () { return m_PubsubRouter; }
    LoadBalancer *GetLoadBalancer () { return m_LoadBalancer; }

    NetworkLayer *GetNetwork () { return m_Network; }	
    Scheduler *GetScheduler () { return m_Scheduler; }
    IPEndPoint& GetAddress () { return m_Address; }
    IPEndPoint& GetBootstrapIP () { return m_BootstrapIP; }

    //////////////////////////////////////////////////////////////////////////
    // Peer management
    void SetSuccessorList (PeerInfoList *ml);
    void MergeSuccessorList (PeerInfoList *newlist);
    void PrintSuccessorList ();
    void PrintPredecessorList ();
    void PrintLongNeighborList ();
    void PrintSortedPeers ();

    Peer *GetPredecessor (); 
    void ClearPredecessor ();
    void SetPredecessor (const IPEndPoint& addr, const NodeRange& range);
    Peer *GetSuccessor (); 
    void SetSuccessor (const IPEndPoint& addr, const NodeRange& range);
    void MergeSuccessor (const IPEndPoint& addr, const NodeRange& range);

    void AddPredecessor (const IPEndPoint& addr, const NodeRange& range);
    void AddLongNeighbor (const IPEndPoint& addr, const NodeRange& range);
    void AddReverseLongNeighbor (const IPEndPoint& addr, const NodeRange& range);

    Peer *LookupPeer (const IPEndPoint& addr);
    Peer *LookupLongNeighbor (const IPEndPoint& addr);
    Peer *LookupSuccessor (const IPEndPoint& addr);

    void RemovePeer (const IPEndPoint addr);

    Peer *GetNearestPeer(const Value &val);
    void GetCoveringPeers (Constraint *cst, vector<Peer *> *nodes);

    //////////////////////////////////////////////////////////////////////////
    // Join management -- this is best delegated to some other class
    void ForcePeerRefresh();
    void StartBootstrapHbeater();
    void OnJoinComplete();
    void PrepareLeave ();
    void StartJoin();
    Histogram* GetNCHistogram () { return m_NCHistogram; }

    //////////////////////////////////////////////////////////////////////////
    // Sampling
    void RegisterMetric (Sampler *s);
    void UnRegisterMetric (Sampler *s);
    void RegisterLoadSampler (LoadSampler *s);
    void UnRegisterLoadSampler (LoadSampler *s);

    void GetSamples (Sampler *s, vector<Sample *> *ret);
    void StartRandomWalk (uint32 handle);
    void DoLocalSampling (uint32 handle);

    LoadSampler *GetLoadSampler () { return m_LoadSampler; }
    NodeCountSampler *GetNodeCountSampler () { return m_NCSampler; }

    //////////////////////////////////////////////////////////////////////////
    // Other functions

    void ProcessMessage (IPEndPoint *from, Message *msg);
    void HandleLivenessPing (IPEndPoint *from, MsgLivenessPing *msg);
    void HandleLivenessPong (IPEndPoint *from, MsgLivenessPong *msg);
    void HandlePointEstimateRequest (IPEndPoint *from, MsgPointEstimateRequest *msg);
    void HandlePointEstimateResponse (IPEndPoint *from, MsgPointEstimateResponse *msg);
    void HandleSampleRequest (IPEndPoint *from, MsgSampleRequest *msg);
    void HandleSampleResponse (IPEndPoint *from, MsgSampleResponse *msg);

    //////////////////////////////////////////////////////////////////////////
    // These are just redirected to the PubsubRouter
    void HandleMercPub (IPEndPoint *from, MsgPublication *msg);
    void HandleMercSub (IPEndPoint *from, MsgSubscription *msg);

    //// If only C++ had the smart inner-classes or delegate kind of support,
    //// we wouldn't have to make these methods public

    void PingPeers ();
    void CheckLiveness ();

    void Print(FILE *stream);
 private:
    bool Less (const Value& val, const Value& other);

    void UpdateSortedPeerList();

    PList& GetSuccessorList () { return m_SuccessorList; }
    PList& GetPredecessorList () { return m_PredecessorList; }
    PList& GetLongNeighborsList () { return m_LongNeighborsList; }
    PList& GetReverseLongNeighborsList () { return m_ReverseLongNeighborsList; }

    MetricInfo *GetRegisteredMetric (uint32 handle);
    MetricInfo *GetRegisteredMetric (Sampler *sampler);
    Peer *GetRandomLongNeighbor ();

    Sample* MakeLocalSample (Sampler *sampler);

    Histogram* InitializeHistogram (int nbkts, int attr, Value& absmin, Value& absmax);
    void MakeHistogram ();
    Histogram* MakeHistogramFromSamples (vector<Sample *>& sam);
    void MakeSamplesDisjoint (PSVec& samples);

    Value GetWeightedMidPoint (const NodeRange& prev, const NodeRange& next);
    float GetNumNodeEstimate (const NodeRange& range, const NCMetric *ncm);
    void AddBucket (Histogram *h, const NodeRange& range, Sample *sample);

};

//////////////////////////////////////////////////////////////////////////
// Information about a hub I am *NOT* a part of...
class NonMemberHub : public Hub {
    friend ostream& operator<<(ostream& out, NonMemberHub *hub);

    BufferManager *m_BufferManager;
 public: 
    NonMemberHub(MercuryNode *mnode, BufferManager *bm, HubInitInfo &initinfo);
    virtual ~NonMemberHub() {}

    IPEndPoint *GetRepAddress() { return &(m_Initinfo.rep); }

    //////////////////////////////////////////////////////////////////////////
    // Other functions
    void ProcessMessage(IPEndPoint *from, Message *msg);
    void Print(FILE *stream);

 private:
    void HandlePublication (IPEndPoint *from, MsgPublication *msg);
};

#endif // __HUB__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
