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
/* -*- Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

/*
  Tuples = attr_index + value

  $Id: Tuple.h 2382 2005-11-03 22:54:59Z ashu $
*/

#ifndef __TUPLE__H
#define __TUPLE__H

#include <string>
#include <vector>
#include <mercury/MercuryID.h>

class Tuple : public Serializable {
 private:
    int      m_AttrIndex;
    Value    m_Val;

 public:
    Tuple (int attr, Value& v) : m_AttrIndex (attr), m_Val (v) {}
    Tuple (const Tuple &t) : 
	m_AttrIndex (t.GetAttrIndex ()), m_Val (t.GetValue ()) {}

    Tuple& operator=(const Tuple& t);

    Tuple(Packet *pkt);
    virtual ~Tuple() {}

    const Value &GetValue() const { return m_Val; }           
    int   GetAttrIndex() const { return m_AttrIndex; }

    ////////////////////////////////////////////////////////
    // Serialization
    void     Serialize(Packet *pkt);
    uint32   GetLength();
    void     Print(FILE *stream);
};

typedef vector<Tuple>    TupleVec;
typedef TupleVec::iterator TupleVecIter;

ostream& operator<<(ostream& out, Tuple &t);
ostream& operator<<(ostream& out, Tuple *t);

#endif // __TYPEDATTRVALUE__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
