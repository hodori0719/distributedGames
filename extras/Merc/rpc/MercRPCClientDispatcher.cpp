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

MercRPCClientDispatcher *MercRPCClientDispatcher::m_Instance;

bool MercRPCClientDispatcher::Dispatch(MercRPC *rpc, MercRPCMarshaller *m) {
#define SWITCH(e) int _val = (e); if (0)
#define CASE(t) else if (_val == (t)) 
#define DEFAULT() else

    DBG << "Dispatch: " << rpc << endl;

    SWITCH(rpc->GetType()) {}
    CASE(MERCRPC_SAMPLER_GET_NAME) {
	MercRPC_SamplerGetName *trpc = dynamic_cast<MercRPC_SamplerGetName *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	string ret(s->GetName());

	MercRPC_StringResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_SAMPLER_GET_LOCAL_RADIUS) {
	MercRPC_SamplerGetLocalRadius *trpc = dynamic_cast<MercRPC_SamplerGetLocalRadius *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	int ret = s->GetLocalRadius();

	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_SAMPLER_GET_SAMPLE_LIFETIME) {
	MercRPC_SamplerGetSampleLifetime *trpc = dynamic_cast<MercRPC_SamplerGetSampleLifetime *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	int ret = s->GetSampleLifeTime();

	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    } 
    CASE(MERCRPC_SAMPLER_GET_NUM_REPORT_SAMPLES) {
	MercRPC_SamplerGetNumReportSamples *trpc = dynamic_cast<MercRPC_SamplerGetNumReportSamples *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	int ret = s->GetNumReportSamples();

	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_SAMPLER_GET_RANDOM_WALK_INTERVAL) {
	MercRPC_SamplerGetRandomWalkInterval *trpc = dynamic_cast<MercRPC_SamplerGetRandomWalkInterval *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	int ret = s->GetRandomWalkInterval();

	MercRPC_IntResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_SAMPLER_GET_POINT_ESTIMATE) {
	MercRPC_SamplerGetPointEstimate *trpc = dynamic_cast<MercRPC_SamplerGetPointEstimate *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	Metric *ret = s->GetPointEstimate();

	MercRPC_MetricResult res(rpc, ret);
	m->HandleRPC(rpc, &res);
    }
    CASE(MERCRPC_SAMPLER_MAKE_LOCAL_ESTIMATE) {
	MercRPC_SamplerMakeLocalEstimate *trpc = dynamic_cast<MercRPC_SamplerMakeLocalEstimate *>(rpc);
	Sampler *s = samplers.GetPtr(trpc->samplerID);
	ASSERT(s);
	Metric *ret = s->MakeLocalEstimate(trpc->samples);

	MercRPC_MetricResult res(rpc, ret);
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
