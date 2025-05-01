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

/**************************************************************************
  PointEvent.cpp

  Class representing a publication.

begin           : Nov 6, 2002
copyright       : (C) 2002-2004 Ashwin Bharambe (ashu@cs.cmu.edu)
copyright       : (C) 2002-2004 Jeff Pang (jeffpang@cs.cmu.edu)

***************************************************************************/

#include <mercury/Event.h>
#include <mercury/Packet.h>
#include <mercury/ObjectLogs.h>      // FIXME: ObjectLogs 
#include <util/Utils.h>
#include <mercury/Hub.h>
#include <mercury/LinkMaintainer.h>
#include <mercury/Message.h>

EventType    PUB_MERC, PUB_POINT;

void RegisterEventTypes() 
{
    PUB_MERC = REGISTER_TYPE (Event, MercuryEvent);
    PUB_POINT = REGISTER_TYPE (Event, PointEvent);
    //	PUB_NBR_REQ = REGISTER_TYPE (Event, NeighborRequestEvent);
}

Event::Event() : m_Matched(false), m_LifeTime(0),
		 m_DeathTime(TIME_NONE),	m_TriggerCount(0)
{
    OBJECT_LOG_DO( m_Nonce = CreateNonce() );
}

Event::Event(Packet * pkt) 
{
    (void) pkt->ReadByte();      // strip off the "type"

    OBJECT_LOG_DO( m_Nonce = pkt->ReadInt() );
    m_LifeTime = pkt->ReadInt();

    byte matched = pkt->ReadByte ();
    if (matched == 0x0)
	m_Matched = false;
    else
	m_Matched = true;

    m_DeathTime = TIME_NONE;
    m_TriggerCount = 0;
}

void Event::Serialize(Packet * pkt)
{
    pkt->WriteByte(GetType());
    OBJECT_LOG_DO( pkt->WriteInt(m_Nonce) );
    pkt->WriteInt(m_LifeTime);
    if (m_Matched)
	pkt->WriteByte (0x1);
    else
	pkt->WriteByte (0x0);
}

uint32 Event::GetLength()
{
    uint32 length = 0;

    length += 1;    // GetType()
    OBJECT_LOG_DO( length += 4 );
    length += 4;    // m_LifeTime
    length += 1;    // m_Matched
    return length;
}

void Event::Print(FILE * stream)
{
    fprintf(stream, "Event (%s) lifetime=%d deathtime=%.3f", 
	    (m_Matched ? "matched" : "unmatched"), m_LifeTime, timeval_to_float (m_DeathTime));
}

Constraint *Event::GetConstraintByAttr (int attr_index) 
{
    for (int i = 0, len = GetNumConstraints (); i < len; i++)
    {
	Constraint *cst = GetConstraint (i);
	if (cst && cst->GetAttrIndex () == attr_index)
	    return cst;
    }

    return NULL;
}

ostream& Event::Print (ostream& out) const
{
    out << "Event (" << (IsMatched() ? "matched" : "unmatched") << 
	") lifetime=" << GetLifeTime() << " deathtime=" << GetDeathTime() ;
    return out;
}

ostream& operator<<(ostream& out, const Event *evt) 
{
    // This call will go down to the DEEPEST class in the hierarchy
    // and come back to Event::Print () through our most immediate
    // children....

    return evt->Print (out);
}

ostream& operator<<(ostream& out, const Event &evt) {
    return out << &evt;
}

////////////////////// MercuryEvent //////////////////////////////
//
MercuryEvent::MercuryEvent (Packet *pkt) : Event(pkt) 
{
    uint32 length = pkt->ReadInt();

    for (uint32 i = 0; i < length; i++) {
	Constraint c(pkt);
	m_Constraints.push_back(c);
    }
}

void MercuryEvent::Serialize(Packet *pkt)	
{ 	
    Event::Serialize (pkt);
    uint32 length = m_Constraints.size();
    pkt->WriteInt(length);

    for (uint32 i = 0; i < length; i++) {
	m_Constraints[i].Serialize(pkt);
    }
}

uint32 MercuryEvent::GetLength()
{
    uint32 length = Event::GetLength();
    length += 4;        // m_Constraints.size();

    for (uint32 i = 0; i < m_Constraints.size(); i++) {
	length += m_Constraints[i].GetLength();
    }

    return length;
}

