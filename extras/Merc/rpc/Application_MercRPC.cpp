#include <mercury/AutoSerializer.h>
#include <rpc/MercRPCMarshaller.h>
#include "Application_MercRPC.h"

MsgType MERCRPC_APPLICATION_JOINBEGIN;
MsgType MERCRPC_APPLICATION_JOINBEGIN_RESULT;
MsgType MERCRPC_APPLICATION_JOINEND;
MsgType MERCRPC_APPLICATION_JOINEND_RESULT;
MsgType MERCRPC_APPLICATION_LEAVEBEGIN;
MsgType MERCRPC_APPLICATION_LEAVEBEGIN_RESULT;
MsgType MERCRPC_APPLICATION_LEAVEEND;
MsgType MERCRPC_APPLICATION_LEAVEEND_RESULT;
MsgType MERCRPC_APPLICATION_RANGEEXPANDED;
MsgType MERCRPC_APPLICATION_RANGEEXPANDED_RESULT;
MsgType MERCRPC_APPLICATION_RANGECONTRACTED;
MsgType MERCRPC_APPLICATION_RANGECONTRACTED_RESULT;
MsgType MERCRPC_APPLICATION_EVENTROUTE;
MsgType MERCRPC_APPLICATION_EVENTROUTE_RESULT;
MsgType MERCRPC_APPLICATION_EVENTLINEAR;
MsgType MERCRPC_APPLICATION_EVENTLINEAR_RESULT;
MsgType MERCRPC_APPLICATION_EVENTATRENDEZVOUS;
MsgType MERCRPC_APPLICATION_EVENTATRENDEZVOUS_RESULT;
MsgType MERCRPC_APPLICATION_EVENTINTERESTMATCH;
MsgType MERCRPC_APPLICATION_EVENTINTERESTMATCH_RESULT;
MsgType MERCRPC_APPLICATION_INTERESTROUTE;
MsgType MERCRPC_APPLICATION_INTERESTROUTE_RESULT;
MsgType MERCRPC_APPLICATION_INTERESTLINEAR;
MsgType MERCRPC_APPLICATION_INTERESTLINEAR_RESULT;
MsgType MERCRPC_APPLICATION_INTERESTATRENDEZVOUS;
MsgType MERCRPC_APPLICATION_INTERESTATRENDEZVOUS_RESULT;
MsgType MERCRPC_APPLICATION_ISLEAVEJOINOK;
MsgType MERCRPC_APPLICATION_ISLEAVEJOINOK_RESULT;

