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

#include <mercury/common.h>
#include <mercury/IPEndPoint.h>
#include <mercury/Packet.h>
#include <cstring>

extern int g_IndentLevel;

IPEndPoint::IPEndPoint(uint32 address, int port):m_IP(address), m_Port(port)
{
}


// Figure out my IP address. For now, a simple gethostbyname(gethostname) works. 
// Later on, it might be required to access the registry and get IPs for the interfaces 
IPEndPoint::IPEndPoint(int port):m_Port(port)
{
    char host_name[256];
    if (gethostname(host_name, sizeof(host_name)) != 0) {
	ASSERT(0);
	//EXCEPTION("IPEndPoint::IPEndPoint(): gethostname failed\n");
    }

    try {
	m_IP = IPEndPoint::Parse(host_name);
    }
    catch(exception e) {
	ASSERT(0);
	//EXCEPTION
	//	("IPEndPoint::IPEndPoint(): name lookup failed on hostname\n");
    }
}

IPEndPoint::IPEndPoint(Packet * pkt)
{
    m_IP = pkt->ReadIntNoSwap();
    m_Port = pkt->ReadShort();
}

IPEndPoint::IPEndPoint(char *addrport)
{
    char *dup = strdup (addrport);
    char *p = strchr(dup, ':');

    // invalid
    if (!p) {
	m_IP   = 0;
	m_Port = 0;
	return;
    }

    *p = 0;
    ConstructFromString(dup, atoi(p + 1));
    free (dup);
}

IPEndPoint::IPEndPoint(char *addr, int port)
{
    ConstructFromString(addr, port);
}

void IPEndPoint::ConstructFromString(char *addr, int port)
{
    ///// Init Local SID
    struct hostent *entry = gethostbyname(addr);
    if (entry)
	m_IP = ((struct in_addr *) entry->h_addr)->s_addr;
    else
	m_IP = inet_addr(addr);

    // invalid, set to SID_NONE
    if (m_IP == (uint32)-1) {
	m_IP   = 0;
	m_Port = 0;
    } else {
	m_Port = port;
    }
}

void IPEndPoint::Serialize(Packet * pkt)
{
    pkt->WriteIntNoSwap(m_IP);
    pkt->WriteShort(m_Port);
}

uint32 IPEndPoint::Parse(char *IP)
{
    struct hostent *entry = gethostbyname(IP);
    if (entry) {
	struct in_addr *ip_inaddr =
	    (struct in_addr *) entry->h_addr_list[0];
	return ip_inaddr->s_addr;
    } else {
	ASSERT(0);
	//ThrowException("IP address could not be resolved!\n");
	return 0;
    }
}

/* inline'd
   void IPEndPoint::ToSockAddr(struct sockaddr_in *address)
   {
   // XXX optimize, assume parent does this
   //memset((void *) address, 0, sizeof(struct sockaddr_in));

   address->sin_family = AF_INET;
   address->sin_addr.s_addr = m_IP;
   address->sin_port = htons((unsigned short) m_Port);
   }
*/

void IPEndPoint::ToString(char *bufOut) const
{
    struct in_addr tmp;
    tmp.s_addr = m_IP;
    // xxx is inet_ntoa thread safe?
    sprintf(bufOut, "%s:%d", inet_ntoa(tmp), m_Port);
}

char *IPEndPoint::ToString() const
{
    static char buf[32];
    ToString(buf);
    return buf;
}

ostream & operator<<(ostream & os, IPEndPoint *ipport)
{
    if (ipport == NULL) 
	return os << "(null ip)";
    return os << *ipport;
}

ostream & operator<<(ostream & os, IPEndPoint &ipport)
{
    // make this thread safe...
    char buf[32];
    ipport.ToString(buf);
    return os << buf;
}

bool operator==(const IPEndPoint& s1, const IPEndPoint& s2) {
    return s1.GetIP() == s2.GetIP() && s1.GetPort() == s2.GetPort();
}

bool operator!=(const IPEndPoint& s1, const IPEndPoint& s2) {
    return !(s1 == s2);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
