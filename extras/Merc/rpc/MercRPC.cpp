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

#include "MercRPC.h"
//#include "RPCableEvent.h"
//#include "RPCableInterest.h"
#include "Application_MercRPC.h"

bool g_IsRPCServer = false;
IPEndPoint g_RPCAddr = SID_NONE;

MsgType MERCRPC, MERCRPC_RESULT,

    MERCRPC_RESULT_VOID, MERCRPC_RESULT_STRING,
    MERCRPC_RESULT_BOOL, MERCRPC_RESULT_INT, MERCRPC_RESULT_FLOAT,
    MERCRPC_RESULT_NEIGHBOR, MERCRPC_RESULT_NEIGHBOR_VEC,
    MERCRPC_RESULT_CONSTRAINT_VEC, MERCRPC_RESULT_SAMPLE_VEC,
    MERCRPC_RESULT_EVENT, MERCRPC_RESULT_INTEREST, MERCRPC_RESULT_METRIC,

    /*	
      MERCRPC_JOIN_BEGIN_CALLBACK, MERCRPC_JOIN_END_CALLBACK,
      MERCRPC_LEAVE_BEGIN_CALLBACK, MERCRPC_LEAVE_END_CALLBACK,
      MERCRPC_INCRID_BEGIN_CALLBACK, MERCRPC_INCRID_END_CALLBACK,
      MERCRPC_EVENT_CALLBACK,
    */

    MERCRPC_SAMPLER_GET_NAME, MERCRPC_SAMPLER_GET_LOCAL_RADIUS,
    MERCRPC_SAMPLER_GET_SAMPLE_LIFETIME, 
    MERCRPC_SAMPLER_GET_NUM_REPORT_SAMPLES,
    MERCRPC_SAMPLER_GET_RANDOM_WALK_INTERVAL,
    MERCRPC_SAMPLER_GET_POINT_ESTIMATE,
    MERCRPC_SAMPLER_MAKE_LOCAL_ESTIMATE,

    /*
      MERCRPC_GET_LOAD_CALLBACK, MERCRPC_GET_PERCENTILE_ID_CALLBACK,
    */

    MERCRPC_ISJOINED, MERCRPC_ALLJOINED, MERCRPC_GETIP, MERCRPC_GETPORT,
    MERCRPC_START, MERCRPC_STOP,
    MERCRPC_SEND_EVENT, MERCRPC_REGISTER_INTEREST, 
    MERCRPC_READ_EVENT, MERCRPC_GET_HUB_CONSTRAINTS,
    MERCRPC_GET_HUB_RANGES, MERCRPC_GET_SUCCESSORS, MERCRPC_GET_PREDECESSORS,
    MERCRPC_GET_LONG_NEIGHBORS, MERCRPC_REGISTER_SAMPLER,
    MERCRPC_REGISTER_LOAD_SAMPLER, MERCRPC_GET_SAMPLES;

EventType RPCABLE_EVENT;
InterestType RPCABLE_INTEREST;

void MercRPC_SetIsRPCServer(bool v) {
    g_IsRPCServer = v;
}

bool MercRPC_IsRPCServer() {
    return g_IsRPCServer;
}

