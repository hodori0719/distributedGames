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

#ifndef __MERC_RPC__H
#define __MERC_RPC__H

#include <vector>
#include <mercury/MercuryNode.h>
#include <mercury/Message.h>
#include <mercury/Sampling.h>
#include <util/Utils.h>

///////////////////////////////////////////////////////////////////////////////

class MercRPC;
class MercRPCMarshaller;

/**
 * The portion that goes on the rpc server (where the "real") class is
 * located. The assumption is a subclass for class T it will have a
 * constructor:
 *
 * subclass(T *actual) { ... }
 */ 
class MercRPCServerStub {
 public:
    virtual bool Dispatch(MercRPC *rpc, MercRPCMarshaller *m) = 0;
};

///////////////////////////////////////////////////////////////////////////////

//
// Message RPCs for Mercury RPC server
//

// XXX Some day fix make_stub.pl to auto generate all of these messages

extern MsgType MERCRPC, MERCRPC_RESULT,

    MERCRPC_RESULT_VOID, MERCRPC_RESULT_STRING,
    MERCRPC_RESULT_BOOL, MERCRPC_RESULT_INT, MERCRPC_RESULT_FLOAT,
    MERCRPC_RESULT_NEIGHBOR, MERCRPC_RESULT_NEIGHBOR_VEC,
    MERCRPC_RESULT_CONSTRAINT_VEC, MERCRPC_RESULT_SAMPLE_VEC,
    MERCRPC_RESULT_EVENT, MERCRPC_RESULT_INTEREST, MERCRPC_RESULT_METRIC,

    MERCRPC_SAMPLER_GET_NAME, MERCRPC_SAMPLER_GET_LOCAL_RADIUS,
    MERCRPC_SAMPLER_GET_SAMPLE_LIFETIME, 
    MERCRPC_SAMPLER_GET_NUM_REPORT_SAMPLES,
    MERCRPC_SAMPLER_GET_RANDOM_WALK_INTERVAL,
    MERCRPC_SAMPLER_GET_POINT_ESTIMATE,
    MERCRPC_SAMPLER_MAKE_LOCAL_ESTIMATE,

    MERCRPC_ISJOINED, MERCRPC_ALLJOINED, MERCRPC_GETIP, MERCRPC_GETPORT,
    MERCRPC_START, MERCRPC_STOP,
    MERCRPC_SEND_EVENT, MERCRPC_REGISTER_INTEREST, 
    MERCRPC_READ_EVENT, MERCRPC_GET_HUB_CONSTRAINTS,
    MERCRPC_GET_HUB_RANGES, MERCRPC_GET_SUCCESSORS, MERCRPC_GET_PREDECESSORS,
    MERCRPC_GET_LONG_NEIGHBORS, MERCRPC_REGISTER_SAMPLER,
    MERCRPC_REGISTER_LOAD_SAMPLER, MERCRPC_GET_SAMPLES;

extern bool g_IsRPCServer;
extern IPEndPoint g_RPCAddr;

void MercRPC_SetIsRPCServer(bool v);
bool MercRPC_IsRPCServer();
void MercRPC_RegisterTypes();

struct MercRPCMessage : public Message {
    enum { RPC, RES };

    byte rpcType;

    MercRPCMessage(uint32 t) : rpcType((byte)t), Message() {}
    MercRPCMessage(uint32 t, Packet *pkt) : rpcType((byte)t), Message(pkt) {}
};

struct MercRPC : public MercRPCMessage {
    DECLARE_TYPE(Message, MercRPC);

    uint32 m_Hubid;
    uint32 m_rpcNonce;

    MercRPC(uint32 hubid) : MercRPCMessage(MercRPCMessage::RPC) {
	sender = g_RPCAddr;
	m_Hubid = hubid;
	m_rpcNonce = CreateNonce();
    }
    MercRPC(Packet *pkt) : MercRPCMessage(MercRPCMessage::RPC, pkt) {
	m_Hubid = pkt->ReadInt();
	m_rpcNonce = pkt->ReadInt();
    }
    void Serialize(Packet *pkt) {
	Message::Serialize(pkt);
	pkt->WriteInt(m_Hubid);
	pkt->WriteInt(m_rpcNonce);
    }
    uint32 GetLength() {
	return Message::GetLength() + sizeof(m_Hubid) + sizeof(m_rpcNonce);
    }
    virtual ~MercRPC() {}

    virtual const char* TypeString() { return "MERC_RPC"; }

    uint32 GetHubID() { return m_Hubid; }
    uint32 GetRPCNonce() { return m_rpcNonce; }
};

