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
#ifndef HISTOGRAM__H
#define HISTOGRAM__H

#include <mercury/common.h>
#include <mercury/Constraint.h>
#include <mercury/Packet.h>
#include <vector>

class HistElem : public Serializable {
    NodeRange  m_Range;
    float      m_Value;    

    // ideally, this whole histelem_t should be something
    // the user of the histogram class passes as a parameter
    // to a template; but then you need to be more careful
    // since you need to make sure you dont have to call any
    // constructor for the templated type T;

    // the default copy constructor should work here;
    // since it works with NodeRange
 public:
    HistElem (const NodeRange &range, float val) : m_Range (range), m_Value (val) {}
    HistElem (Packet *pkt);
    virtual ~HistElem() {} 

    const NodeRange& GetRange() const { return m_Range; }
    float GetValue () const { return m_Value; }
    void  SetValue (float v) { m_Value = v; }

    uint32 GetLength();
    void Serialize(Packet *pkt);
    void Print(FILE *stream);
};

ostream& operator<<(ostream& os, const HistElem *he);
ostream& operator<<(ostream& os, const HistElem &he);

class Histogram : public Serializable {
    vector <HistElem> m_Buckets;
 public:
    Histogram () {}
    Histogram (Packet *pkt);
    virtual ~Histogram() {}

    void AddBucket (const NodeRange& r, float v) {
	HistElem e (r, v);
	m_Buckets.push_back (e);
    }

    const HistElem* GetBucket(int i) const { return &m_Buckets[i]; }
    int GetNumBuckets() const { return (int) m_Buckets.size (); }

    void SortBuckets ();
    int GetBucketForValue (const Value &val);

    uint32 GetLength();
    void Serialize(Packet *pkt);
    void Print(FILE *stream);
};

ostream& operator<< (ostream& os, const Histogram *histo);
ostream& operator<< (ostream& os, const Histogram &histo);

#endif // HISTOGRAM__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