void MercRPC_RegisterTypes() {
    //MERCRPC = REGISTER_TYPE (Message, MercRPC);
    //MERCRPC_RESULT = REGISTER_TYPE (Message, MercRPCResult);

    MERCRPC_RESULT_VOID = REGISTER_TYPE (Message, MercRPC_VoidResult);
    MERCRPC_RESULT_BOOL = REGISTER_TYPE (Message, MercRPC_BoolResult);
    MERCRPC_RESULT_INT = REGISTER_TYPE (Message, MercRPC_IntResult); 
    MERCRPC_RESULT_FLOAT = REGISTER_TYPE (Message, MercRPC_FloatResult);
    MERCRPC_RESULT_STRING = REGISTER_TYPE (Message, MercRPC_StringResult);
    MERCRPC_RESULT_NEIGHBOR = REGISTER_TYPE (Message, MercRPC_NeighborResult);
    MERCRPC_RESULT_NEIGHBOR_VEC = REGISTER_TYPE (Message, MercRPC_NeighborVecResult);
    MERCRPC_RESULT_CONSTRAINT_VEC = REGISTER_TYPE (Message, MercRPC_ConstraintVecResult);
    MERCRPC_RESULT_SAMPLE_VEC = REGISTER_TYPE (Message, MercRPC_SampleVecResult);
    MERCRPC_RESULT_EVENT = REGISTER_TYPE (Message, MercRPC_EventResult); 
    MERCRPC_RESULT_INTEREST = REGISTER_TYPE (Message, MercRPC_InterestResult);
    MERCRPC_RESULT_METRIC = REGISTER_TYPE (Message, MercRPC_MetricResult);

    MERCRPC_ISJOINED = REGISTER_TYPE (Message, MercRPC_IsJoined);
    MERCRPC_ALLJOINED = REGISTER_TYPE (Message, MercRPC_AllJoined);
    MERCRPC_GETIP = REGISTER_TYPE (Message, MercRPC_GetIP);
    MERCRPC_GETPORT = REGISTER_TYPE (Message, MercRPC_GetPort);
    MERCRPC_START = REGISTER_TYPE (Message, MercRPC_Start);
    MERCRPC_STOP = REGISTER_TYPE (Message, MercRPC_Stop);
    MERCRPC_SEND_EVENT = REGISTER_TYPE (Message, MercRPC_SendEvent);
    MERCRPC_REGISTER_INTEREST = REGISTER_TYPE (Message, MercRPC_RegisterInterest);
    MERCRPC_READ_EVENT = REGISTER_TYPE (Message, MercRPC_ReadEvent); 
    MERCRPC_GET_HUB_CONSTRAINTS = REGISTER_TYPE (Message, MercRPC_GetHubConstraints);
    MERCRPC_GET_HUB_RANGES = REGISTER_TYPE (Message, MercRPC_GetHubRanges);
    MERCRPC_GET_SUCCESSORS = REGISTER_TYPE (Message, MercRPC_GetSuccessors);
    MERCRPC_GET_PREDECESSORS = REGISTER_TYPE (Message, MercRPC_GetPredecessors);
    MERCRPC_GET_LONG_NEIGHBORS = REGISTER_TYPE (Message, MercRPC_GetLongNeighbors); 
    MERCRPC_REGISTER_SAMPLER = REGISTER_TYPE (Message, MercRPC_RegisterSampler);
    MERCRPC_REGISTER_LOAD_SAMPLER = REGISTER_TYPE (Message, MercRPC_RegisterLoadSampler);
    MERCRPC_GET_SAMPLES = REGISTER_TYPE (Message, MercRPC_GetSamples);

    MERCRPC_SAMPLER_GET_NAME = REGISTER_TYPE (Message, MercRPC_SamplerGetName);
    MERCRPC_SAMPLER_GET_LOCAL_RADIUS = REGISTER_TYPE (Message, MercRPC_SamplerGetLocalRadius);
    MERCRPC_SAMPLER_GET_SAMPLE_LIFETIME = REGISTER_TYPE (Message, MercRPC_SamplerGetSampleLifetime);
    MERCRPC_SAMPLER_GET_NUM_REPORT_SAMPLES = REGISTER_TYPE (Message, MercRPC_SamplerGetNumReportSamples);
    MERCRPC_SAMPLER_GET_RANDOM_WALK_INTERVAL = REGISTER_TYPE (Message, MercRPC_SamplerGetRandomWalkInterval);
    MERCRPC_SAMPLER_GET_POINT_ESTIMATE = REGISTER_TYPE (Message, MercRPC_SamplerGetPointEstimate);
    MERCRPC_SAMPLER_MAKE_LOCAL_ESTIMATE = REGISTER_TYPE (Message, MercRPC_SamplerMakeLocalEstimate);

    Application_RegisterTypes();
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
