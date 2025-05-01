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
#ifndef __LINK_MAINTAINER__H
#define __LINK_MAINTAINER__H

#include <mercury/common.h>
#include <mercury/MercuryID.h>
#include <mercury/Timer.h>
#include <mercury/IPEndPoint.h>
#include <mercury/ID.h>
#include <mercury/Parameters.h>
#include <mercury/Constraint.h>

class MemberHub;
class LinkMaintainer;
class NetworkLayer;

class JoinRequestTimer : public Timer {
    MemberHub   *m_Hub;
    u_long m_Timeout;
    IPEndPoint m_RepAddress;
    bool m_LeaveJoin;
 public:    
    JoinRequestTimer(MemberHub *hub, IPEndPoint repaddr, int timeout, bool leavejoin = false);

    const IPEndPoint& GetRepAddress () const { return m_RepAddress; }
    void OnTimeout();
};

class SuccListMaintenanceTimer : public Timer {
    LinkMaintainer *m_LinkMaintainer;
 public:
    SuccListMaintenanceTimer(LinkMaintainer *lm, int timeout);
    void OnTimeout();
};

class KickOldPeersTimer : public Timer {
    MemberHub *m_Hub;
 public:
    KickOldPeersTimer(MemberHub *hub, int timeout);
    void OnTimeout();
};

//////////////////////////////////////////////////////////////////////////
// Forward declarations
class Peer;
class PubsubRouter;
class Scheduler;

struct IPEndPoint;
struct Message;
struct MsgJoinRequest;
struct MsgJoinResponse;
struct MsgNotifySuccessor;
struct MsgGetPred;
struct MsgPred;
struct MsgGetSuccessorList;
struct MsgSuccessorList;
struct MsgNeighborRequest;
struct MsgNeighborResponse;
struct MsgPublication;
struct MsgLeaveNotification;
struct MsgLinkBreak;

class MercuryNode;
class LinkMaintainer;

class NeighborRequestTimer : public Timer {
    LinkMaintainer *m_LM;
    uint32 m_Nonce;
 public:
    NeighborRequestTimer (LinkMaintainer *lm, uint32 nonce) : 
	Timer (Parameters::LongNeighborResponseTimeout), 
	m_LM (lm), m_Nonce (nonce) {}

    void OnTimeout ();
};


typedef map<uint32, ref<NeighborRequestTimer> > NRTMap;
typedef NRTMap::iterator NRTMapIter;

typedef SIDTimeValMap JRMap;
typedef SIDTimeValMapIter JRMapIter;

class LinkMaintainer : public MessageHandler
{
    friend class NeighborRequestTimer;
    friend class LNContinuation;

    MemberHub              *m_Hub;
    MercuryNode            *m_MercuryNode;
    ref<JoinRequestTimer>   m_JoinRequestTimer;

    NetworkLayer           *m_Network;
    int                     m_Epoch;

    IPEndPoint              m_Address;
    Scheduler              *m_Scheduler;
    int                     m_SLMaintInvocations;      // how many times has DoSuccListMaintenance () been invoked?
    JRMap                   m_JoinResponseSIDs;
    NRTMap                  m_NRTimers;                // timers associated with outstanding long neighbor requests

 public:
    LinkMaintainer(MemberHub *hub);
    virtual ~LinkMaintainer () {}

    void Start ();
    void ProcessMessage (IPEndPoint *from, Message *msg);
    void OnPeerDeath (ref<Peer> p);
    void RepairLongPointers ();
    void RepairPointer (uint32 oldnonce, int nodeCount);

    void DoSuccListMaintenance ();
    void DoLeaveJoin (IPEndPoint *newsucc, double currentload);
 private:
    uint32 SendNeighborRequest (Value& dest_val);

    void HandleNotifySuccessor( IPEndPoint *from, MsgNotifySuccessor *msg );
    void HandleGetPred( IPEndPoint *from, MsgGetPred *msg );
    void HandlePred( IPEndPoint *from, MsgPred *msg );
    void HandleGetSuccList( IPEndPoint *from, MsgGetSuccessorList *msg );
    void HandleSuccList( IPEndPoint *from, MsgSuccessorList *msg );

    void HandleJoinRequest( IPEndPoint *from, MsgJoinRequest *msg);
    void HandleJoinResponse( IPEndPoint *from, MsgJoinResponse *msg );
    void HandleNeighborRequest (IPEndPoint *from, MsgNeighborRequest *msg);
    void HandleNeighborResponse(IPEndPoint *from, MsgNeighborResponse *msg);
    void HandleLeaveNotification (IPEndPoint *from, MsgLeaveNotification *msg);
    void HandleLinkBreak (IPEndPoint *from, MsgLinkBreak *msg);

    bool IsRequestDuplicate (IPEndPoint *from);
    Value GenerateHarmonicValue (int nodeCount);
    void SendLeaveNotification (IPEndPoint addr, NodeRange range, double currentload, IPEndPoint newsucc);
    void EndLeave (IPEndPoint newsucc);
};

#endif  // __LINK_MAINTAINER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
