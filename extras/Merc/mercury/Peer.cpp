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

#include <mercury/Peer.h>
#include <mercury/Message.h>
#include <mercury/Constraint.h>
#include <mercury/MercuryNode.h>
#include <mercury/Hub.h>
#include <mercury/Parameters.h>

#define PING_CACHE_SIZE 10             // maintain info about last <k> pings when matching pongs
#define RTT_SAMPLES     20             // maintain past <k> RTT samples

// I would ideally like to not explicitly get rid of this 'const' thing
// as I have done here, but this just escalates to const's not being
// present at every other line in the code. Dont have the time to fix
// them!

/*
  bool operator<(const Peer *a, const Peer *b) {
  return a->GetRange().GetMin() < b->GetRange().GetMin();
  }
*/

Peer::Peer (const IPEndPoint &address,  const NodeRange &range, const MercuryNode *node, const Hub *h):
    m_Address(address), m_Range(range), m_MercuryNode ((MercuryNode *) node), m_Hub ((Hub *) h), m_Seqno (1),
    m_PeerType (PEER_NONE)
{
    m_LastMsgTime = m_LastSuccessorPingReceived = m_LastLongNeighborPingReceived = m_MercuryNode->GetScheduler ()->TimeNow ();
    memset (&m_LastPingSent, 0, sizeof (TimeVal));
}

Peer::~Peer() {}

void Peer::SetRange(const NodeRange &range) {
    m_Range = NodeRange(range);
}

u_long Peer::GetRTTEstimate ()
{
    if (m_RTTSamples.size () == 0)
	return 0;

    double mean = 0;
    for (list<uint32>::iterator it = m_RTTSamples.begin(); it != m_RTTSamples.end (); it++) {
	mean += (double) (*it);
    }

    mean /= m_RTTSamples.size ();
    return (u_long) mean;
}

void Peer::Print(FILE * stream)
{
    fprintf(stream, "(Peer address=%s ", m_Address.ToString());
    fprintf(stream, "range=");
    m_Range.Print(stream);
    fprintf(stream, ")");
}

static char *_PTString (byte p)
{
    static char s[80];

    s[0] = '\0';
    if (p & PEER_SUCCESSOR) 
	strcat (s, "succ;");
    if (p & PEER_PREDECESSOR)
	strcat (s, "pred;");
    if (p & PEER_LONG_NBR)
	strcat (s, "lnbr;");
    if (p & PEER_REV_LONG_NBR)
	strcat (s, "revn;");
    return s;
}

ostream& operator<<(ostream& out, const Peer *p)
{
    if (p == NULL)
	return out << "(null peer)" << endl;

    out << "(" << p->GetAddress().ToString() 
	<< " " << p->GetRange() << " " << _PTString (p->GetPeerType ()) << ")"; //  << " lastheard=" << p->GetLastPongTime ();
    return out;
}

ostream& operator<< (ostream& out, const Peer &p) {
    return out << &p;
}

bool Peer::LostPongs () 
{
    TimeVal& now = m_MercuryNode->GetScheduler ()->TimeNow ();
    bool ret = (now - m_LastMsgTime > (sint32) Parameters::PeerPongTimeout);
    if (ret) {
	// DB_DO (-1) {
	MTWARN << " (A)DEATH: hub(" << (int) m_Hub->GetID() << ") peer=" << this << endl; 
	// }
    }
    return ret;
}

void Peer::AddPeerType (PeerType p) {
    m_PeerType |= p;
    if (p == PEER_PREDECESSOR) 
	m_LastSuccessorPingReceived = m_MercuryNode->GetScheduler ()->TimeNow ();
    if (p == PEER_REV_LONG_NBR)
	m_LastLongNeighborPingReceived = m_MercuryNode->GetScheduler ()->TimeNow ();
}

void Peer::RegisterIncomingPing (PeerType pt, TimeVal& now)
{
    ASSERT (pt == PEER_SUCCESSOR || pt == PEER_LONG_NBR);

    MTDB (-10) << " got ping from " << m_Address << " for hub " << (int) m_Hub->GetID () << endl;
    if (pt == PEER_SUCCESSOR)
	m_LastSuccessorPingReceived = now;
    else
	m_LastLongNeighborPingReceived = now;
    m_LastMsgTime = now;
}

