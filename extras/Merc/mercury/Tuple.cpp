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
  Tuples = attr_index + value

  $Id: Tuple.cpp 2382 2005-11-03 22:54:59Z ashu $
*/

#include <mercury/Tuple.h>
#include <mercury/Packet.h>

Tuple& Tuple::operator= (const Tuple& t) 
{
    if (this == &t) return *this;

    m_AttrIndex = t.GetAttrIndex();
    m_Val = t.GetValue();

    return *this;
}

Tuple::Tuple(Packet *pkt) 
    : m_AttrIndex (pkt->ReadInt()), m_Val (pkt)
{
}

uint32 Tuple::GetLength()
{
    uint32 length = 4;   // m_AttrIndex
    length += m_Val.GetLength();

    return length;
}

void Tuple::Serialize(Packet * pkt)
{
    pkt->WriteInt(m_AttrIndex);
    m_Val.Serialize(pkt);
}

void Tuple::Print(FILE * stream)
{
    fprintf(stream, "%s=", G_GetTypeName (GetAttrIndex()));
    m_Val.Print(stream);
}

ostream& operator<<(ostream& out, Tuple &t) {
    return out << &t ;
}

ostream& operator<<(ostream& out, Tuple *t) {
    out << G_GetTypeName (t->GetAttrIndex()) << "=" << t->GetValue();
    return out;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
