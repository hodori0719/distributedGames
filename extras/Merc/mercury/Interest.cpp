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

#include <mercury/Event.h>
#include <mercury/Interest.h>
#include <mercury/Packet.h>
#include <util/Utils.h>

#include <mercury/ObjectLogs.h>      // FIXME: om dependency

InterestType INTEREST;

void RegisterInterestTypes() {
    INTEREST = REGISTER_TYPE (Interest, Interest);
}

Interest::Interest () : m_Subscriber (SID_NONE), m_GUID (GUID_NONE)
{
    m_LifeTime = g_Preferences.sub_lifetime;
    m_DeathTime = TIME_NONE;

    OBJECT_LOG_DO( m_Nonce = CreateNonce() );
}

Interest::Interest(IPEndPoint& subscriber) : m_Subscriber (subscriber), m_GUID(GUID_NONE)
{
    m_LifeTime = g_Preferences.sub_lifetime;
    m_DeathTime = TIME_NONE;

    OBJECT_LOG_DO( m_Nonce = CreateNonce() );
}

Interest::Interest(IPEndPoint& subscriber, guid_t g) : 
    m_Subscriber (subscriber), m_GUID(g)
{
    m_LifeTime = g_Preferences.sub_lifetime;
    m_DeathTime = TIME_NONE;

    OBJECT_LOG_DO( m_Nonce = CreateNonce() );
}

Interest::Interest(Packet * pkt) 
{
    (void) pkt->ReadByte();        // strip off the "type"

    m_Subscriber = IPEndPoint(pkt);
    m_GUID = guid_t(pkt);

    uint32 length = pkt->ReadInt();

    for (uint32 i = 0; i < length; i++) {
	Constraint c(pkt);
	m_Constraints.push_back(c);
    }

    m_LifeTime = pkt->ReadInt();
    m_DeathTime = TIME_NONE;

    OBJECT_LOG_DO( m_Nonce = pkt->ReadInt() );
}

void Interest::Serialize(Packet * pkt)
{
    pkt->WriteByte(GetType());

    m_Subscriber.Serialize(pkt);
    m_GUID.Serialize(pkt);

    uint32 length = m_Constraints.size();
    pkt->WriteInt(length);

    for (uint32 i = 0; i < length; i++) {
	m_Constraints[i].Serialize(pkt);
    }

    pkt->WriteInt(m_LifeTime);
    OBJECT_LOG_DO( pkt->WriteInt(m_Nonce) );
}

Constraint *Interest::GetConstraintByAttr (int attr) 
{
    for (int i = 0, len = m_Constraints.size(); i < len; i++)
    {
	if (m_Constraints[i].GetAttrIndex() == attr) {
	    return &m_Constraints[i];
	}
    }
    return NULL;
}

void Interest::AddConstraint(Constraint& c) {
    for (int i = 0, len = m_Constraints.size(); i < len; i++)
    {
	if (m_Constraints[i].GetAttrIndex() == c.GetAttrIndex()) {
	    m_Constraints[i] = c;
	    return;
	}
    }
    m_Constraints.push_back(c);
}

uint32 Interest::GetLength()
{
    uint32 length = 1 /* GetType() */ + m_Subscriber.GetLength();
    length += 4;			// #constraints
    length += m_GUID.GetLength();

    for (int i = 0; i < (int) m_Constraints.size(); i++)
	length += m_Constraints[i].GetLength();

    length += 4;    // m_LifeTime

    OBJECT_LOG_DO( length += 4 );  // m_Nonce
    return length;
}

bool Interest::Overlaps (Event *evt)
{
    bool tried = false;

    for (int i = 0, len = m_Constraints.size(); i < len; i++)
    {
	Constraint *c = &(m_Constraints[i]);
	Constraint *oc = evt->GetConstraintByAttr (c->GetAttrIndex());

	DB (20) << "interest constraint=" << c << endl;

	if (oc != NULL) {
	    tried = true;

	    DB (20) << "event constraint=" << oc << endl;

	    if (!c->Overlaps (*oc))
		return false;
	}
	else {
	    DB (20) << " no event constraint! " << endl;
	}
    }
    //INFO << "olap!" << endl;
    return tried;
}

#if 0
bool Interest::Covers (PointEvent * evt)
{
    bool tried = false;
    for (int i = 0, len = m_Constraints.size(); i < len; i++)
    {
	Constraint *c = &(m_Constraints[i]);
	const Value *val = evt->GetValue(c->GetAttrIndex());

	if (val != NULL) {
	    tried = true;
	    if (!c->Covers (*val))
		return false;
	}
    }
    //INFO << "covers!" << endl;
    return tried;
}
#endif

bool Interest::IsEqual(Interest * other)
{
    return m_GUID == other->GetGUID();
}


void Interest::Print(FILE * stream)
{
    fprintf (stream, "subscriber="); 
    m_Subscriber.Print (stream);
    fprintf (stream, " ");

    uint32 length = m_Constraints.size();	
    for (uint32 i = 0; i < length; i++) {
	m_Constraints[i].Print(stream);
    }
}

ostream& Interest::Print (ostream& out) const
{
    Interest *sub = (Interest *) this;

    out << "Interest guid=" << sub->GetGUID() << " subscriber=[" << sub->GetSubscriber() << "] cons=[";

    for (Interest::iterator it = sub->begin (); it != sub->end (); it++) {
	if (it != sub->begin ()) out << ",";
	out << *it;
    }
    out << "] lifetime=" << sub->GetLifeTime() << " deathtime=" << sub->GetDeathTime();
    return out;
}

ostream& operator<<(ostream& out, const Interest &sub) {
    return out << &sub;
}

ostream& operator<<(ostream& out, const Interest *sub) 
{	
    // This call will go down to the DEEPEST class in the hierarchy
    // and come back to Interest::Print () through our most immediate
    // children....
    return sub->Print (out);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
