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

#ifndef __REALNET_MARSHALLER__H
#define __REALNET_MARSHALLER__H

#include <list>
#include <map>
#include <util/types.h>
#include <util/debug.h>
#include <wan-env/RealNet.h>
#include "MercRPC.h"
#include "MercRPCMarshaller.h"

typedef map<uint32, MercRPCResult *> ResultMap;
typedef ResultMap::iterator ResultMapIter;

class RealNetMarshaller : public MercRPCMarshaller {
 private:
    IPEndPoint app;
    RealNet *net;
    TransportType proto;
    ResultMap rmap;
    list<MercRPC *> reqs;

    void DoWork();
 public:

    RealNetMarshaller(RealNet *net, TransportType proto = PROTO_TCP,
		      IPEndPoint target = SID_NONE) : 
	net(net), proto(proto), app(target) {}
    virtual ~RealNetMarshaller() {}

    ///////////////////////////////////////////////////////////////////////////
    // Send RPC

    virtual uint32 CallAsync(MercRPC *rpc);
    virtual MercRPCResult *AsyncResult(uint32 nonce);

    virtual void Delete(MercRPCResult *res) {
	if (!res) return;
	net->FreeMessage(res);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Recv RPC

    // must not block, return NULL if none
    virtual MercRPC *GetRPC();
    virtual void HandleRPC(MercRPC *rpc, MercRPCResult *res);

    virtual void Delete(MercRPC *rpc) {
	if (!rpc) return;
	net->FreeMessage(rpc);
    }
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
