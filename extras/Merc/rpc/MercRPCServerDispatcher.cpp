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
#include "MercRPCDispatcher.h"
#include "MercRPCMarshaller.h"
#include "RPCableSampler.h"
#include <mercury/MercuryNode.h>

MercRPCServerDispatcher *MercRPCServerDispatcher::m_Instance = NULL;

bool MercRPCServerDispatcher::Dispatch(MercRPC *rpc, MercRPCMarshaller *m)
{
#define SWITCH(e) int _val = (e); if (0)
#define CASE(t) else if (_val == (t)) 
#define DEFAULT() else

    DBG << "Dispatch: " << rpc << endl;

    SWITCH(rpc->GetType()) {}
    CASE(MERCRPC_ISJOINED) {
	bool ret = n->IsJoined();
	MercRPC_BoolResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_ALLJOINED) {
	bool ret = n->AllJoined();
	MercRPC_BoolResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GETIP) {
	uint32 ret = n->GetIP();
	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GETPORT) {
	uint32 ret = n->GetPort();
	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_START) {
	n->Start();
	MercRPC_VoidResult res(rpc);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_STOP) {
	n->Stop();
	MercRPC_VoidResult res(rpc);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_SEND_EVENT) {
	MercRPC_SendEvent *trpc = dynamic_cast<MercRPC_SendEvent *>(rpc);
	Event *v = trpc->GetVal();
	n->SendEvent(v);

	MercRPC_VoidResult res(rpc);
	m->HandleRPC(rpc, &res);
    } 
    CASE(MERCRPC_REGISTER_INTEREST) {
	MercRPC_RegisterInterest *trpc = dynamic_cast<MercRPC_RegisterInterest *>(rpc);
	Interest *v = trpc->GetVal();
	n->RegisterInterest(v);

	MercRPC_VoidResult res(rpc);
	m->HandleRPC(rpc, &res);
    } 
    CASE(MERCRPC_READ_EVENT) {
	Event *ev = n->ReadEvent();

	MercRPC_EventResult res(rpc, ev);
	m->HandleRPC(rpc, &res);
	delete ev;
    }
    CASE(MERCRPC_GET_HUB_CONSTRAINTS) {
	vector<Constraint> vec = n->GetHubConstraints();

	MercRPC_ConstraintVecResult res(rpc, vec);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GET_HUB_RANGES) {
	vector<Constraint> vec = n->GetHubRanges();

	MercRPC_ConstraintVecResult res(rpc, vec);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GET_SUCCESSORS) {
	vector<Neighbor> vec = n->GetSuccessors(rpc->GetHubID());

	MercRPC_NeighborVecResult res(rpc, vec);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GET_PREDECESSORS) {
	vector<Neighbor> vec = n->GetPredecessors(rpc->GetHubID());

	MercRPC_NeighborVecResult res(rpc, vec);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GET_LONG_NEIGHBORS) {
	vector<Neighbor> vec = n->GetLongNeighbors(rpc->GetHubID());

	MercRPC_NeighborVecResult res(rpc, vec);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_REGISTER_SAMPLER) {
	MercRPC_RegisterSampler *trpc = dynamic_cast<MercRPC_RegisterSampler *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	if (s == NULL) {
	    s = new RPCableSampler(trpc->samplerID);
	    samplers.Add(trpc->samplerID, s);
	}
	int ret = n->RegisterSampler(rpc->GetHubID(), s);

	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_REGISTER_LOAD_SAMPLER) {
	MercRPC_RegisterLoadSampler *trpc = dynamic_cast<MercRPC_RegisterLoadSampler *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	if (s == NULL) {
	    s = new RPCableSampler(trpc->samplerID);
	    samplers.Add(trpc->samplerID, s);
	}
	int ret = n->RegisterSampler(rpc->GetHubID(), s);

	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_GET_SAMPLES) {
	MercRPC_GetSamples *trpc = dynamic_cast<MercRPC_GetSamples *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT (s != NULL); // xxx should handle gracefully
	vector<Sample *> vec;
	int ret = n->GetSamples(rpc->GetHubID(), s, &vec);
	ASSERT(ret >= 0); // xxx should pass this along to app!

	MercRPC_SampleVecResult res(rpc, vec);
	m->HandleRPC(rpc, &res);
    }
    DEFAULT() {
	return false;
    }

    return true;

#undef SWITCH
#undef CASE
#undef DEFAULT
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
