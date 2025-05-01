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

#ifndef __MERCURYNODE_CLIENT_STUB__H
#define __MERCURYNODE_CLIENT_STUB__H

#include <map>
#include <mercury/MercuryNode.h>
#include <wan-env/WANMercuryNode.h>
#include <mercury/ID.h>
#include "MercRPC.h"
#include "MercRPCMarshaller.h"
#include "MercRPCDispatcher.h"
#include "PointerMap.h"

class Application;

// xxx --- this entire crappy crap code should be auto-generated. it is a pain
// in the ass to maintain. :P
class MercuryNodeInterface {
 public:
    virtual void RegisterApplication(Application *app) = 0;

    virtual bool IsJoined() = 0;
    virtual bool AllJoined() = 0;
    virtual uint32 GetIP() = 0;
    virtual uint16 GetPort() = 0;

    virtual void Start () = 0;
    virtual void Stop () = 0;
    virtual void SendEvent (Event *pub) = 0;
    virtual void RegisterInterest (Interest *sub) = 0;
    virtual Event* ReadEvent () = 0;  
    //virtual vector<Constraint> GetHubConstraints ();
    //virtual vector< pair<int,string> > GetHubNames ();
    virtual vector<Constraint> GetHubRanges () = 0;
    virtual vector<Neighbor> GetSuccessors (int hubid) = 0;
    virtual vector<Neighbor> GetPredecessors (int hubid) = 0;
    virtual vector<Neighbor> GetLongNeighbors (int hubid) = 0;
    virtual int RegisterSampler (int hubid, Sampler *s) = 0;
    virtual int RegisterLoadSampler (int hubid, LoadSampler *s) = 0;
    virtual int GetSamples (int hubid, Sampler *s, vector<Sample *> *ret) = 0;

    virtual void DoWork(u_long timeout = 10) { }
    virtual void SendApplicationPackets( ) { }
};

class MercuryNodeDummyStub : public MercuryNodeInterface {
 private:
    WANMercuryNode *n;
 public:
    MercuryNodeDummyStub(uint16 port) {
	n = WANMercuryNode::GetInstance(port);
    }

    virtual void RegisterApplication(Application *app) {
	n->RegisterApplication(app);
    }

    virtual bool IsJoined() {
	return n->IsJoined();
    }
    virtual bool AllJoined() {
	return n->AllJoined();
    }
    virtual uint32 GetIP() {
	return n->GetIP();
    }
    virtual uint16 GetPort() {
	return n->GetPort();
    }

    virtual void Start () {
	//n->Start(); Jeff: What the heck is start for?
	n->FireUp ();
    }
    virtual void Stop () {
	n->Stop();
    }
    virtual void SendEvent (Event *pub) {
	n->SendEvent(pub);
    }
    virtual void RegisterInterest (Interest *sub) {
	n->RegisterInterest(sub);
    }
    virtual Event* ReadEvent () {
	return n->ReadEvent();
    } 
    //virtual vector<Constraint> GetHubConstraints ();
    //virtual vector< pair<int,string> > GetHubNames ();
    virtual vector<Constraint> GetHubRanges () {
	return n->GetHubRanges();
    }
    virtual vector<Neighbor> GetSuccessors (int hubid) {
	return n->GetSuccessors(hubid);
    }
    virtual vector<Neighbor> GetPredecessors (int hubid) {
	return n->GetPredecessors(hubid);
    }
    virtual vector<Neighbor> GetLongNeighbors (int hubid) {
	return n->GetLongNeighbors(hubid);
    }
    virtual int RegisterSampler (int hubid, Sampler *s) {
	return n->RegisterSampler(hubid, s);
    }
    virtual int RegisterLoadSampler (int hubid, LoadSampler *s) {
	return n->RegisterLoadSampler(hubid, s);
    }
    virtual int GetSamples (int hubid, Sampler *s, vector<Sample *> *ret) {
	return n->GetSamples(hubid, s, ret);
    }

    virtual void DoWork(u_long timeout = 10) { n->DoWork(timeout); }

    virtual void SendApplicationPackets( ) { n->SendApplicationPackets(); }
};

class MercuryNodeClientStub : public MercuryNodeInterface {
 private:

    static MercuryNodeClientStub *m_Instance;

    MercRPCMarshaller *m;
    PointerMap<Sampler> *samplers;

 protected:

    MercuryNodeClientStub(MercRPCMarshaller *m,
			  PointerMap<Sampler> *s) : m(m), samplers(s) {}

 public:

    static void Init(MercRPCMarshaller *m) {
	ASSERT(m_Instance == NULL);
	// xxx -- this code is fucking ugly. somehow make it consistent
	// with the application client stub
	MercRPCMarshaller::Init(m);
	MercRPCClientDispatcher *d = MercRPCClientDispatcher::GetInstance();
	m->SetDispatcher(d);
	m_Instance = 
	    new MercuryNodeClientStub(m, d->GetSamplers());
    }

    static MercuryNodeClientStub *GetInstance() {
	ASSERT(m_Instance != NULL);
	return m_Instance;
    }

    virtual ~MercuryNodeClientStub() {};

    // xxx - yuck. this is spaghetti code :P
    void RegisterApplication(Application *app) {
	MercRPCClientDispatcher::GetInstance()->RegisterApplication(app);
    }

    bool IsJoined();
    bool AllJoined();
    uint32 GetIP();
    uint16 GetPort();

    void Start ();
    void Stop ();
    void SendEvent (Event *pub);
    void RegisterInterest (Interest *sub);
    Event* ReadEvent ();  
    //vector<Constraint> GetHubConstraints ();
    //vector< pair<int,string> > GetHubNames ();
    vector<Constraint> GetHubRanges ();
    vector<Neighbor> GetSuccessors (int hubid);
    vector<Neighbor> GetPredecessors (int hubid);
    vector<Neighbor> GetLongNeighbors (int hubid);
    int RegisterSampler (int hubid, Sampler *s);
    int RegisterLoadSampler (int hubid, LoadSampler *s);
    int GetSamples (int hubid, Sampler *s, vector<Sample *> *ret);
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
