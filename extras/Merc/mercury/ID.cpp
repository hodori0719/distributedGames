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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

/**************************************************************************
  ID.cpp

begin           : Oct 16, 2003
version         : $Id: ID.cpp 2382 2005-11-03 22:54:59Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#include <mercury/ID.h>

    GUID::GUID(Packet *pkt) {
    m_IP = pkt->ReadInt();
    m_Port = pkt->ReadShort();
    m_LocalOID = pkt->ReadInt();
  }

const char *GUID::ToString(char *bufOut) const
{
    struct in_addr s;
    s.s_addr = m_IP;

    sprintf(bufOut, "%s:%u:%u", inet_ntoa(s), m_Port, m_LocalOID);
    return bufOut;
}

const char *GUID::ToString() const
{
    static char buf[48];
    ToString(buf);
    return buf;
}

void GUID::Serialize(Packet *pkt) {
    pkt->WriteInt(m_IP);
    pkt->WriteShort(m_Port);
    pkt->WriteInt(m_LocalOID);
}

uint32 GUID::GetLength() {
    return 4 + 2 + 4;
}

void GUID::Print(FILE *stream) {
    fprintf(stream, ToString());
}

///////////////////////////////////////////////////////////////////////////////

GUID GUID_NONE = GUID();
SID  SID_NONE  = SID();

ostream& operator<<(ostream& out, const GUID& guid) {
    return (out << guid.ToString());
}

istream& operator>>(istream& in, GUID& guid) {
    uint32 val;
    in >> val;
    guid.m_IP = val;
    in.ignore(1, ':');
    in >> val;
    guid.m_Port = (uint16)val;
    in.ignore(1, ':');
    in >> val;
    guid.m_LocalOID = val;
    return in;
}

bool operator==(const GUID& g1, const GUID& g2) {
    return g1.GetIP() == g2.GetIP() && g1.GetPort() == g2.GetPort() && 
	g1.GetLocalOID() == g2.GetLocalOID();
}

bool operator!=(const GUID& g1, const GUID& g2) {
    return !(g1 == g2);
}

ostream& operator<<(ostream& out, const SID& sid) {
    return (out << sid.ToString());
}

istream& operator>>(istream& in, SID& sid) {
    uint32 val;
    in >> val;
    sid.m_IP = val;
    in.ignore(1, ':');
    in >> val;
    sid.m_Port = val;
    return in;
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
