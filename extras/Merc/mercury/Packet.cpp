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

#include <mercury/Packet.h>
#include <cstdlib>

// CHECK: do all constructors initialize everything?
Packet::Packet():m_Size(DEFAULT_PACKET_SIZE), m_BufPosition(0), m_Used(0)
{
    m_Buffer = new byte[m_Size];
}

Packet::Packet(int size):m_Size(size), m_BufPosition(0), m_Used(0)
{
    m_Buffer = new byte[m_Size];
}

Packet::Packet(Packet * pkt)
{
    m_Size = pkt->ReadInt();
    m_BufPosition = 0;
    m_Used = m_Size;	
    m_Buffer = new byte[m_Size];
    pkt->ReadBuffer(m_Buffer, m_Size);
}

Packet::Packet(const Packet& pkt)
{
    m_Size = pkt.m_Size;
    m_BufPosition = pkt.m_BufPosition;
    m_Used = pkt.m_Size;	
    m_Buffer = new byte[m_Size];
    memcpy(m_Buffer, pkt.m_Buffer, m_Size);
}

Packet::~Packet()
{
    if(m_Buffer != NULL)
	delete[]m_Buffer;
    m_Buffer = NULL;
}

void Packet::Serialize(Packet *pkt)
{
    pkt->WriteInt(m_Used);
    pkt->WriteBuffer(m_Buffer, m_Used);
}

uint32 Packet::GetLength()
{
    return 4 + m_Used;
}

void Packet::DumpBuffer()
{
    DumpBuffer(0, m_Size);
}

void Packet::DumpBuffer(int pos, int len)
{
    Debug::msg ("Pos=%d, len=%d:::", pos, len);
    for (int i = 0; i < len; i++) {
	Debug::msg ("%02x ", m_Buffer[pos + i]);
	if (i != 0 && i % 20 == 0)
	    Debug::msg ("\n");
    }
    Debug::msg ("\n");
}

ostream & operator<<(ostream & os, Packet * pkt)
{
    os << "(Packet "
       << "size=" << pkt->m_Size << " bufPos=" << pkt->m_BufPosition
       << " data=[";
    int i;

    for (i = 0; i < 64; i++) {
	if (i % 16 == 0)
	    os << endl;
	os << merc_va("%02x ", pkt->m_Buffer[i]);
	//os << hex << (int) pkt.m_Buffer[i] << " "; // why would "hex" not work? :(
    }
    if (i % 16 != 0)
	os << endl;
    os << (pkt->m_Size > 64 ? "..." : "") << "] )";
    return os;
}

ostream & operator<<(ostream & os, Packet & pkt)
{
    return os << &pkt;
}

void Packet::IncrBufPosition (int incr)
{
    ASSERT (m_BufPosition + incr - 1 < m_Size);
    m_BufPosition += incr;
    m_Used = MAX (m_Used, m_BufPosition);
}

void Packet::WriteBuffer(byte * buf, int len)
{
    ASSERT(m_BufPosition + len - 1 < m_Size);
    memcpy(m_Buffer + m_BufPosition, buf, len);
    m_BufPosition += len;
    m_Used = MAX(m_Used, m_BufPosition);
}

void Packet::ReadBuffer(byte * buf, int len)
{
    ASSERT(m_BufPosition + len - 1 < m_Size);
    memcpy(buf, m_Buffer + m_BufPosition, len);
    m_BufPosition += len;
}

void Packet::WriteByte(byte b)
{
    ASSERT(m_BufPosition < m_Size);
    m_Buffer[m_BufPosition++] = b;
    m_Used = MAX(m_Used, m_BufPosition);
}

byte Packet::ReadByte()
{
    ASSERT(m_BufPosition < m_Size);
    return m_Buffer[m_BufPosition++];
}

// Dont increment m_BufPosition
byte Packet::PeekByte()
{
    ASSERT(m_BufPosition < m_Size);
    return m_Buffer[m_BufPosition];
}

void Packet::WriteBool(bool b)
{
    WriteByte (b ? 0x1 : 0x0);
}

bool Packet::ReadBool()
{
    byte b = ReadByte ();
    if (b == 0x0) 
	return false;
    return true;
}

void Packet::WriteShort(uint16 s)
{
    ASSERT(m_BufPosition + 1 < m_Size);
    uint16 ns = htons(s);
    memcpy(m_Buffer + m_BufPosition, &ns, 2);
    m_BufPosition += 2;
    m_Used = MAX(m_Used, m_BufPosition);
}

uint16 Packet::ReadShort()
{
    ASSERT(m_BufPosition + 1 < m_Size);
    uint16 s;
    memcpy(&s, m_Buffer + m_BufPosition, 2);
    m_BufPosition += 2;

    return ntohs(s);
}

void Packet::WriteInt(uint32 i)
{
    ASSERT(m_BufPosition + 3 < m_Size);
    uint32 ni = htonl(i);
    memcpy(m_Buffer + m_BufPosition, &ni, 4);
    m_BufPosition += 4;
    m_Used = MAX(m_Used, m_BufPosition);
}

uint32 Packet::ReadInt()
{
    ASSERT(m_BufPosition + 3 < m_Size);
    uint32 i;
    memcpy(&i, m_Buffer + m_BufPosition, 4);
    m_BufPosition += 4;

    return ntohl(i);
}

//
// IP addresses obtained from the Winsock library are typically in network byte orders 
// We don't want to unnecessarily perform byte conversions on them. XXX Speaking of which,
// the right thing to do probably is IPAddress::Serialize(). Duh!
//
void Packet::WriteIntNoSwap(uint32 i)
{
    ASSERT(m_BufPosition + 3 < m_Size);
    memcpy(m_Buffer + m_BufPosition, &i, 4);
    m_BufPosition += 4;
    m_Used = MAX(m_Used, m_BufPosition);
}

uint32 Packet::ReadIntNoSwap()
{
    ASSERT(m_BufPosition + 3 < m_Size);
    uint32 i;
    memcpy(&i, m_Buffer + m_BufPosition, 4);
    m_BufPosition += 4;
    return i;
}

void Packet::WriteFloat(float f)
{
    ASSERT(m_BufPosition + 3 < m_Size);

    // careful! plain casting won't do because the compiler will try to be unnecessarily smart!
    union {
	float _f;
	uint32 _i;
    } dat;
    dat._f = f;
    dat._i = htonl(dat._i);
    memcpy(m_Buffer + m_BufPosition, &dat, 4);
    m_BufPosition += 4;
    m_Used = MAX(m_Used, m_BufPosition);
}

float Packet::ReadFloat()
{
    ASSERT(m_BufPosition + 3 < m_Size);

    // again, plain cast won't do...
    union {
	float _f;
	uint32 _i;
    } dat;
    memcpy(&dat, m_Buffer + m_BufPosition, 4);
    m_BufPosition += 4;

    dat._i = ntohl(dat._i);
    return dat._f;
}

void Packet::WriteString(string & s)
{
    WriteInt(s.length());
    WriteBuffer((byte *) s.c_str(), s.length());
}

void Packet::ReadString(string & s)
{
    int strLen = (int) ReadInt();

    // Using a better string constructor now. No unsafe use of the const string.c_str()
    ASSERT(m_BufPosition + strLen - 1 < m_Size);
    s = string((const char *) m_Buffer + m_BufPosition, strLen);

    m_BufPosition += strLen;
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