struct MercRPCResult : public MercRPCMessage {
    DECLARE_TYPE(Message, MercRPCResult);

    enum {
	CODE_ERR = 0x1,
    };

    uint32 m_ErrorCode;
    uint32 m_rpcNonce;

    MercRPCResult(MercRPC *req) : MercRPCMessage(MercRPCMessage::RES) {
	sender = g_RPCAddr;
	m_rpcNonce = req->GetRPCNonce();
	m_ErrorCode = 0;	
    }
    MercRPCResult(Packet *pkt) : MercRPCMessage(MercRPCMessage::RES, pkt) {
	m_rpcNonce = pkt->ReadInt();
	m_ErrorCode = pkt->ReadInt();
    }
    void Serialize(Packet *pkt) {
	Message::Serialize(pkt);
	pkt->WriteInt(m_rpcNonce);
	pkt->WriteInt(m_ErrorCode);
    }
    uint32 GetLength() {
	return Message::GetLength() + sizeof(m_rpcNonce) + sizeof(m_ErrorCode);
    }
    virtual ~MercRPCResult() {}

    virtual const char* TypeString() { return "MERC_RPC_RESULT"; }

    uint32 GetRPCNonce() { return m_rpcNonce; }
    void SetErrorCode(uint32 code) {
	m_ErrorCode = code;
    }
    uint32 GetErrorCode() {
	return m_ErrorCode;
    }
    bool Error() {
	return m_ErrorCode != 0;
    }
};

struct MercRPC_VoidResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_VoidResult);
    MercRPC_VoidResult(MercRPC *req) : MercRPCResult(req) {}
    MercRPC_VoidResult(Packet *pkt) : MercRPCResult(pkt) {}

    const char* TypeString() { return "MERC_RPC_VOID_RESULT"; }
};

struct MercRPC_BoolResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_BoolResult);

    bool m_Val;

    MercRPC_BoolResult(MercRPC *req, bool val) : 
	m_Val(val), MercRPCResult(req) {
    }
    MercRPC_BoolResult(Packet *pkt) : MercRPCResult(pkt) {
	m_Val = pkt->ReadBool();
    }
    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteBool(m_Val);
    }
    uint32 GetLength() {
	return MercRPCResult::GetLength() + 1;
    }

    uint32 GetVal() { return m_Val; }

    const char* TypeString() { return "MERC_RPC_BOOL_RESULT"; }
};

struct MercRPC_IntResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_IntResult);

    uint32 m_Val;

    MercRPC_IntResult(MercRPC *req, uint32 val) : 
	m_Val(val), MercRPCResult(req) {
    }
    MercRPC_IntResult(Packet *pkt) : MercRPCResult(pkt) {
	m_Val = pkt->ReadInt();
    }
    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteInt(m_Val);
    }
    uint32 GetLength() {
	return MercRPCResult::GetLength() + sizeof(m_Val);
    }

    uint32 GetVal() { return m_Val; }

    const char* TypeString() { return "MERC_RPC_INT_RESULT"; }
};

struct MercRPC_FloatResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_FloatResult);

    float m_Val;

    MercRPC_FloatResult(MercRPC *req, float val) : 
	m_Val(val), MercRPCResult(req) {
    }
    MercRPC_FloatResult(Packet *pkt) : MercRPCResult(pkt) {
	m_Val = pkt->ReadFloat();
    }
    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteFloat(m_Val);
    }
    uint32 GetLength() {
	return MercRPCResult::GetLength() + sizeof(m_Val);
    }

    float GetVal() { return m_Val; }

    const char* TypeString() { return "MERC_RPC_FLOAT_RESULT"; }
};

struct MercRPC_StringResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_StringResult);

    string m_Val;

    MercRPC_StringResult(MercRPC *req, const string& val) : 
	m_Val(val), MercRPCResult(req) {
    }
    MercRPC_StringResult(Packet *pkt) : MercRPCResult(pkt) {
	pkt->ReadString(m_Val);
    }
    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteString(m_Val);
    }
    uint32 GetLength() {
	return MercRPCResult::GetLength() + 4 + m_Val.length();
    }

    string& GetVal() { return m_Val; }

    const char* TypeString() { return "MERC_RPC_STRING_RESULT"; }
};

