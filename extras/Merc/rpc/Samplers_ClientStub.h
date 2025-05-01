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

#ifndef __SAMPLERS_CLIENT_STUB__H
#define __SAMPLERS_CLIENT_STUB__H

#include <map>
#include <mercury/ID.h>
#include <mercury/Event.h>
#include <mercury/Interest.h>
#include <mercury/Application.h>
#include "MercRPC.h"
#include "MercRPCMarshaller.h"

/**
 * This is the unified interface on the merc RPC server which makes callbacks
 * to the application.
 */
class Samplers_ClientStub {
 private:

    static Samplers_ClientStub *m_Instance;

    MercRPCMarshaller *m;

 protected:

    Samplers_ClientStub(MercRPCMarshaller *m) : m(m) {}

 public:

    static Samplers_ClientStub *GetInstance() {
	if (m_Instance == NULL) {
	    m_Instance = 
		new Samplers_ClientStub(MercRPCMarshaller::GetInstance());
	}
	return m_Instance;
    }

    virtual ~Samplers_ClientStub() {};

    string SamplerGetName (uint32 id);
    int SamplerGetLocalRadius (uint32 id);
    u_long SamplerGetSampleLifeTime (uint32 id);
    int SamplerGetNumReportSamples (uint32 id);
    int SamplerGetRandomWalkInterval (uint32 id);
    Metric* SamplerGetPointEstimate (uint32 id);
    Metric* SamplerMakeLocalEstimate (uint32 id, vector<Metric* >& samples);
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
