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
#ifndef __PUBSUBROUTER__H
#define __PUBSUBROUTER__H

#include <list>
#include <mercury/Event.h>
#include <mercury/IPEndPoint.h>
#include <mercury/Sampling.h>

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
struct Message;
struct MsgPublication;    
struct MsgSubscription;
struct MsgAck;
struct MsgSubscriptionList;
struct MsgTriggerList;
struct MsgLinearSubscription;
struct MsgLinearPublication;

class Hub;
class Interest;
class MemberHub;
class Cache;
class NetworkLayer;
class LinkMaintainer;
class BufferManager;
class MercuryNode;
class Scheduler;
class CountResetter;

//////////////////////////////////////////////////////////////////////////
// STL wrappers

class StopRangeChange;
class PubsubStore;

#define MAX_PUBSUB_TTL 20
//////////////////////////////////////////////////////////////////////////
// This class handles the main routing of publications and subscriptions
// Each hub that I am a part of has an instance of this class associated 
// with itself...
class PubsubRouter  : public MessageHandler
{
    friend class CountResetter;
    friend class StopRangeChange;

    MemberHub           *m_Hub;
    BufferManager       *m_BufferManager;
    LinkMaintainer      *m_LinkMaintainer;

    MercuryNode         *m_MercuryNode;
    NetworkLayer        *m_Network;
    IPEndPoint           m_Address;
    Scheduler           *m_Scheduler;


    // FIXME: a (possibly) separate cache should exist with somebody 
    //        for handling ACKs which come across the hubs. These acks should 
    //        be taken into account when MercuryNode sends cross-hub pubs

    Cache               *m_Cache;          // Cache of nodes I have seen when sending to this hub!

    PubsubStore         *m_Store;

    int m_RoutedSubs, m_RoutedPubs;
    float m_RoutingLoad;
    list<float> m_LoadWindows;
    int m_WindowIndexAtChange;
    double m_RangeRatioAtChange;

    bool m_RangeChanged;
    ptr<StopRangeChange> m_StopRangeChangeTimer;
    IPEndPoint m_LastHop;
 public:
    PubsubRouter(MemberHub *hub, BufferManager *bm, LinkMaintainer *lm);
    virtual ~PubsubRouter();

    void ProcessMessage(IPEndPoint *from, Message *msg);

    Cache *GetCache() { return m_Cache; }

    void PrintSubscriptionList(FILE *stream);

    vector<int> HandoverSubscriptions(IPEndPoint *from);
    vector<int> HandoverTriggers(IPEndPoint *from);
    void PurgeOutofRangeData ();

    void RouteData(IPEndPoint *from, Message *msg);

    void PrintSubscriptionList(ostream& stream);
    void PrintPublicationList(ostream& stream);

    float GetRoutingLoad () { return m_RoutingLoad; }
    void SetRangeChanged ();
    void UpdateRangeLoad (const NodeRange& newrange);

    void ClearData ();
    void GetMatchingSubscriptions (const NodeRange& range, list<Interest *>& matched);
    void GetMatchingTriggers (const NodeRange& range, list<MsgPublication *>& matched);
    void AddNewTrigger (MsgPublication *pmsg);
    void AddNewInterest (Interest *in);

    const IPEndPoint *ComputeNextHop(const Value &val );

    void SendQuickPong (IPEndPoint *from);
 private:

    bool RouteRangeData(IPEndPoint *from, Message *msg, const IPEndPoint* &next_hop);
    void DoFanout (Constraint *newcst, Message *msg);

    void HandleMercMatchedPub( IPEndPoint *from, MsgPublication *msg );    
    void HandleLinearPublication (IPEndPoint *from, MsgLinearPublication *lpmsg);
    void HandleLinearSubscription (IPEndPoint *from, MsgLinearSubscription *lsmsg);

    void HandlePubAtRendezvous( IPEndPoint *from, MsgPublication *msg, bool pubMatchesLeft, bool pubMatchesRight );
    void HandleSubAtRendezvous( IPEndPoint *from, MsgSubscription *msg );
    void HandleSubscriptionList(IPEndPoint *from, MsgSubscriptionList *slmsg);
    void HandleTriggerList(IPEndPoint *from, MsgTriggerList *slmsg);

    void TriggerPublications(MsgSubscription *smsg);

    void DeliverPubToSubscribers(MsgPublication *pmsg, bool pubMatchesLeft, bool pubMatchesRight);
    void SendAck(MsgPublication *pmsg);

    bool CheckAppLinear (Message *msg);
    bool CheckAppRoute (Message *msg);    

    void NewLoadWindow ();
};

class PubsubLoadSampler : public LoadSampler {
    PubsubRouter *m_PSRouter;
 public:
    PubsubLoadSampler (MemberHub *hub);

    virtual const char *GetName () const { return "PubsubLoadSampler"; }
    virtual Metric *GetPointEstimate ();
};
#endif // __PUBSUBROUTER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
