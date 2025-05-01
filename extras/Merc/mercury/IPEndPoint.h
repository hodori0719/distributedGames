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
#ifndef __NETUTILS__H
#define __NETUTILS__H

#include "common.h"
#include <mercury/Packet.h>

// Encapsulate a ip:port pair and some functions related to it. 
// Basic point is that you NEVER get confused between what might be in network
// byte order and what would be in host byte order in other parts of the code.

struct IPEndPoint : public Serializable {
    uint32   m_IP;     // network byte order
    uint16   m_Port;   // host byte order 

    virtual ~IPEndPoint () {} 

    static const uint32 LOOPBACK_ADDRBITS = 0x7f000001;

    IPEndPoint() : m_IP(0), m_Port(0) {}
    IPEndPoint(uint32 address, int port);
    IPEndPoint(int port); // figure out my own IP address...

    // WARNING: these constructors block on a DNS lookup
    // They construct SID_NONE if the name fails to resolve
    IPEndPoint(char *addrport); // name:port string 
    IPEndPoint(char *addr, int port);
    void ConstructFromString(char *addr, int port);

    uint32 NetworkToHostOrder() { return ntohl(m_IP); }
    bool IsLoopBack() { return m_IP == LOOPBACK_ADDRBITS; }

    inline uint32 GetIP() const { return m_IP; }
    inline uint16 GetPort() const { return m_Port; }

    static uint32 Parse(char *ip);

    inline void ToSockAddr(struct sockaddr_in *address)
	{
	    // XXX optimize, assume parent does this
	    //memset((void *) address, 0, sizeof(struct sockaddr_in));

	    address->sin_family = AF_INET;
	    address->sin_addr.s_addr = m_IP;
	    address->sin_port = htons((unsigned short) m_Port);
	}

    void ToString(char *bufOut) const;
    char *ToString() const;

    IPEndPoint(Packet *pkt);
    void Serialize(Packet *pkt);
    uint32 GetLength() {
	return sizeof(m_IP) + sizeof(m_Port);
    }
    void Print(FILE *stream) {
	fprintf(stream, "%s", ToString());
    }

    friend ostream& operator<<(ostream& os, IPEndPoint* ipport);
    friend ostream& operator<<(ostream& os, IPEndPoint& ipport);
};

bool operator== (const IPEndPoint& s1, const IPEndPoint& s2);
bool operator!= (const IPEndPoint& s1, const IPEndPoint& s2);

#endif // __NETUTILS__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