void Application_RegisterTypes() {
   MERCRPC_APPLICATION_JOINBEGIN =
      REGISTER_TYPE(Message,MercRPC_Application_JoinBegin);
   MERCRPC_APPLICATION_JOINBEGIN_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_JoinBegin_Result);
   MERCRPC_APPLICATION_JOINEND =
      REGISTER_TYPE(Message,MercRPC_Application_JoinEnd);
   MERCRPC_APPLICATION_JOINEND_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_JoinEnd_Result);
   MERCRPC_APPLICATION_LEAVEBEGIN =
      REGISTER_TYPE(Message,MercRPC_Application_LeaveBegin);
   MERCRPC_APPLICATION_LEAVEBEGIN_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_LeaveBegin_Result);
   MERCRPC_APPLICATION_LEAVEEND =
      REGISTER_TYPE(Message,MercRPC_Application_LeaveEnd);
   MERCRPC_APPLICATION_LEAVEEND_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_LeaveEnd_Result);
   MERCRPC_APPLICATION_RANGEEXPANDED =
      REGISTER_TYPE(Message,MercRPC_Application_RangeExpanded);
   MERCRPC_APPLICATION_RANGEEXPANDED_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_RangeExpanded_Result);
   MERCRPC_APPLICATION_RANGECONTRACTED =
      REGISTER_TYPE(Message,MercRPC_Application_RangeContracted);
   MERCRPC_APPLICATION_RANGECONTRACTED_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_RangeContracted_Result);
   MERCRPC_APPLICATION_EVENTROUTE =
      REGISTER_TYPE(Message,MercRPC_Application_EventRoute);
   MERCRPC_APPLICATION_EVENTROUTE_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_EventRoute_Result);
   MERCRPC_APPLICATION_EVENTLINEAR =
      REGISTER_TYPE(Message,MercRPC_Application_EventLinear);
   MERCRPC_APPLICATION_EVENTLINEAR_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_EventLinear_Result);
   MERCRPC_APPLICATION_EVENTATRENDEZVOUS =
      REGISTER_TYPE(Message,MercRPC_Application_EventAtRendezvous);
   MERCRPC_APPLICATION_EVENTATRENDEZVOUS_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_EventAtRendezvous_Result);
   MERCRPC_APPLICATION_EVENTINTERESTMATCH =
      REGISTER_TYPE(Message,MercRPC_Application_EventInterestMatch);
   MERCRPC_APPLICATION_EVENTINTERESTMATCH_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_EventInterestMatch_Result);
   MERCRPC_APPLICATION_INTERESTROUTE =
      REGISTER_TYPE(Message,MercRPC_Application_InterestRoute);
   MERCRPC_APPLICATION_INTERESTROUTE_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_InterestRoute_Result);
   MERCRPC_APPLICATION_INTERESTLINEAR =
      REGISTER_TYPE(Message,MercRPC_Application_InterestLinear);
   MERCRPC_APPLICATION_INTERESTLINEAR_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_InterestLinear_Result);
   MERCRPC_APPLICATION_INTERESTATRENDEZVOUS =
      REGISTER_TYPE(Message,MercRPC_Application_InterestAtRendezvous);
   MERCRPC_APPLICATION_INTERESTATRENDEZVOUS_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_InterestAtRendezvous_Result);
   MERCRPC_APPLICATION_ISLEAVEJOINOK =
      REGISTER_TYPE(Message,MercRPC_Application_IsLeaveJoinOK);
   MERCRPC_APPLICATION_ISLEAVEJOINOK_RESULT =
      REGISTER_TYPE(Message,MercRPC_Application_IsLeaveJoinOK_Result);
}

