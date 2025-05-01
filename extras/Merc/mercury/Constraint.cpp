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

/*
  Constraints = attr_index + min + max

  $Id: Constraint.cpp 2507 2005-12-21 22:05:36Z jeffpang $
*/

#include <string>
#include <util/debug.h>
#include <mercury/Constraint.h>
#include <mercury/Packet.h>
#include <util/Benchmark.h>

Constraint RANGE_NONE = Constraint (-1, VALUE_NONE, VALUE_NONE);

Constraint& Constraint::operator= (const Constraint& c)
{
    if (this == &c) return *this;

    m_AttrIndex = c.GetAttrIndex();
    m_Min = c.GetMin();
    m_Max = c.GetMax();

    return *this;
}

Constraint::Constraint(Packet *pkt) : m_AttrIndex (pkt->ReadInt ())
				    , m_Min (pkt), m_Max (pkt)
{
}

void Constraint::Clamp (const Value& amin, const Value& amax) {
    if (m_Min < amin) {
	DB_DO(10) {	WARN << "clamping left end: " << m_Min << " to " << amin << endl;}
	m_Min = amin;
    }

    if (m_Max > amax) {
	DB_DO(10) { WARN << "clamping right end: " << m_Max << " to " << amax << endl;	}
	m_Max = amax;
    }
}

void Constraint::GetRouteDirectionsWrapped (NodeRange& r, bool& left, bool& center, bool& right)
{
    left = false;
    right = center = true;

    if (m_Min >= r.GetMax () && m_Max < r.GetMin ())
	center = false;
}

void Constraint::GetRouteDirections (NodeRange& r, bool& left, bool& center, bool& right, bool amRightmost)
{
    START(CST::GetRouteDirections);
    left = right = center = false;

    if (r.GetMin () > r.GetMax ()) {
	GetRouteDirectionsWrapped (r, left, center, right);
	return;
    }

    if (m_Min < r.GetMin())
	left = true;

    if (amRightmost)
	right = false;
    else {
	if (m_Max >= r.GetMax())
	    right = true;
    }

    if (amRightmost) {
	// trying to make it faster
	if (m_Max < r.GetMin () || m_Min > r.GetMax ())
	    center = false;
	else
	    center = true;

	/*
	  if ((m_Min >= r.GetMin () && m_Min <= r.GetMax ())
	  || (r.GetMin () >= m_Min && r.GetMin () <= m_Max))
	  center = true;
	*/
    }
    else {
	if (m_Max < r.GetMin () || m_Min >= r.GetMax ())
	    center = false;
	else 
	    center = true;
	/*
	  if ((m_Min >= r.GetMin() && m_Min < r.GetMax()) 
	  || (r.GetMin () >= m_Min && r.GetMin () <= m_Max))
	  center = true;
	*/
    }

    STOP(CST::GetRouteDirections);
    return;
}

bool Constraint::Covers (const Value& val) const
{
    // Jeff: 
    // (1) fixed to handle the wrap around case
    // (2) made left-exclusive: [min,max) because this is used to
    //     test for range ownership (otherwise touching ranges will
    //     both claim the one overlapping id). Dunno if we wanted
    //     min or max ownership, but I use min in my code so I picked
    //     that arbitrarily for now. :) Let me know if you change this.
    //
    // xxx should not the  rest of the overlap, etc. logic below also
    // respect left-exclusiveness? I dunno what they are all used for
    // but if they are ever used for ownership testing they definitely
    // should...
    if (m_Min > m_Max)
	return val >= m_Min && val < m_Max;
    return m_Min <= val && val < m_Max;
}

bool Constraint::Overlaps (const Constraint& oc) const
{
    return (m_Min >= oc.GetMin() && m_Min <= oc.GetMax()) || 
	(oc.GetMin () >= m_Min && oc.GetMin () <= m_Max);
}

bool Constraint::OverlapsNodeRange (const NodeRange& nr) const
{
    if (nr.GetMin () < nr.GetMax ()) 
	return Overlaps (nr);

    if (m_Min >= nr.GetMax () && m_Max < nr.GetMin ())
	return false;
    return true;
}

Value Constraint::GetSpan (const Value& absmin, const Value& absmax) const
{
    Value ret;

    if (m_Min < m_Max) {
	ret = m_Max;
	ret -= m_Min;
	return ret;
    }

    if (m_Min == m_Max) {
	ret = 0;
	return ret;
    }

    /* ret = (absmax - m_Min) + (m_Max - absmin)  */ 
    ret = absmax;    
    ret -= m_Min;
    ret += m_Max;
    ret -= absmin;
    return ret;
}

Value Constraint::GetOverlap (const Constraint& cst) const 
{
    Value ret = 0;

    // just to use old code
    Constraint *r = (Constraint *) this;
    Constraint *s = (Constraint *) &cst;
#define LOW(x)       x->GetMin()
#define HIGH(x)      x->GetMax()

    if (LOW(s) <= LOW(r) && HIGH(s) > LOW(r)) {
	if (HIGH(s) > HIGH(r)) {
	    ret = HIGH(r);
	    ret -= LOW(r);
	}
	else {
	    ret = HIGH(s);
	    ret -= LOW(r);
	}
    } 
    else if (LOW(s) < HIGH(r) && HIGH(s) >= HIGH(r)) {
	ret = HIGH(r);
	ret -= LOW(s);
    } else if (LOW(r) < HIGH(s) && HIGH(s) < HIGH(r)) {
	ret = HIGH(s);
	ret -= LOW(s);
    }
#undef LOW
#undef HIGH

    if (ret < 0) 
	ret = 0;
    return ret;
}

uint32 Constraint::GetLength()
{
    uint32 length = 4;   // m_AttrIndex
    length += m_Min.GetLength() + m_Max.GetLength();

    return length;
}

void Constraint::Serialize(Packet * pkt)
{
    pkt->WriteInt(m_AttrIndex);
    m_Min.Serialize(pkt);
    m_Max.Serialize(pkt);
}

void Constraint::Print(FILE * stream)
{
    fprintf(stream, "%s:[", (m_AttrIndex == -1 ? "null" : G_GetTypeName (m_AttrIndex)));
    m_Min.Print(stream);
    fprintf(stream, ",");
    m_Max.Print(stream);
    fprintf(stream, "]");	
}

ostream& operator<<(ostream& out, const Constraint &c) {
    return out << &c ;
}

ostream& operator<<(ostream& out, const Constraint *c) {
    if (c == NULL)
	return out << "(null constraint)";

    int i = c->GetAttrIndex();
    out << G_GetTypeName(i) << ":[" << c->GetMin() << "," << c->GetMax() << "]" ;
    return out;
}

bool operator== (const Constraint& c1, const Constraint& c2) 
{
    return c1.GetAttrIndex () == c2.GetAttrIndex () && 
	c1.GetMin () == c2.GetMin () &&
	c1.GetMax () == c2.GetMax ();
}

bool operator!= (const Constraint& c1, const Constraint& c2) {
    return !(c1 == c2);
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