struct MercRPC_NeighborResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_NeighborResult);

    Neighbor m_Val;

    MercRPC_NeighborResult(MercRPC *req, 
			   const Neighbor& val) : 
	m_Val(val), MercRPCResult(req) {
    }
    MercRPC_NeighborResult(Packet *pkt) : MercRPCResult(pkt) {
	m_Val = Neighbor(pkt);
    }
    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	m_Val.Serialize(pkt);
    }
    uint32 GetLength() {
	return MercRPCResult::GetLength() + m_Val.GetLength();
    }

    Neighbor& GetVal() { return m_Val; }

    const char* TypeString() { return "MERC_RPC_NEIGHBOR_RESULT"; }
};

struct MercRPC_NeighborVecResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_NeighborVecResult);

    vector<Neighbor> val;

    MercRPC_NeighborVecResult(MercRPC *req, vector<Neighbor>& val) : 
	val(val), MercRPCResult(req) {}
    MercRPC_NeighborVecResult(Packet *pkt) : MercRPCResult(pkt) {
	uint32 len = pkt->ReadInt();
	for (uint32 i=0; i<len; i++) {
	    val.push_back( Neighbor(pkt) );
	}
    }

    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteInt(val.size());
	for (uint32 i=0; i<val.size(); i++) {
	    val[i].Serialize(pkt);
	}
    }
    uint32 GetLength() {
	uint32 len = MercRPCResult::GetLength() + 4;
	for (uint32 i=0; i<val.size(); i++) {
	    len += val[i].GetLength();
	}
	return len;
    }

    const char* TypeString() { return "MERC_RPC_NEIGHBORVEC_RESULT"; }
};

struct MercRPC_ConstraintVecResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_ConstraintVecResult);

    vector<Constraint> val;

    MercRPC_ConstraintVecResult(MercRPC *req, vector<Constraint>& val) : 
	val(val), MercRPCResult(req) {}
    MercRPC_ConstraintVecResult(Packet *pkt) : MercRPCResult(pkt) {
	uint32 len = pkt->ReadInt();
	for (uint32 i=0; i<len; i++) {
	    val.push_back( Constraint(pkt) );
	}
    }

    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteInt(val.size());
	for (uint32 i=0; i<val.size(); i++) {
	    val[i].Serialize(pkt);
	}
    }
    uint32 GetLength() {
	uint32 len = MercRPCResult::GetLength() + 4;
	for (uint32 i=0; i<val.size(); i++) {
	    len += val[i].GetLength();
	}
	return len;
    }

    const char* TypeString() { return "MERC_RPC_CONSTRAINTVEC_RESULT"; }
};

struct MercRPC_SampleVecResult : public MercRPCResult {
    DECLARE_TYPE(Message, MercRPC_SampleVecResult);

    vector<Sample *> val;

    MercRPC_SampleVecResult(MercRPC *req, vector<Sample *>& val) : 
	MercRPCResult(req) {
	for (uint32 i=0; i<val.size(); i++) {
	    this->val.push_back(val[i]->Clone());
	}
    }
    MercRPC_SampleVecResult(Packet *pkt) : MercRPCResult(pkt) {
	uint32 len = pkt->ReadInt();
	for (uint32 i=0; i<len; i++) {
	    val.push_back( new Sample(pkt) );
	}
    }
    virtual ~MercRPC_SampleVecResult() {
	for (uint32 i=0; i<val.size(); i++) {
	    delete val[i];
	}
    }

    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteInt(val.size());
	for (uint32 i=0; i<val.size(); i++) {
	    val[i]->Serialize(pkt);
	}
    }
    uint32 GetLength() {
	uint32 len = MercRPCResult::GetLength() + 4;
	for (uint32 i=0; i<val.size(); i++) {
	    len += val[i]->GetLength();
	}
	return len;
    }

    const char* TypeString() { return "MERC_RPC_SAMPLEVEC_RESULT"; }
};

template <class T>
struct MercRPC_ObjectResult : public MercRPCResult {
    T *m_Val;

    MercRPC_ObjectResult(MercRPC *req, T *val) : 
	m_Val(val), MercRPCResult(req) {
    }
    MercRPC_ObjectResult(Packet *pkt) : MercRPCResult(pkt) {
	bool b = pkt->ReadBool();
	if (b)
	    m_Val = CreateObject<T>(pkt);
	else
	    m_Val = NULL;
    }

    void Serialize(Packet *pkt) {
	MercRPCResult::Serialize(pkt);
	pkt->WriteBool(m_Val != NULL);
	if (m_Val != NULL)
	    m_Val->Serialize(pkt);
    }
    uint32 GetLength() {
	return MercRPCResult::GetLength() + 1 + (m_Val?m_Val->GetLength():0);
    }

