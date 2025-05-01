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
#include <mercury/Histogram.h>
#include <mercury/Sampling.h>
#include <mercury/common.h>
#include <util/debug.h>

///////////////////////// HistElem ///////////////////////////////

HistElem::HistElem (Packet *pkt) : m_Range (RANGE_NONE)
{
    m_Value = pkt->ReadFloat ();
    m_Range = NodeRange (pkt);
}

uint32 HistElem::GetLength () 
{
    return m_Range.GetLength() + 4;
}

void HistElem::Serialize (Packet *pkt) 
{
    pkt->WriteFloat (m_Value);
    m_Range.Serialize (pkt);
}

void HistElem::Print(FILE *stream) {
    fprintf (stream, "range=");
    m_Range.Print (stream);
    fprintf (stream, " value=%.2f", m_Value);
}

ostream& operator<<(ostream& os, const HistElem *he)
{
    return os << *he;
}

ostream& operator<<(ostream& os, const HistElem &he)
{
    os << "range=" << he.GetRange() << " value=" <<  he.GetValue();
    return os;
}

///////////////////////// Histogram //////////////////////////////

Histogram::Histogram(Packet *pkt) 
{
    int nbkts = pkt->ReadInt();

    for (int i = 0; i < nbkts; i++) {
	m_Buckets.push_back (HistElem (pkt));
    }
}

struct less_bucket_t {
    bool operator () (const HistElem& ae, const HistElem& be) const {
	return ae.GetRange ().GetMin () < be.GetRange ().GetMin ();
    }
};

void Histogram::SortBuckets ()
{
    sort (m_Buckets.begin (), m_Buckets.end (), less_bucket_t ());
}

uint32 Histogram::GetLength () 
{
    uint32 length = 4;
    for (int i = 0, len = m_Buckets.size (); i < len; i++) {
	length += m_Buckets[i].GetLength ();
    }
    return length;
}

void Histogram::Serialize (Packet *pkt) 
{
    int nbkts = m_Buckets.size ();

    pkt->WriteInt (nbkts);
    for (int i = 0; i < nbkts; i++) {
	m_Buckets[i].Serialize (pkt);
    }
}

void Histogram::Print(FILE *stream) 
{
    fprintf (stream, "n_buckets=%d;", m_Buckets.size ());

    for (int i = 0, len = m_Buckets.size (); i < len; i++) {
	m_Buckets[i].Print (stream); fprintf (stream, " ");
    }
}

// check which bucket does m_Range->GetMIn
int Histogram::GetBucketForValue (const Value &val)
{
    for (int i = 0, len = m_Buckets.size (); i < len; i++) {
	HistElem *he = &m_Buckets[i];

	if (he->GetRange ().GetMin () <= val
	    && he->GetRange ().GetMax () >= val)	{
	    return i;
	}
    }
    return -1;
}

ostream& operator<<(ostream& os, const Histogram *histo) {
    return os << *histo;
}

ostream& operator<<(ostream& os, const Histogram &histo)
{
    os << "#buckets=" << histo.GetNumBuckets() << endl;
    for (int i = 0; i < histo.GetNumBuckets(); i++) {
	os << merc_va("\t%d=", i) << histo.GetBucket(i) << ";" << endl;
    }
    os << "================================================" << endl;
    return os;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