bool Application_ServerStub::Dispatch(MercRPC *rpc, MercRPCMarshaller *m) {
    int type = rpc->GetType();
    if (0) {}
    else if (type == MERCRPC_APPLICATION_JOINBEGIN) {
	MercRPC_Application_JoinBegin *mrpc = dynamic_cast<MercRPC_Application_JoinBegin *>(rpc);
         MercRPC_Application_JoinBegin_Result res(mrpc);
         iface->JoinBegin(mrpc->succ);
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_JOINEND) {
	MercRPC_Application_JoinEnd *mrpc = dynamic_cast<MercRPC_Application_JoinEnd *>(rpc);
         MercRPC_Application_JoinEnd_Result res(mrpc);
         iface->JoinEnd(mrpc->succ);
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_LEAVEBEGIN) {
	MercRPC_Application_LeaveBegin *mrpc = dynamic_cast<MercRPC_Application_LeaveBegin *>(rpc);
         MercRPC_Application_LeaveBegin_Result res(mrpc);
         iface->LeaveBegin();
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_LEAVEEND) {
	MercRPC_Application_LeaveEnd *mrpc = dynamic_cast<MercRPC_Application_LeaveEnd *>(rpc);
         MercRPC_Application_LeaveEnd_Result res(mrpc);
         iface->LeaveEnd();
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_RANGEEXPANDED) {
	MercRPC_Application_RangeExpanded *mrpc = dynamic_cast<MercRPC_Application_RangeExpanded *>(rpc);
         MercRPC_Application_RangeExpanded_Result res(mrpc);
         iface->RangeExpanded(mrpc->oldrange, mrpc->newrange);
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_RANGECONTRACTED) {
	MercRPC_Application_RangeContracted *mrpc = dynamic_cast<MercRPC_Application_RangeContracted *>(rpc);
         MercRPC_Application_RangeContracted_Result res(mrpc);
         iface->RangeContracted(mrpc->oldrange, mrpc->newrange);
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_EVENTROUTE) {
	MercRPC_Application_EventRoute *mrpc = dynamic_cast<MercRPC_Application_EventRoute *>(rpc);
         MercRPC_Application_EventRoute_Result res(mrpc, iface->EventRoute(mrpc->ev, mrpc->lastHop));
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_EVENTLINEAR) {
	MercRPC_Application_EventLinear *mrpc = dynamic_cast<MercRPC_Application_EventLinear *>(rpc);
         MercRPC_Application_EventLinear_Result res(mrpc, iface->EventLinear(mrpc->ev, mrpc->lastHop));
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_EVENTATRENDEZVOUS) {
	MercRPC_Application_EventAtRendezvous *mrpc = dynamic_cast<MercRPC_Application_EventAtRendezvous *>(rpc);
         MercRPC_Application_EventAtRendezvous_Result res(mrpc, iface->EventAtRendezvous(mrpc->ev, mrpc->lastHop, mrpc->nhops));
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_EVENTINTERESTMATCH) {
	MercRPC_Application_EventInterestMatch *mrpc = dynamic_cast<MercRPC_Application_EventInterestMatch *>(rpc);
         MercRPC_Application_EventInterestMatch_Result res(mrpc);
         iface->EventInterestMatch(mrpc->ev, mrpc->in, mrpc->subscriber);
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_INTERESTROUTE) {
	MercRPC_Application_InterestRoute *mrpc = dynamic_cast<MercRPC_Application_InterestRoute *>(rpc);
         MercRPC_Application_InterestRoute_Result res(mrpc, iface->InterestRoute(mrpc->in, mrpc->lastHop));
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_INTERESTLINEAR) {
	MercRPC_Application_InterestLinear *mrpc = dynamic_cast<MercRPC_Application_InterestLinear *>(rpc);
         MercRPC_Application_InterestLinear_Result res(mrpc, iface->InterestLinear(mrpc->in, mrpc->lastHop));
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_INTERESTATRENDEZVOUS) {
	MercRPC_Application_InterestAtRendezvous *mrpc = dynamic_cast<MercRPC_Application_InterestAtRendezvous *>(rpc);
         MercRPC_Application_InterestAtRendezvous_Result res(mrpc, iface->InterestAtRendezvous(mrpc->in, mrpc->lastHop));
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else if (type == MERCRPC_APPLICATION_ISLEAVEJOINOK) {
	MercRPC_Application_IsLeaveJoinOK *mrpc = dynamic_cast<MercRPC_Application_IsLeaveJoinOK *>(rpc);
         MercRPC_Application_IsLeaveJoinOK_Result res(mrpc, iface->IsLeaveJoinOK());
         m->HandleRPC(rpc, &res);
	 return true;
    }
    else {
	return false;
    }
}

Application_ClientStub *Application_ClientStub::m_Instance = NULL;

Application_ClientStub *Application_ClientStub::GetInstance() {
    if (m_Instance == NULL) {
	m_Instance =
	    new Application_ClientStub(MercRPCMarshaller::GetInstance());
    }
    return m_Instance;
}

PubsubStore* Application_ClientStub::GetPubsubStore (int hubId) {
   return NULL;
}

void Application_ClientStub::JoinBegin (const IPEndPoint& succ) {
   MercRPC_Application_JoinBegin rpc(succ);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_JOINBEGIN_RESULT);
   return;
}

void Application_ClientStub::JoinEnd (const IPEndPoint& succ) {
   MercRPC_Application_JoinEnd rpc(succ);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_JOINEND_RESULT);
   return;
}

void Application_ClientStub::LeaveBegin () {
   MercRPC_Application_LeaveBegin rpc;   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_LEAVEBEGIN_RESULT);
   return;
}