void MercuryEvent::Print(FILE *stream)
{
    Event::Print(stream);
    fprintf (stream, " constraints=[");
    for (uint32 i = 0; i < m_Constraints.size(); i++) {
	m_Constraints[i].Print(stream);
	if (i != m_Constraints.size() - 1)
	    fprintf(stream, ",");
    }
    fprintf (stream, "]");
}


void MercuryEvent::AddConstraint(Constraint& c) {
    for (int i = 0, len = m_Constraints.size(); i < len; i++)
    {
	if (m_Constraints[i].GetAttrIndex() == c.GetAttrIndex()) {
	    m_Constraints[i] = c;
	    return;
	}
    }
    m_Constraints.push_back(c);
}

ostream& MercuryEvent::Print (ostream& out) const
{
    Event::Print (out);

    out << " constraints=[";
    for (MercuryEvent::iterator it = MercuryEvent::begin (); it != MercuryEvent::end (); it++) {
	if (it != MercuryEvent::begin ()) out << ",";
	out << *it;
    }
    out << "]";
    return out;
}

//////////////////////////////////////////////////////////////////////////
// PointEvent

// We don't expect events to have too many attributes. Therefore,
// our checking implementation doesn't use a hash table.

void PointEvent::AddConstraint (Constraint& c)
{
    ASSERT (c.GetMin () == c.GetMax ());
    MercuryEvent::AddConstraint (c);
}

void PointEvent::AddTuple (Tuple& t) 
{
    for (int i = 0, len = m_Constraints.size(); i < len; i++)
    {
	if (m_Constraints[i].GetAttrIndex() == t.GetAttrIndex()) {
	    m_Constraints[i] = Constraint (t.GetAttrIndex (), t.GetValue (), t.GetValue ());
	    return;
	}
    }
    Constraint c (t.GetAttrIndex (), t.GetValue (), t.GetValue ());
    m_Constraints.push_back(c);
}

Value* PointEvent::GetValueByAttr (int attr_index) 
{
    Constraint *c = GetConstraintByAttr (attr_index);
    if (c == NULL)
	return NULL;
    else 
	return (Value *) &c->GetMin ();

    return NULL;
}

ostream& PointEvent::Print (ostream& out) const 
{
    return MercuryEvent::Print (out);
}

#if 0
///////////////////// NeighborRequestEvent /////////////////////////////
//
NeighborRequestEvent::NeighborRequestEvent(Packet *pkt) : PointEvent(pkt) 
{
    m_Epoch = pkt->ReadInt();
}

void NeighborRequestEvent::Serialize(Packet *pkt) 
{
    PointEvent::Serialize(pkt);
    pkt->WriteInt(m_Epoch);
}

uint32 NeighborRequestEvent::GetLength() 
{
    return PointEvent::GetLength() + 4;
}

void NeighborRequestEvent::Print(FILE *stream)
{
    PointEvent::Print(stream);
    fprintf(stream, " epoch=%d", m_Epoch);
}

bool NeighborRequestEvent::OnRendezvousReceive(MemberHub *hub, MsgPublication *pmsg)
{
    hub->GetLinkMaintainer()->HandleNeighborRequest(&(pmsg->sender), pmsg);
    return false;
}

ostream& NeighborRequestEvent::Print (ostream& out) const
{
    PointEvent::Print (out);

    out << " epoch=" << m_Epoch;
    return out;
}

SamplerEvent::SamplerEvent (Packet *pkt) : PointEvent (pkt)
{
    m_MetricID = (uint32) pkt->ReadInt ();
}

bool SamplerEvent::OnRendezvousReceive (MemberHub *hub, MsgPublication *pmsg)
{
    // 1. if no local sampling has been performed for this metric, start it; 
    //    when done, goto 2.
    // 2. send a local sample and the most recent k_2 samples obtained to 
    //    pmsg->creator

    hub->GetHistogramMaintainer()->HandleSampleRequest (&pmsg->sender, pmsg);
    return false;
}

void SamplerEvent::Serialize (Packet *pkt)
{
    PointEvent::Serialize (pkt);
    pkt->WriteInt ((int) m_MetricID);
}

uint32 SamplerEvent::GetLength ()
{
    return PointEvent::GetLength () + 4;
}

void SamplerEvent::Print (FILE *stream)
{
    PointEvent::Print (stream);
    fprintf (stream, " metricid=%d", m_MetricID);
}

ostream& SamplerEvent::Print (ostream& out) const 
{
    PointEvent::Print (out);
    return out << " metricid=" << m_MetricID;
}
#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
