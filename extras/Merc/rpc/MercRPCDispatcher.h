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

#ifndef __MERCRPC_DISPATCHER__H
#define __MERCRPC_DISPATCHER__H

#include <util/types.h>
#include <util/debug.h>
#include <mercury/Sampling.h>
#include "PointerMap.h"
#include "MercRPC.h"
#include "MercRPCMarshaller.h"
#include "Application_MercRPC.h"
//#include "ApplicationHandler.h"

class MercuryNode;

/**
 * Interface for handling incoming RPCs from the marshaller
 */
class MercRPCDispatcher {
 protected:
    vector<MercRPCServerStub *> stubs;

    MercRPCDispatcher() {}
 public:

    void RegisterHandler(MercRPCServerStub * s) {
	stubs.push_back(s);
    }

    void ProcessRPCs() {
	MercRPCMarshaller *m = MercRPCMarshaller::GetInstance();
	MercRPC *rpc;
	while ((rpc = m->GetRPC()) != NULL) {
	    bool handled = false;
	    for (uint32 i=0; i < stubs.size(); i++) {
		handled = stubs[i]->Dispatch(rpc, m);
		if (handled) break;
	    }
	    if (!handled) {
		WARN << "unhandled RPC: " << rpc << endl;
	    }
	}
    }
};

/**
 * On the mercury node side.
 */
class MercRPCServerDispatcher : 
public MercRPCDispatcher, public MercRPCServerStub {
 private:
    static MercRPCServerDispatcher *m_Instance;

    MercuryNode *n;
    PointerMap<Sampler> samplers;

 protected:
    MercRPCServerDispatcher(MercuryNode *n) : n(n) {};

 public:
    bool Dispatch(MercRPC *rpc, MercRPCMarshaller *m);

    static void Init(MercuryNode *n) {
	ASSERT(m_Instance == NULL);
	m_Instance = new MercRPCServerDispatcher(n);
	m_Instance->RegisterHandler(m_Instance);
    }
    static MercRPCServerDispatcher *GetInstance() {
	ASSERT(m_Instance);
	return m_Instance;
    }

    virtual ~MercRPCServerDispatcher() {}

    PointerMap<Sampler> *GetSamplers() {
	return &samplers;
    }
};

/**
 * On the application side.
 */
class MercRPCClientDispatcher : 
public MercRPCDispatcher, public MercRPCServerStub {
 private:
    static MercRPCClientDispatcher *m_Instance;
    PointerMap<Sampler> samplers;

    string SamplerGetName (uint32 id) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return string(s->GetName());
    }
    int SamplerGetLocalRadius (uint32 id) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return s->GetLocalRadius();
    }
    u_long SamplerGetSampleLifeTime (uint32 id) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return s->GetSampleLifeTime();
    }
    int SamplerGetNumReportSamples (uint32 id) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return s->GetNumReportSamples();
    }
    int SamplerGetRandomWalkInterval (uint32 id) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return s->GetRandomWalkInterval();
    }
    Metric* SamplerGetPointEstimate (uint32 id) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return s->GetPointEstimate();
    }
    Metric* SamplerMakeLocalEstimate (uint32 id, vector<Metric* >& samples) {
	Sampler *s = samplers.GetPtr(id);
	ASSERT(s);
	return s->MakeLocalEstimate(samples);
    }

 protected:
    MercRPCClientDispatcher() {};

 public:
    bool Dispatch(MercRPC *rpc, MercRPCMarshaller *m);

    static MercRPCClientDispatcher *GetInstance() {
	if (m_Instance == NULL) {
	    m_Instance = new MercRPCClientDispatcher();
	    m_Instance->RegisterHandler(m_Instance);
	}
	return m_Instance;
    }

    virtual ~MercRPCClientDispatcher() {}

    void RegisterApplication(Application *app) {
	m_Instance->RegisterHandler(new Application_ServerStub(app));
    }

    PointerMap<Sampler> *GetSamplers() {
	return &samplers;
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