void Application_ClientStub::LeaveEnd () {
   MercRPC_Application_LeaveEnd rpc;   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_LEAVEEND_RESULT);
   return;
}

void Application_ClientStub::RangeExpanded (const NodeRange& oldrange, const NodeRange& newrange) {
   MercRPC_Application_RangeExpanded rpc(oldrange, newrange);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_RANGEEXPANDED_RESULT);
   return;
}

void Application_ClientStub::RangeContracted (const NodeRange& oldrange, const NodeRange& newrange) {
   MercRPC_Application_RangeContracted rpc(oldrange, newrange);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_RANGECONTRACTED_RESULT);
   return;
}

int Application_ClientStub::EventRoute (Event * ev, const IPEndPoint& lastHop) {
   MercRPC_Application_EventRoute rpc(ev, lastHop);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((int)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_EVENTROUTE_RESULT);
   MercRPC_Application_EventRoute_Result *tres = dynamic_cast<MercRPC_Application_EventRoute_Result *>(res);
   return tres->res;
}

int Application_ClientStub::EventLinear (Event * ev, const IPEndPoint& lastHop) {
   MercRPC_Application_EventLinear rpc(ev, lastHop);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((int)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_EVENTLINEAR_RESULT);
   MercRPC_Application_EventLinear_Result *tres = dynamic_cast<MercRPC_Application_EventLinear_Result *>(res);
   return tres->res;
}

EventProcessType Application_ClientStub::EventAtRendezvous (Event * ev, const IPEndPoint& lastHop, int nhops) {
   MercRPC_Application_EventAtRendezvous rpc(ev, lastHop, nhops);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((EventProcessType)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_EVENTATRENDEZVOUS_RESULT);
   MercRPC_Application_EventAtRendezvous_Result *tres = dynamic_cast<MercRPC_Application_EventAtRendezvous_Result *>(res);
   return tres->res;
}

void Application_ClientStub::EventInterestMatch (const Event * ev, const Interest * in, const IPEndPoint& subscriber) {
   MercRPC_Application_EventInterestMatch rpc(ev, in, subscriber);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ;
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_EVENTINTERESTMATCH_RESULT);
   return;
}

int Application_ClientStub::InterestRoute (Interest * in, const IPEndPoint& lastHop) {
   MercRPC_Application_InterestRoute rpc(in, lastHop);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((int)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_INTERESTROUTE_RESULT);
   MercRPC_Application_InterestRoute_Result *tres = dynamic_cast<MercRPC_Application_InterestRoute_Result *>(res);
   return tres->res;
}

int Application_ClientStub::InterestLinear (Interest * in, const IPEndPoint& lastHop) {
   MercRPC_Application_InterestLinear rpc(in, lastHop);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((int)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_INTERESTLINEAR_RESULT);
   MercRPC_Application_InterestLinear_Result *tres = dynamic_cast<MercRPC_Application_InterestLinear_Result *>(res);
   return tres->res;
}

InterestProcessType Application_ClientStub::InterestAtRendezvous (Interest * in, const IPEndPoint& lastHop) {
   MercRPC_Application_InterestAtRendezvous rpc(in, lastHop);
   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((InterestProcessType)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_INTERESTATRENDEZVOUS_RESULT);
   MercRPC_Application_InterestAtRendezvous_Result *tres = dynamic_cast<MercRPC_Application_InterestAtRendezvous_Result *>(res);
   return tres->res;
}

bool Application_ClientStub::IsLeaveJoinOK () {
   MercRPC_Application_IsLeaveJoinOK rpc;   MercRPCResult *res = m->Call(&rpc);
   if (!res) return ((bool)0);
   ASSERT(res && !res->Error() && res->GetType() == MERCRPC_APPLICATION_ISLEAVEJOINOK_RESULT);
   MercRPC_Application_IsLeaveJoinOK_Result *tres = dynamic_cast<MercRPC_Application_IsLeaveJoinOK_Result *>(res);
   return tres->res;
}

