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
// vim:sw=4

/*
  Constraints are _only_ ranges now, instead of supporting arbitrary "operators".
  The functionality offered remains almost the same, though.

  $Id: Constraint.h 2382 2005-11-03 22:54:59Z ashu $
*/

#ifndef __CONSTRAINT__H
#define __CONSTRAINT__H

#include <string>
#include <vector>
#include <mercury/MercuryID.h>

#define RMIN(x) x.GetMin ()
#define RMAX(x) x.GetMax ()

typedef class Constraint Interval;
typedef class Constraint NodeRange;

class Constraint : public Serializable { 
 private:
    int      m_AttrIndex;
    Value    m_Min, m_Max;

 public:
    Constraint() {}
    Constraint(int attr, Value min, Value max) 
	: m_AttrIndex (attr), m_Min (min), m_Max (max) {}

    Constraint(const Constraint &c) 
	: m_AttrIndex (c.GetAttrIndex ()), m_Min (c.GetMin ())
	, m_Max (c.GetMax ()) {}

    Constraint& operator=(const Constraint &c); 

    bool operator== (Constraint &c) {
	return m_Min == c.GetMin() && m_Max == c.GetMax();
    }

    Constraint(Packet *pkt);
    virtual ~Constraint() {}

    const Value& GetMin() const { return m_Min; }
    const Value& GetMax() const { return m_Max; }
    int   GetAttrIndex() const { return m_AttrIndex; }

    void Clamp (const Value& amin, const Value& amax);
    void GetRouteDirections (NodeRange& r, bool& left, bool& center, bool& right, bool amRightmost);
    bool Covers (const Value& val) const ;
    bool Overlaps (const Constraint& cst) const;
    bool OverlapsNodeRange (const NodeRange& nr) const;

    Value GetSpan (const Value& absmin, const Value& absmax) const;
    Value GetOverlap (const Constraint& cst) const;

    ////////////////////////////////////////////////////////
    // Serialization
    void   Serialize(Packet *pkt);
    uint32 GetLength();
    void   Print(FILE *stream);

 private:
    void GetRouteDirectionsWrapped (NodeRange& r, bool& left, bool& center, bool& right);
};


typedef vector<Constraint> ConstraintVec;
typedef ConstraintVec::iterator ConstraintVecIter;

bool operator== (const Constraint& c1, const Constraint& c2);
bool operator!= (const Constraint& c1, const Constraint& c2);

ostream& operator<<(ostream& out, const Constraint& c);
ostream& operator<<(ostream& out, const Constraint *c);

extern Constraint RANGE_NONE;
#endif // __CONSTRAINT__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