TimeVal Peer::GetLastPingReceived (PeerType pt) const 
{
    ASSERT (pt == PEER_SUCCESSOR || pt == PEER_LONG_NBR);
    if (pt == PEER_SUCCESSOR)
	return m_LastSuccessorPingReceived;
    else
	return m_LastLongNeighborPingReceived;
}

bool Peer::LostPings (PeerType pt)
{
    TimeVal& now = m_MercuryNode->GetScheduler ()->TimeNow ();
    bool ret;

    ASSERT (pt == PEER_SUCCESSOR || pt == PEER_LONG_NBR);

    if (pt == PEER_SUCCESSOR)
	ret = (now - m_LastSuccessorPingReceived > (sint32) Parameters::PeerPongTimeout);
    else
	ret = (now - m_LastLongNeighborPingReceived > (sint32) Parameters::PeerPongTimeout);

    if (ret) {
	// DB_DO (-10) {
	MTWARN << " (P) rev-" << _PTString (pt) << " DEATH: hub(" << (int) m_Hub->GetID() << ") peer=(" << this << ")" << endl; 
	// }
    }
    return ret;
}

void Peer::HandleLivenessPong (MsgLivenessPong *pong)
{
    byte seqno = pong->GetSeqno ();

    m_LastMsgTime = m_MercuryNode->GetScheduler ()->TimeNow ();
    m_Range = pong->GetRange ();

    MTDB (-10) << " got pong from " << m_Address << " for hub " << (int) pong->hubID << endl;

    if (seqno != 0xff) 
	UpdateRTTEstimate (pong);
}

void Peer::RecvdMessage ()
{
    m_LastMsgTime = m_MercuryNode->GetScheduler ()->TimeNow ();
}

// This is just elementary!
void Peer::UpdateRTTEstimate (MsgLivenessPong *pong)
{
    for (PingInfoListIter it = m_SentPings.begin (); it != m_SentPings.end (); it++) {
	if (it->seqno != pong->GetSeqno ())
	    continue;

	MDB (20) << " pong matched to ping! seqno=" << (int) it->seqno << endl;
	uint32 rtt_millis = pong->recvTime - it->time;

	if (m_RTTSamples.size () > RTT_SAMPLES)
	    m_RTTSamples.pop_front ();
	m_RTTSamples.push_back (rtt_millis);

	// dont continue any further
	m_SentPings.erase (it);
	return;
    }
}

void Peer::RegisterSentPing (TimeVal& now, byte seqno)
{
    if (m_SentPings.size () >= PING_CACHE_SIZE)
	m_SentPings.pop_front ();

    m_SentPings.push_back (PingInfo (now, seqno));
    m_LastPingSent = now;
}

bool Peer::IsFartherSuccessor ()
{
    MemberHub *hub = (MemberHub *) m_Hub;

    // make sure I am not immediate successor
    if (GetAddress () == hub->GetSuccessor ()->GetAddress ())
	return false;

    return hub->LookupSuccessor (m_Address) != NULL;
}

void Peer::SendLivenessPing ()
{
    MemberHub *hub = dynamic_cast <MemberHub *> (m_Hub);
    if (!hub->GetRange ())
	return;

    MTDB (-10) << " sending ping for hub[" << (int) hub->GetID() << "] to " << GetAddress() << endl;

    MsgLivenessPing *ping = new MsgLivenessPing (hub->GetID(), m_MercuryNode->GetAddress (), *hub->GetRange(), m_Seqno++);
    if (m_Seqno == 0xff)
	m_Seqno++;

    if (IsSuccessor ()) 
	ping->AddPeerType (PEER_SUCCESSOR);
    if (IsLongNeighbor ())
	ping->AddPeerType (PEER_LONG_NBR);

    m_MercuryNode->GetNetwork()->SendMessage(ping, (IPEndPoint *) &GetAddress (), Parameters::TransportProto);

    RegisterSentPing (m_MercuryNode->GetScheduler()->TimeNow(), ping->GetSeqno());		
    delete ping;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
