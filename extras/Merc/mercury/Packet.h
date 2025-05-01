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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:nil -*- */

#ifndef __PACKET__H
#define __PACKET__H

#include <mercury/common.h>

/* This class represents the "wire form" of a packet.
   TODO: can perform a bunch of optimizations related to memory allocation, if needed.
   For example, maintain a "buffer pool" of packets since too many packets won't be alive 
   at any particular moment of time... */

class Packet  : public Serializable {
 protected:
    Packet (void *dummy, bool junk) : m_Buffer(NULL) {}

    byte*  m_Buffer;
    int    m_BufPosition;  // overloaded use while writing and reading from network. Careful...
    int    m_Size;         // max size of buffer. set at construct time.   
    int    m_Used;         // the total number of bytes in buffer used

 public:                  
    static const int DEFAULT_PACKET_SIZE = 1400;


    Packet();         // allocate a default size buffer  
    Packet(int size); // so that we can allocate a buffer.. 
    Packet(Packet * pkt); // serializable
    Packet(const Packet& other); // copy constructor
    virtual ~Packet();    

    // Assume serializing after packet is full and buf position is at end
    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *) { ASSERT(0); }

    // XXX: once again these are dangerous; exposing a lot of stuff 
    // out! the main problem is that memcpy, read, and such things 
    // aren't object oriented, so you need to expose the goddamn 
    // buffer there...
    const int GetBufPosition() const { return m_BufPosition;  }
    void IncrBufPosition (int incr);

    const int GetUsed () const { return m_Used; }

    // XXX: this is dangerous!
    byte *GetBuffer () { return m_Buffer; }

    void ResetBufPosition() { m_BufPosition = 0;   }

    int GetMaxSize() { return m_Size;  }

    void DumpBuffer();
    void DumpBuffer(int pos, int len);

    virtual uint32 GetSize() { return m_Size; }

    //
    //  handle endian-ness issues... these routines also modify the buffer-position 
    //  as they read or write data
    // 
    virtual void WriteBuffer(byte *buf, int len);
    virtual void ReadBuffer(byte *buf, int len);

    virtual void WriteByte(byte b); 
    virtual byte ReadByte();
    virtual byte PeekByte();

    virtual void WriteBool (bool b);
    virtual bool ReadBool ();

    virtual void WriteShort(uint16 s);
    virtual uint16 ReadShort();

    virtual void WriteInt(uint32 i);
    virtual uint32 ReadInt();

    virtual void WriteIntNoSwap(uint32 i);
    virtual uint32 ReadIntNoSwap();

    virtual void WriteFloat(float f);
    virtual float ReadFloat();

    virtual void WriteString(string& s);
    virtual void ReadString(string& s);

    //virtual void WriteBlob(size_t len, void *data);
    //virtual size_t ReadBlob(void *data);

    friend ostream& operator<<(ostream& os, Packet& pkt);
    friend ostream& operator<<(ostream& os, Packet *pkt);
};

ostream& operator<<(ostream& os, Packet& pkt);
ostream& operator<<(ostream& os, Packet *pkt);

#endif // __PACKET__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
