#ifndef __APPLICATION_MERCRPC_H_
#define __APPLICATION_MERCRPC_H_

#include <rpc/MercRPC.h>
#include <mercury/Application.h>

void Application_RegisterTypes();

extern MsgType MERCRPC_APPLICATION_JOINBEGIN;
struct MercRPC_Application_JoinBegin : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_JoinBegin);
   IPEndPoint succ;

   MercRPC_Application_JoinBegin(const IPEndPoint& succ) : succ(succ), MercRPC((uint32)0) {}
   MercRPC_Application_JoinBegin(Packet *pkt) : MercRPC(pkt) {
      succ = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      succ.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += succ.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_JOINBEGIN"; }
};

extern MsgType MERCRPC_APPLICATION_JOINBEGIN_RESULT;
struct MercRPC_Application_JoinBegin_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_JoinBegin_Result);

   MercRPC_Application_JoinBegin_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_JoinBegin_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_JOINBEGIN_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_JOINEND;
struct MercRPC_Application_JoinEnd : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_JoinEnd);
   IPEndPoint succ;

   MercRPC_Application_JoinEnd(const IPEndPoint& succ) : succ(succ), MercRPC((uint32)0) {}
   MercRPC_Application_JoinEnd(Packet *pkt) : MercRPC(pkt) {
      succ = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      succ.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += succ.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_JOINEND"; }
};

extern MsgType MERCRPC_APPLICATION_JOINEND_RESULT;
struct MercRPC_Application_JoinEnd_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_JoinEnd_Result);

   MercRPC_Application_JoinEnd_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_JoinEnd_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_JOINEND_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_LEAVEBEGIN;
struct MercRPC_Application_LeaveBegin : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_LeaveBegin);

   MercRPC_Application_LeaveBegin() : MercRPC((uint32)0) {}
   MercRPC_Application_LeaveBegin(Packet *pkt) : MercRPC(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_LEAVEBEGIN"; }
};

extern MsgType MERCRPC_APPLICATION_LEAVEBEGIN_RESULT;
struct MercRPC_Application_LeaveBegin_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_LeaveBegin_Result);

   MercRPC_Application_LeaveBegin_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_LeaveBegin_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_LEAVEBEGIN_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_LEAVEEND;
struct MercRPC_Application_LeaveEnd : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_LeaveEnd);

   MercRPC_Application_LeaveEnd() : MercRPC((uint32)0) {}
   MercRPC_Application_LeaveEnd(Packet *pkt) : MercRPC(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_LEAVEEND"; }
};

extern MsgType MERCRPC_APPLICATION_LEAVEEND_RESULT;
struct MercRPC_Application_LeaveEnd_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_LeaveEnd_Result);

   MercRPC_Application_LeaveEnd_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_LeaveEnd_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_LEAVEEND_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_RANGEEXPANDED;
struct MercRPC_Application_RangeExpanded : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_RangeExpanded);
   NodeRange oldrange;
   NodeRange newrange;

   MercRPC_Application_RangeExpanded(const NodeRange& oldrange, const NodeRange& newrange) : oldrange(oldrange), newrange(newrange), MercRPC((uint32)0) {}
   MercRPC_Application_RangeExpanded(Packet *pkt) : MercRPC(pkt) {
      oldrange = NodeRange(pkt);
      newrange = NodeRange(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      oldrange.Serialize(pkt);
      newrange.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += oldrange.GetLength();
      len += newrange.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_RANGEEXPANDED"; }
};

extern MsgType MERCRPC_APPLICATION_RANGEEXPANDED_RESULT;
struct MercRPC_Application_RangeExpanded_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_RangeExpanded_Result);

   MercRPC_Application_RangeExpanded_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_RangeExpanded_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_RANGEEXPANDED_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_RANGECONTRACTED;
