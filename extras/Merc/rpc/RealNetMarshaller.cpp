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

#include "MercRPC.h"
#include "RealNetMarshaller.h"

uint32 RealNetMarshaller::CallAsync(MercRPC *rpc) {
    if (app == SID_NONE) {
	WARN << "Dropping rpc since no application available" << endl;
	return 0;
    }

    DBG << "Call(" << app << "):[" << rpc->GetRPCNonce() 
	<< "] " << rpc << endl; 
    net->SendMessage(rpc, &app, proto);
    return rpc->GetRPCNonce();
}

MercRPCResult *RealNetMarshaller::AsyncResult(uint32 nonce) {
    if (nonce == 0) 
	return NULL;

    while (rmap.find(nonce) == rmap.end()) {
	MercRPCMarshaller::DoWork();
	DoWork();
    }
    MercRPCResult *res = rmap[nonce];
    DBG << "Return(" << app << "):[" << nonce << "] " << res << endl;
    rmap.erase(nonce);
    return res;
}

MercRPC *RealNetMarshaller::GetRPC() {
    DoWork();
    if (reqs.size() == 0) {
	return NULL;
    }
    MercRPC *rpc = reqs.front();
    reqs.pop_front();
    return rpc;
}

void RealNetMarshaller::HandleRPC(MercRPC *rpc, MercRPCResult *res) {
    if (app == SID_NONE) {
	WARN << "Dropping rpc result since no application available" << endl;
	return;
    }

    DBG << "Handle(" << app << "):[" << rpc->GetRPCNonce() << "] " 
	<< res << endl;
    net->SendMessage(res, &app, proto);
    return;
}

void RealNetMarshaller::DoWork() {
    IPEndPoint from((uint32)0, 0);
    Message *msg = 0;
    ConnStatusType status;

    do {
	status = net->GetNextMessage(&from, &msg);

	//DBG << "Got ConnectionStatus: " << g_ConnStatusStrings[status]
	//    << endl;

	switch (status) {
	case CONN_NEWINCOMING:
	    INFO << "app registered at: " << from << endl;
	    app = from;
	case CONN_OK: {

	    MercRPCMessage *rpcmsg = dynamic_cast<MercRPCMessage *>(msg);
	    if (!rpcmsg) {
		WARN << "got non-rpc message: " << msg->TypeString() << endl;
		delete msg;
		msg = NULL;
	    }
	    if (rpcmsg->rpcType == MercRPCMessage::RPC) {
		MercRPC *rpc = dynamic_cast<MercRPC *>(msg);
		reqs.push_back(rpc);
	    } else if (rpcmsg->rpcType == MercRPCMessage::RES) {
		MercRPCResult *res = dynamic_cast<MercRPCResult *>(msg);
		rmap[res->GetRPCNonce()] = res;
	    } else {
		WARN << "got bad rpc type: " << rpcmsg->rpcType << endl;
		delete msg;
		msg = NULL;
	    }

	    break;
	}
	case CONN_CLOSED:
	    INFO << "app unregistered at: " << from << endl;
	    app = SID_NONE;
	    break;
	case CONN_ERROR:
	    WARN << "connection error from: " << from.ToString() << endl;
	    break;
	case CONN_NOMSG:
	    break;
	default:
	    Debug::die("Hmm... got weird connection status.");
	    break;
	}

    } while (msg == NULL && status != CONN_NOMSG);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
