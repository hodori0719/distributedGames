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

#ifndef __RPCABLE_SAMPLER__H
#define __RPCABLE_SAMPLER__H

#include <string>
#include <mercury/Sampling.h>
#include "Samplers_ClientStub.h"

class RPCableSampler : public LoadSampler /* xxx hack */ {
 private:
    uint32 id;

    mutable string name;
    mutable int localRadius;
    mutable u_long sampleLifeTime;
    mutable int numReportSamples;
    mutable int randomWalkInterval;

    Samplers_ClientStub *stub;
 public:
    RPCableSampler(uint32 id) : 
	id(id), stub(Samplers_ClientStub::GetInstance()),
	name(""), localRadius(0), sampleLifeTime(0), 
	numReportSamples(0), randomWalkInterval(0),
	LoadSampler(NULL) {}

    virtual const char *GetName () const {
	// cache this
	if (name == "") {
	    name = stub->SamplerGetName(id);
	}
	return name.c_str();
    }

    virtual int GetLocalRadius () const {
	// cache this
	if (!localRadius) {
	    localRadius = stub->SamplerGetLocalRadius(id);
	}
	return localRadius;
    }

    virtual u_long GetSampleLifeTime () const {
	// cache this
	if (!sampleLifeTime) {
	    sampleLifeTime = stub->SamplerGetSampleLifeTime(id);
	}
	return sampleLifeTime;
    }

    virtual int GetNumReportSamples () const {
	// cache this
	if (!numReportSamples) {
	    numReportSamples = stub->SamplerGetNumReportSamples(id);
	}
	return numReportSamples;
    }

    virtual int GetRandomWalkInterval () const {
	// cache this
	if (!randomWalkInterval) {
	    randomWalkInterval = stub->SamplerGetRandomWalkInterval(id);
	}
	return randomWalkInterval;
    }

    virtual Metric* GetPointEstimate () {
	return stub->SamplerGetPointEstimate(id);
    }

    virtual Metric* MakeLocalEstimate (vector<Metric* >& samples) {
	return stub->SamplerMakeLocalEstimate(id, samples);
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
