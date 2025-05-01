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

#include <mercury/Hub.h>
#include "Samplers_ClientStub.h"

Samplers_ClientStub *Samplers_ClientStub::m_Instance = NULL;

string Samplers_ClientStub::SamplerGetName (uint32 id) {
    MercRPC_SamplerGetName rpc(id);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return string(""); // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_STRING);

    MercRPC_StringResult *tres = dynamic_cast<MercRPC_StringResult *>(res);
    return tres->GetVal();
}
int Samplers_ClientStub::SamplerGetLocalRadius (uint32 id) {
    MercRPC_SamplerGetLocalRadius rpc(id);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return 0; // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}
u_long Samplers_ClientStub::SamplerGetSampleLifeTime (uint32 id) {
    MercRPC_SamplerGetSampleLifetime rpc(id);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return 0; // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}
int Samplers_ClientStub::SamplerGetNumReportSamples (uint32 id) {
    MercRPC_SamplerGetNumReportSamples rpc(id);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return 0; // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}
int Samplers_ClientStub::SamplerGetRandomWalkInterval (uint32 id) {
    MercRPC_SamplerGetRandomWalkInterval rpc(id);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return 0; // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_INT);

    MercRPC_IntResult *tres = dynamic_cast<MercRPC_IntResult *>(res);
    return tres->GetVal();
}
Metric* Samplers_ClientStub::SamplerGetPointEstimate (uint32 id) {
    MercRPC_SamplerGetPointEstimate rpc(id);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return NULL; // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_METRIC);

    MercRPC_MetricResult *tres = dynamic_cast<MercRPC_MetricResult *>(res);
    return tres->GetVal();
}
Metric* Samplers_ClientStub::SamplerMakeLocalEstimate (uint32 id, vector<Metric* >& samples) {
    MercRPC_SamplerMakeLocalEstimate rpc(id, samples);
    MercRPCResult *res = m->Call(&rpc);
    if (!res) return NULL; // default value if no client
    ASSERT(res && !res->Error() && res->GetType() == MERCRPC_RESULT_METRIC);

    MercRPC_MetricResult *tres = dynamic_cast<MercRPC_MetricResult *>(res);
    return tres->GetVal();
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