struct MercRPC_Application_RangeContracted : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_RangeContracted);
   NodeRange oldrange;
   NodeRange newrange;

   MercRPC_Application_RangeContracted(const NodeRange& oldrange, const NodeRange& newrange) : oldrange(oldrange), newrange(newrange), MercRPC((uint32)0) {}
   MercRPC_Application_RangeContracted(Packet *pkt) : MercRPC(pkt) {
      oldrange = NodeRange(pkt);
      newrange = NodeRange(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      oldrange.Serialize(pkt);
      newrange.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += oldrange.GetLength();
      len += newrange.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_RANGECONTRACTED"; }
};

extern MsgType MERCRPC_APPLICATION_RANGECONTRACTED_RESULT;
struct MercRPC_Application_RangeContracted_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_RangeContracted_Result);

   MercRPC_Application_RangeContracted_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_RangeContracted_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_RANGECONTRACTED_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTROUTE;
struct MercRPC_Application_EventRoute : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_EventRoute);
   Event *ev;
   IPEndPoint lastHop;

   MercRPC_Application_EventRoute(Event * ev, const IPEndPoint& lastHop) : ev(ev->Clone()), lastHop(lastHop), MercRPC((uint32)0) {}
   MercRPC_Application_EventRoute(Packet *pkt) : MercRPC(pkt) {
      ev = CreateObject<Event>(pkt);
      lastHop = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      ev->Serialize(pkt);
      lastHop.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += ev->GetLength();
      len += lastHop.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTROUTE"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTROUTE_RESULT;
struct MercRPC_Application_EventRoute_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_EventRoute_Result);
   int res;

   MercRPC_Application_EventRoute_Result(MercRPC *req, int res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_EventRoute_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (int)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteInt((uint32)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTROUTE_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTLINEAR;
struct MercRPC_Application_EventLinear : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_EventLinear);
   Event *ev;
   IPEndPoint lastHop;

   MercRPC_Application_EventLinear(Event * ev, const IPEndPoint& lastHop) : ev(ev->Clone()), lastHop(lastHop), MercRPC((uint32)0) {}
   MercRPC_Application_EventLinear(Packet *pkt) : MercRPC(pkt) {
      ev = CreateObject<Event>(pkt);
      lastHop = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      ev->Serialize(pkt);
      lastHop.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += ev->GetLength();
      len += lastHop.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTLINEAR"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTLINEAR_RESULT;
struct MercRPC_Application_EventLinear_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_EventLinear_Result);
   int res;

   MercRPC_Application_EventLinear_Result(MercRPC *req, int res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_EventLinear_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (int)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteInt((uint32)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTLINEAR_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTATRENDEZVOUS;
struct MercRPC_Application_EventAtRendezvous : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_EventAtRendezvous);
   Event *ev;
   IPEndPoint lastHop;
   int nhops;

   MercRPC_Application_EventAtRendezvous(Event * ev, const IPEndPoint& lastHop, int nhops) : ev(ev->Clone()), lastHop(lastHop), nhops(nhops), MercRPC((uint32)0) {}
   MercRPC_Application_EventAtRendezvous(Packet *pkt) : MercRPC(pkt) {
      ev = CreateObject<Event>(pkt);
      lastHop = IPEndPoint(pkt);
      nhops = (int)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      ev->Serialize(pkt);
      lastHop.Serialize(pkt);
      pkt->WriteInt((uint32)nhops);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += ev->GetLength();
      len += lastHop.GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTATRENDEZVOUS"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTATRENDEZVOUS_RESULT;
struct MercRPC_Application_EventAtRendezvous_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_EventAtRendezvous_Result);
   EventProcessType res;

   MercRPC_Application_EventAtRendezvous_Result(MercRPC *req, EventProcessType res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_EventAtRendezvous_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (EventProcessType)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteInt((uint32)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTATRENDEZVOUS_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTINTERESTMATCH;
struct MercRPC_Application_EventInterestMatch : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_EventInterestMatch);
   Event *ev;
   Interest *in;
   IPEndPoint subscriber;

   MercRPC_Application_EventInterestMatch(const Event * ev, const Interest * in, const IPEndPoint& subscriber) : ev(ev->Clone()), in(in->Clone()), subscriber(subscriber), MercRPC((uint32)0) {}
   MercRPC_Application_EventInterestMatch(Packet *pkt) : MercRPC(pkt) {
      ev = CreateObject<Event>(pkt);
      in = CreateObject<Interest>(pkt);
      subscriber = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      ev->Serialize(pkt);
      in->Serialize(pkt);
      subscriber.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += ev->GetLength();
      len += in->GetLength();
      len += subscriber.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTINTERESTMATCH"; }
};

extern MsgType MERCRPC_APPLICATION_EVENTINTERESTMATCH_RESULT;
struct MercRPC_Application_EventInterestMatch_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_EventInterestMatch_Result);

   MercRPC_Application_EventInterestMatch_Result(MercRPC *req) : MercRPCResult(req) {}
   MercRPC_Application_EventInterestMatch_Result(Packet *pkt) : MercRPCResult(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_EVENTINTERESTMATCH_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_INTERESTROUTE;
struct MercRPC_Application_InterestRoute : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_InterestRoute);
   Interest *in;
   IPEndPoint lastHop;

   MercRPC_Application_InterestRoute(Interest * in, const IPEndPoint& lastHop) : in(in->Clone()), lastHop(lastHop), MercRPC((uint32)0) {}
   MercRPC_Application_InterestRoute(Packet *pkt) : MercRPC(pkt) {
      in = CreateObject<Interest>(pkt);
      lastHop = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      in->Serialize(pkt);
      lastHop.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += in->GetLength();
      len += lastHop.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_INTERESTROUTE"; }
};

extern MsgType MERCRPC_APPLICATION_INTERESTROUTE_RESULT;
struct MercRPC_Application_InterestRoute_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_InterestRoute_Result);
   int res;

   MercRPC_Application_InterestRoute_Result(MercRPC *req, int res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_InterestRoute_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (int)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteInt((uint32)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_INTERESTROUTE_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_INTERESTLINEAR;
struct MercRPC_Application_InterestLinear : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_InterestLinear);
   Interest *in;
   IPEndPoint lastHop;

   MercRPC_Application_InterestLinear(Interest * in, const IPEndPoint& lastHop) : in(in->Clone()), lastHop(lastHop), MercRPC((uint32)0) {}
   MercRPC_Application_InterestLinear(Packet *pkt) : MercRPC(pkt) {
      in = CreateObject<Interest>(pkt);
      lastHop = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      in->Serialize(pkt);
      lastHop.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += in->GetLength();
      len += lastHop.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_INTERESTLINEAR"; }
};

extern MsgType MERCRPC_APPLICATION_INTERESTLINEAR_RESULT;
struct MercRPC_Application_InterestLinear_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_InterestLinear_Result);
   int res;

   MercRPC_Application_InterestLinear_Result(MercRPC *req, int res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_InterestLinear_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (int)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteInt((uint32)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_INTERESTLINEAR_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_INTERESTATRENDEZVOUS;
struct MercRPC_Application_InterestAtRendezvous : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_InterestAtRendezvous);
   Interest *in;
   IPEndPoint lastHop;

   MercRPC_Application_InterestAtRendezvous(Interest * in, const IPEndPoint& lastHop) : in(in->Clone()), lastHop(lastHop), MercRPC((uint32)0) {}
   MercRPC_Application_InterestAtRendezvous(Packet *pkt) : MercRPC(pkt) {
      in = CreateObject<Interest>(pkt);
      lastHop = IPEndPoint(pkt);
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
      in->Serialize(pkt);
      lastHop.Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      len += in->GetLength();
      len += lastHop.GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_INTERESTATRENDEZVOUS"; }
};

extern MsgType MERCRPC_APPLICATION_INTERESTATRENDEZVOUS_RESULT;
struct MercRPC_Application_InterestAtRendezvous_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_InterestAtRendezvous_Result);
   InterestProcessType res;

   MercRPC_Application_InterestAtRendezvous_Result(MercRPC *req, InterestProcessType res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_InterestAtRendezvous_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (InterestProcessType)pkt->ReadInt();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteInt((uint32)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 4;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_INTERESTATRENDEZVOUS_RESULT"; }
};

extern MsgType MERCRPC_APPLICATION_ISLEAVEJOINOK;
struct MercRPC_Application_IsLeaveJoinOK : public MercRPC {
   DECLARE_TYPE(Message, MercRPC_Application_IsLeaveJoinOK);

   MercRPC_Application_IsLeaveJoinOK() : MercRPC((uint32)0) {}
   MercRPC_Application_IsLeaveJoinOK(Packet *pkt) : MercRPC(pkt) {
   }
   void Serialize(Packet *pkt) {
      MercRPC::Serialize(pkt);
   }
   uint32 GetLength() {
      int len = MercRPC::GetLength();
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_ISLEAVEJOINOK"; }
};

extern MsgType MERCRPC_APPLICATION_ISLEAVEJOINOK_RESULT;
struct MercRPC_Application_IsLeaveJoinOK_Result : public MercRPCResult {
   DECLARE_TYPE(Message, MercRPC_Application_IsLeaveJoinOK_Result);
   bool res;

   MercRPC_Application_IsLeaveJoinOK_Result(MercRPC *req, bool res) : res(res), MercRPCResult(req) {}
   MercRPC_Application_IsLeaveJoinOK_Result(Packet *pkt) : MercRPCResult(pkt) {
      res = (bool)pkt->ReadByte();
   }
   void Serialize(Packet *pkt) {
      MercRPCResult::Serialize(pkt);
      pkt->WriteByte((byte)res);
   }
   uint32 GetLength() {
      int len = MercRPCResult::GetLength();
      len += 1;
      return len;
   }
   const char* TypeString() { return "MERCRPC_APPLICATION_ISLEAVEJOINOK_RESULT"; }
};

class Application_ServerStub : public virtual MercRPCServerStub {
  private:
    Application *iface;
  public:
    Application_ServerStub(Application *iface) : iface(iface) {}
    bool Dispatch(MercRPC *rpc, MercRPCMarshaller *m);
};

class MercRPCMarshaller;

class Application_ClientStub : public Application {
  private:
    static Application_ClientStub *m_Instance;
    MercRPCMarshaller *m;
  protected:
    Application_ClientStub(MercRPCMarshaller *m) : m(m) {}
  public:
    static Application_ClientStub *GetInstance();
    virtual ~Application_ClientStub() {}
    PubsubStore* GetPubsubStore (int hubId);
    void JoinBegin (const IPEndPoint& succ);
    void JoinEnd (const IPEndPoint& succ);
    void LeaveBegin ();
    void LeaveEnd ();
    void RangeExpanded (const NodeRange& oldrange, const NodeRange& newrange);
    void RangeContracted (const NodeRange& oldrange, const NodeRange& newrange);
    int EventRoute (Event * ev, const IPEndPoint& lastHop);
    int EventLinear (Event * ev, const IPEndPoint& lastHop);
    EventProcessType EventAtRendezvous (Event * ev, const IPEndPoint& lastHop, int nhops);
    void EventInterestMatch (const Event * ev, const Interest * in, const IPEndPoint& subscriber);
    int InterestRoute (Interest * in, const IPEndPoint& lastHop);
    int InterestLinear (Interest * in, const IPEndPoint& lastHop);
    InterestProcessType InterestAtRendezvous (Interest * in, const IPEndPoint& lastHop);
    bool IsLeaveJoinOK ();
};

#endif /* APPLICATION_MERCRPC */
