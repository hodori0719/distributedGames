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

#include <util/debug.h>
#include "MercuryNodeClientStub.h"
#include "MercRPC.h"
#include "MercRPCMarshaller.h"

MercuryNodeClientStub *MercuryNodeClientStub::m_Instance;

bool MercuryNodeClientStub::IsJoined() {
    MercRPC_IsJoined rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_BOOL);

    MercRPC_BoolResult *tres = dynamic_cast<MercRPC_BoolResult *>(res);
    return tres->GetVal();
}

bool MercuryNodeClientStub::AllJoined() {
    MercRPC_AllJoined rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_BOOL);

    MercRPC_BoolResult *tres = dynamic_cast<MercRPC_BoolResult *>(res);
    return tres->GetVal();
}

uint32 MercuryNodeClientStub::GetIP() {
    MercRPC_GetIP rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}

uint16 MercuryNodeClientStub::GetPort() {
    MercRPC_GetPort rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return static_cast<uint16>(tres->GetVal());
}

void MercuryNodeClientStub::Start () {
    MercRPC_Start rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error());
    m->Delete(res);
}

void MercuryNodeClientStub::Stop () {
    MercRPC_Stop rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error());
    m->Delete(res);
}

void MercuryNodeClientStub::SendEvent (Event *pub) {
    MercRPC_SendEvent rpc(pub);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error());
    m->Delete(res);
}

void MercuryNodeClientStub::RegisterInterest (Interest *sub) {
    MercRPC_RegisterInterest rpc(sub);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error());
    m->Delete(res);
}

Event* MercuryNodeClientStub::ReadEvent () {
    MercRPC_ReadEvent rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_EVENT);

    MercRPC_EventResult *tres = dynamic_cast<MercRPC_EventResult *>(res);
    Event *ret = tres->GetVal();
    m->Delete(res);
    return ret;
}

vector<Constraint> MercuryNodeClientStub::GetHubRanges () {
    MercRPC_GetHubRanges rpc;
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && 
	   res->GetType() == MERCRPC_RESULT_CONSTRAINT_VEC);

    MercRPC_ConstraintVecResult *tres = 
	dynamic_cast<MercRPC_ConstraintVecResult *>(res);
    vector<Constraint> ret = tres->val;
    m->Delete(res);
    return ret;
}

vector<Neighbor> MercuryNodeClientStub::GetSuccessors (int hubid) {
    MercRPC_GetSuccessors rpc(hubid);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && 
	   res->GetType() == MERCRPC_RESULT_NEIGHBOR_VEC);

    MercRPC_NeighborVecResult *tres = 
	dynamic_cast<MercRPC_NeighborVecResult *>(res);
    vector<Neighbor> ret = tres->val;
    m->Delete(res);
    return ret;
}

vector<Neighbor> MercuryNodeClientStub::GetPredecessors (int hubid) {
    MercRPC_GetPredecessors rpc(hubid);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && 
	   res->GetType() == MERCRPC_RESULT_NEIGHBOR_VEC);

    MercRPC_NeighborVecResult *tres = 
	dynamic_cast<MercRPC_NeighborVecResult *>(res);
    vector<Neighbor> ret = tres->val;
    m->Delete(res);
    return ret;
}

vector<Neighbor> MercuryNodeClientStub::GetLongNeighbors (int hubid) {
    MercRPC_GetLongNeighbors rpc(hubid);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && 
	   res->GetType() == MERCRPC_RESULT_NEIGHBOR_VEC);

    MercRPC_NeighborVecResult *tres = 
	dynamic_cast<MercRPC_NeighborVecResult *>(res);
    vector<Neighbor> ret = tres->val;
    m->Delete(res);
    return ret;
}

int MercuryNodeClientStub::RegisterSampler (int hubid, Sampler *s) {
    uint32 id = samplers->GetID(s);
    MercRPC_RegisterSampler rpc(id, hubid);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}

int MercuryNodeClientStub::RegisterLoadSampler (int hubid, LoadSampler *s) {
    uint32 id = samplers->GetID(s);
    MercRPC_RegisterLoadSampler rpc(id, hubid);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}

int MercuryNodeClientStub::GetSamples (int hubid, Sampler *s, 
				       vector<Sample *> *ret) {
    uint32 id = samplers->GetID(s);
    MercRPC_RegisterLoadSampler rpc(id, hubid);
    MercRPCResult *res = m->Call(&rpc);
    ASSERT(res && !res->Error() && 
	   res->GetType() == MERCRPC_RESULT_SAMPLE_VEC);

    MercRPC_SampleVecResult *tres = dynamic_cast<MercRPC_SampleVecResult *>(res);
    *ret = tres->val;
    return 0; // XXX
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
