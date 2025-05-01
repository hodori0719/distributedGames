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
#ifndef __NEIGHBOR__H
#define __NEIGHBOR__H   

#include <mercury/ID.h>
#include <mercury/Constraint.h>
#include <mercury/IPEndPoint.h>

/**
 * This is the information about a node that is exported to
 * the application. This is a subset of the 'Peer' class 
 * inside Mercury.
 **/
struct Neighbor : public Serializable {
    SID addr;
    NodeRange range;
    u_long rtt_millis;

    Neighbor() {};
    Neighbor (SID addr, NodeRange range, u_long rtt_millis) : 
	addr (addr), range (range), rtt_millis (rtt_millis) {}

    Neighbor(Packet *pkt) : addr(pkt), range(pkt), rtt_millis(pkt->ReadInt()) {
    }
    void Serialize(Packet *pkt) {
	addr.Serialize(pkt);
	range.Serialize(pkt);
	pkt->WriteInt(rtt_millis);
    }
    uint32 GetLength() {
	return addr.GetLength() + range.GetLength() + 4;
    }
    void Print(FILE *) {
    }
};

ostream& operator<<(ostream& out, const Neighbor& nbr);
ostream& operator<<(ostream& out, const Neighbor* nbr);

#endif /* __NEIGHBOR__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
