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

#ifndef __MERCRPC_MARSHALLER__H
#define __MERCRPC_MARSHALLER__H

#include <util/types.h>
#include <util/debug.h>

class MercRPC;
class MercRPCResult;
class MercRPCDispatcher;

/**
 * Interface responsible for sending and receiving RPCs.
 */
class MercRPCMarshaller {
 private:
    static MercRPCMarshaller *m_Instance;
 protected:
    MercRPCDispatcher *m_Dispatcher;

    void DoWork();

 public:
    static void Init(MercRPCMarshaller *i) {
	m_Instance = i;
    }
    static MercRPCMarshaller *GetInstance() {
	ASSERT(m_Instance);
	return m_Instance;
    }

    MercRPCMarshaller() : m_Dispatcher(NULL) {}

    void SetDispatcher(MercRPCDispatcher *d) {
	m_Dispatcher = d;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Send RPC

    virtual MercRPCResult *Call(MercRPC *rpc) {
	return AsyncResult( CallAsync(rpc) );
    }
    virtual uint32 CallAsync(MercRPC *rpc) = 0;
    // while this call blocks, it should periodically call
    // DoWork() otherwise the server might block if it requires
    // us to process and rpc
    virtual MercRPCResult *AsyncResult(uint32 nonce) = 0;

    virtual void Delete(MercRPCResult *res) = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Recv RPC

    // must not block, return NULL if none
    virtual MercRPC *GetRPC() = 0;
    virtual void HandleRPC(MercRPC *rpc, MercRPCResult *res) = 0;
    virtual void Delete(MercRPC *rpc) = 0;
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
