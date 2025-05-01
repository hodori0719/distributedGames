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

#ifndef __TCP_CONNECTION__H
#define __TCP_CONNECTION__H

#include <wan-env/TCPTransport.h>
#include <wan-env/UnbufferedConnection.h>

class TCPConnection : public UnbufferedConnection {

    friend class TCPTransport;

    int _InitNewMessage();
    int _ReadMessage_TCP();

 protected:

    TCPConnection(Transport *t, Socket sock, IPEndPoint *otherEnd);

    int Send(Packet *tofill);
    int PerformRead();

 public:

    Packet *GetNextPacket(PacketAuxInfo* aux);
    virtual ~TCPConnection();
};

#endif // __TCP_CONNECTION__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