    // assume this gets called and result belongs to app
    T *GetVal() { return m_Val; }
};

#define MercRPC_ObjectResultInst(name, T) \
struct name : public MercRPC_ObjectResult<T> { \
    DECLARE_TYPE(Message, name); \
	name(MercRPC *req, T *val) : MercRPC_ObjectResult<T>(req, val) {} \
	name(Packet *pkt) : MercRPC_ObjectResult<T>(pkt) {} \
	const char* TypeString() { return "MERC_RPC_" #T "_RESULT"; } \
}

MercRPC_ObjectResultInst(MercRPC_EventResult, Event);
MercRPC_ObjectResultInst(MercRPC_InterestResult, Interest);
MercRPC_ObjectResultInst(MercRPC_MetricResult, Metric);

///////////////////////////////////////////////////////////////////////////////

#define MercRPC_NullCall(name) \
struct MercRPC_##name : public MercRPC { \
    DECLARE_TYPE(Message, MercRPC_##name); \
	MercRPC_##name() : MercRPC((uint32)0) {} \
	MercRPC_##name(Packet *pkt) : MercRPC(pkt) {} \
	virtual const char* TypeString() { return "MERC_RPC_" #name; } \
}

#define MercRPC_HubIDCall(name) \
struct MercRPC_##name : public MercRPC { \
    DECLARE_TYPE(Message, MercRPC_##name); \
	MercRPC_##name(uint32 hubid) : MercRPC(hubid) {} \
	MercRPC_##name(Packet *pkt) : MercRPC(pkt) {} \
	virtual const char* TypeString() { return "MERC_RPC_" #name; } \
}

///////////////////////////////////////////////////////////////////////////////

#define MercRPC_SamplerCallback(name) \
struct MercRPC_Sampler##name : public MercRPC { \
    DECLARE_TYPE(Message, MercRPC_Sampler##name); \
	uint32 samplerID; \
	MercRPC_Sampler##name(uint32 id) : samplerID(id), MercRPC((uint32)0) {} \
	MercRPC_Sampler##name(Packet *pkt) : MercRPC(pkt) { \
	    samplerID = pkt->ReadInt(); \
	} \
    void Serialize(Packet *pkt) { \
	MercRPC::Serialize(pkt); \
	    pkt->WriteInt(samplerID); \
    } \
    uint32 GetLength() { \
	return MercRPC::GetLength() + sizeof(samplerID); \
    } \
    virtual const char* TypeString() { return "MERC_RPC_" #name; } \
}

MercRPC_SamplerCallback(GetName);
MercRPC_SamplerCallback(GetLocalRadius);
MercRPC_SamplerCallback(GetSampleLifetime);
MercRPC_SamplerCallback(GetNumReportSamples);
MercRPC_SamplerCallback(GetRandomWalkInterval);
MercRPC_SamplerCallback(GetPointEstimate);

struct MercRPC_SamplerMakeLocalEstimate : public MercRPC { 
    DECLARE_TYPE(Message, MercRPC_SamplerMakeLocalEstimate); 

    uint32 samplerID;
    vector<Metric *> samples;

    MercRPC_SamplerMakeLocalEstimate(uint32 id, vector<Metric *>& samples) : 
	samplerID(id), samples(samples), MercRPC((uint32)0) {} 
    MercRPC_SamplerMakeLocalEstimate(Packet *pkt) : MercRPC(pkt) { 
	samplerID = pkt->ReadInt(); 
	uint32 len = pkt->ReadInt();
	for (uint32 i=0; i<len; i++) {
	    samples.push_back( CreateObject<Metric>(pkt) );
	}
    } 
    void Serialize(Packet *pkt) { 
	MercRPC::Serialize(pkt); 
	pkt->WriteInt(samplerID);
	for (uint32 i=0; i<samples.size(); i++) {
	    samples[i]->Serialize(pkt);
	}
    } 
    uint32 GetLength() { 
	uint32 len = MercRPC::GetLength() + sizeof(samplerID);
	for (uint32 i=0; i<samples.size(); i++) {
	    len += samples[i]->GetLength();
	}
	return len;
    } 

    virtual const char* TypeString() { return "MERC_RPC_SamplerMakeLocalEstimate"; }
};

///////////////////////////////////////////////////////////////////////////////

MercRPC_NullCall(IsJoined);
MercRPC_NullCall(AllJoined);
MercRPC_NullCall(GetIP);
MercRPC_NullCall(GetPort);

MercRPC_NullCall(Start);
MercRPC_NullCall(Stop);

struct MercRPC_SendEvent : public MercRPC {
    DECLARE_TYPE(Message, MercRPC_SendEvent);

    Event *m_Val;

    MercRPC_SendEvent(Event *val) : 
	m_Val(val->Clone()), MercRPC((uint32)0) {
    }
    MercRPC_SendEvent(Packet *pkt) : MercRPC(pkt) {
	m_Val = CreateObject<Event>(pkt);
    }
    virtual ~MercRPC_SendEvent() {
	delete m_Val;
    }

    void Serialize(Packet *pkt) {
	MercRPC::Serialize(pkt);
	m_Val->Serialize(pkt);
    }
    uint32 GetLength() {
	return MercRPC::GetLength() + m_Val->GetLength();
    }

    Event *GetVal() { return m_Val; }

    virtual const char* TypeString() { return "MERC_RPC_SENDEVENT"; }
};

struct MercRPC_RegisterInterest : public MercRPC {
    DECLARE_TYPE(Message, MercRPC_RegisterInterest);

    Interest *m_Val;

    MercRPC_RegisterInterest(Interest *val) : 
	m_Val(val->Clone()), MercRPC((uint32)0) {
    }
    MercRPC_RegisterInterest(Packet *pkt) : MercRPC(pkt) {
	m_Val = CreateObject<Interest>(pkt);
    }
    virtual ~MercRPC_RegisterInterest() {
	delete m_Val;
    }

    void Serialize(Packet *pkt) {
	MercRPC::Serialize(pkt);
	m_Val->Serialize(pkt);
    }
    uint32 GetLength() {
	return MercRPC::GetLength() + m_Val->GetLength();
    }

    Interest *GetVal() { return m_Val; }

    virtual const char* TypeString() { return "MERC_RPC_REGISTERINTEREST"; }
};

MercRPC_NullCall(ReadEvent);

MercRPC_NullCall(GetHubConstraints);

// GetHubNames

MercRPC_NullCall(GetHubRanges);

MercRPC_HubIDCall(GetSuccessors);
MercRPC_HubIDCall(GetPredecessors);
MercRPC_HubIDCall(GetLongNeighbors);

struct MercRPC_RegisterSampler : public MercRPC {
    DECLARE_TYPE(Message, MercRPC_RegisterSampler);

    uint32 samplerID;

    MercRPC_RegisterSampler(uint32 samplerID, uint32 hubid) : 
	samplerID(samplerID), MercRPC(hubid) {
    }
    MercRPC_RegisterSampler(Packet *pkt) : MercRPC(pkt) {
	samplerID = pkt->ReadInt();
    }

    void Serialize(Packet *pkt) {
	MercRPC::Serialize(pkt);
	pkt->WriteInt(samplerID);
    }
    uint32 GetLength() {
	return MercRPC::GetLength() + sizeof(samplerID);
    }

    virtual const char* TypeString() { return "MERC_RPC_REGISTERSAMPLER"; }
};

struct MercRPC_RegisterLoadSampler : public MercRPC {
    DECLARE_TYPE(Message, MercRPC_RegisterLoadSampler);

    uint32 samplerID;

    MercRPC_RegisterLoadSampler(uint32 samplerID, uint32 hubid) : 
	samplerID(samplerID), MercRPC(hubid) {
    }
    MercRPC_RegisterLoadSampler(Packet *pkt) : MercRPC(pkt) {
	samplerID = pkt->ReadInt();
    }

    void Serialize(Packet *pkt) {
	MercRPC::Serialize(pkt);
	pkt->WriteInt(samplerID);
    }
    uint32 GetLength() {
	return MercRPC::GetLength() + sizeof(samplerID);
    }

    virtual const char* TypeString() { return "MERC_RPC_REGISTERLOADSAMPLER"; }
};

struct MercRPC_GetSamples : public MercRPC {
    DECLARE_TYPE(Message, MercRPC_GetSamples);

    uint32 samplerID;

    MercRPC_GetSamples(uint32 samplerID, uint32 hubid) : 
	samplerID(samplerID), MercRPC(hubid) {
    }
    MercRPC_GetSamples(Packet *pkt) : MercRPC(pkt) {
	samplerID = pkt->ReadInt();
    }

    void Serialize(Packet *pkt) {
	MercRPC::Serialize(pkt);
	pkt->WriteInt(samplerID);
    }
    uint32 GetLength() {
	return MercRPC::GetLength() + sizeof(samplerID);
    }

    virtual const char* TypeString() { return "MERC_RPC_GETSAMPLES"; }
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
