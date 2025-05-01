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
  Peer.h

  Represents a node in mercury. To start, contains: a connection identifier.
  Soon, lots of other information like alive/dead status, range it is 
  responsible for and stuff... 

begin          : Nov 11, 2002
copyright      : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/


#ifndef __PEER__H
#define __PEER__H

#include <list>
#include <mercury/common.h>
#include <mercury/NetworkLayer.h>
#include <mercury/IPEndPoint.h>
#include <mercury/Constraint.h>

    typedef enum {
	PEER_NONE = 0x0,
	PEER_SUCCESSOR = 0x1,
	PEER_PREDECESSOR = 0x2, 
	PEER_LONG_NBR = 0x4,
	PEER_REV_LONG_NBR = 0x8
    } PeerType;

struct MsgLivenessPong;
class MercuryNode;
class Hub;

struct PingInfo {
    TimeVal time;
    byte    seqno;

    PingInfo (TimeVal t, byte s) : time (t), seqno (s) {}
};

typedef list<PingInfo> PingInfoList;
typedef PingInfoList::iterator PingInfoListIter;

class Peer {
    IPEndPoint     m_Address;                 // ip-address:port of the peer
    NodeRange      m_Range;                   // range it is handling,

    TimeVal        m_LastMsgTime;
    TimeVal        m_LastPingSent;
    TimeVal        m_LastSuccessorPingReceived;        // for "reverse" peers
    TimeVal        m_LastLongNeighborPingReceived;

    MercuryNode   *m_MercuryNode;
    Hub           *m_Hub;

    list<uint32>   m_RTTSamples;
    PingInfoList   m_SentPings;
    byte           m_Seqno; 
    byte           m_PeerType;
 public:        
    Peer (const IPEndPoint &address, const NodeRange &range, const MercuryNode *node, const Hub *hub);
    // default copy-constructs fine...
    ~Peer();

    void AddPeerType (PeerType p);
    void RemovePeerType (PeerType p) { m_PeerType &= ~p; }
    const byte GetPeerType () const { return m_PeerType; }

    const bool IsSuccessor () const { return m_PeerType & PEER_SUCCESSOR; }
    const bool IsPredecessor () const { return m_PeerType & PEER_PREDECESSOR; }
    const bool IsLongNeighbor () const { return m_PeerType & PEER_LONG_NBR; }
    const bool IsReverseLongNeighbor () const { return m_PeerType & PEER_REV_LONG_NBR; }

    bool LostPings (PeerType pt);
    bool LostPongs ();

    const IPEndPoint& GetAddress() const { return m_Address; }

    const NodeRange& GetRange() const { return m_Range; }
    void SetRange (const NodeRange &range);

    TimeVal GetLastMsgTime () const { return m_LastMsgTime; }
    TimeVal GetLastPingTime () const { return m_LastPingSent; }
    void RecvdMessage (); 

    // Send ping to this peer
    void SendLivenessPing (); 

    void RegisterSentPing (TimeVal& now, byte seqno);

    void RegisterIncomingPing (PeerType pt, TimeVal& now);
    TimeVal GetLastPingReceived (PeerType pt) const;

    byte GetNextSeqno () { return m_Seqno++; }

    // Receive pong to the sent ping
    void HandleLivenessPong (MsgLivenessPong *pong);
    void Print(FILE *stream);

    u_long GetRTTEstimate ();
 private:
    void UpdateRTTEstimate (MsgLivenessPong *pong);
    bool IsFartherSuccessor ();
};

ostream& operator<<(ostream& out, const Peer *p);
ostream& operator<<(ostream& out, const Peer &p);

/*
  typedef list<Peer> PeerList;
  typedef PeerList::iterator PeerListIter;
*/

struct PeerInfo : public Serializable {
    IPEndPoint addr;
    NodeRange  range;

    PeerInfo(const IPEndPoint &a, const NodeRange &r) : addr(a), range(r) {}
    PeerInfo(Packet *pkt) {
	addr = IPEndPoint(pkt);
	range = NodeRange(pkt);
    }
    void Serialize(Packet *pkt) {
	addr.Serialize(pkt);
	range.Serialize(pkt);
    }
    uint32 GetLength() {
	return addr.GetLength() + range.GetLength();
    }
    void Print(FILE *) {}

    const NodeRange &GetRange() const { return range; }
    const IPEndPoint &GetAddress() const { return addr; }
};

typedef list<PeerInfo> PeerInfoList;
typedef PeerInfoList::iterator PeerInfoListIter;

#endif // __PEER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
