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
  Buffer.h

begin           : Nov 6, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <mercury/common.h>

    /**
     * Auto resizing buffer.
     */
    class Buffer : public Serializable
{
 protected:
    void Resize(uint32 newsize);
 public:
    static const uint32 INITIAL_SIZE = 128;

    byte *data;
    uint32   offset;
    uint32   size;

    Buffer() : 
	data(new byte[INITIAL_SIZE]), offset(0), size(INITIAL_SIZE) {}
    Buffer(uint32 size) :
	data(new byte[size]), offset(0), size(size) {}

    virtual ~Buffer() { delete[] data; }

    void Add(byte *data, uint32 len);
    uint32 Size() const { return offset; }
    void Reset();
    bool operator==(const Buffer& other) const;

    Buffer(Packet *pkt);
    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *fp);

    ///// DEBUGGING

    static void Test(int argc, char **argv);
};

ostream& operator<<(ostream& os, Buffer& pkt);
ostream& operator<<(ostream& os, Buffer *pkt);

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
