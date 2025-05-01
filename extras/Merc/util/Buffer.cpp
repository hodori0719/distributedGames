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

/**************************************************************************
  Buffer.cpp

begin           : Nov 6, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/

#include <util/debug.h>
#include <mercury/Packet.h>
#include <util/Buffer.h>

    void Buffer::Resize(uint32 newsize)
{
    ASSERT(newsize > offset+1);
    byte *old = data;
    data = new byte[newsize];
    memcpy(data, old, offset);
    size = newsize;
    delete[] old;
}

void Buffer::Add(byte *toadd, uint32 len)
{
    if (offset + len >= size)
	Resize(MAX(2*size, offset+len+1));
    ASSERT(offset + len < size);
    memcpy(data + offset, toadd, len);
    offset += len;
}

void Buffer::Reset()
{
    if (offset == 0) 
	return;

    delete[] data;
    data = new byte[INITIAL_SIZE];
    offset = 0;
    size = INITIAL_SIZE;
}

Buffer::Buffer(Packet *pkt)
{
    uint32 len = pkt->ReadInt();
    data = new byte[len];
    pkt->ReadBuffer(data, len);
    offset = len;
    size = len;
}

void Buffer::Serialize(Packet *pkt)
{
    pkt->WriteInt(offset);
    pkt->WriteBuffer(data, offset);
}

uint32 Buffer::GetLength()
{
    return 4 + offset;
}

void Buffer::Print(FILE *fp)
{
    ASSERT(0);
}

bool Buffer::operator==(const Buffer& other) const
{
    if (Size() != other.Size())
	return false;

    for (uint32 i=0; i<Size(); i++) {
	if (data[i] != other.data[i]) 
	    return false;
    }

    return true;
}

ostream& operator<<(ostream& os, Buffer *buf)
{
    os << "(Buffer"
       << " size=" << buf->size << " bufPos=" << buf->offset
       << " data=[";
    int i;

    for (i = 0; i < 64; i++) {
	if (i % 16 == 0)
	    os << endl;
	os << merc_va("%02x ", buf->data[i]);
    }
    if (i % 16 != 0)
	os << endl;
    os << (buf->size > 64 ? "..." : "") << "] )";
    return os;
}

ostream& operator<<(ostream& os, Buffer& buf)
{
    return os << &buf;
}

void Buffer::Test(int argc, char **argv)
{
    int trials = 100;
    int rounds, size, i, j, k;
    Buffer *orig, *des;

    for (i=0; i<trials; i++) {
	rounds = (int)(1000*drand48());

	orig = new Buffer();

	for (j=0; j<rounds; j++) {
	    size = (int)(1000*drand48());
	    byte *data = new byte[size];

	    for (k=0; k<size; k++)
		data[k] = k;

	    orig->Add(data, size);
	    delete[] data;
	}

	Packet pkt(orig->GetLength());
	orig->Serialize(&pkt);
	pkt.ResetBufPosition();

	des = new Buffer(&pkt);

	ASSERT(*orig == *des);

	delete orig;
	delete des;
    }
}

void DumpBuffer(byte *buf, int len)
{
    Debug::msg ("len=%d:::", len);
    for (int i = 0; i < len; i++) {
	Debug::msg ("%02x ", buf[i]);
	if (i != 0 && i % 20 == 0)
	    Debug::msg ("\n");
    }
    Debug::msg ("\n");
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
