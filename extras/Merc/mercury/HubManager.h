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

#ifndef __HUBMANAGER__H
#define __HUBMANAGER__H

#include <mercury/Timer.h>
#include <mercury/IPEndPoint.h>
#include <fstream>

struct MsgBootstrapResponse;
class HubManager;
struct IPEndPoint;

#include <mercury/Hub.h>
typedef vector<Hub *> HubVec;
typedef HubVec::iterator HubVecIter;

struct MsgPublication;
struct MsgSubscription;
class BufferManager;
class MercuryNode;
class Scheduler;

class HubManager : public MessageHandler
{
    friend class BootstrapRequestTimer;
    friend class BootstrapHeartbeatTimer;
    friend ostream& operator<<(ostream& out, HubManager *hm);

    HubVec         m_HubVec;
    ref<Timer>     m_BootstrapRequestTimer, m_BootstrapHeartbeatTimer;

    MercuryNode   *m_MercuryNode;

    // convenience;
    NetworkLayer  *m_Network;
    Scheduler     *m_Scheduler;
    IPEndPoint     m_Address;

    BufferManager *m_BufferManager;
    IPEndPoint     m_BootstrapIP;

 public:
    HubManager(MercuryNode *mnode, BufferManager *bufferManager);
    virtual ~HubManager() {}

    void PerformBootstrap();
    void BootstrapUsingServer(char *bootstrap);
    void ProcessMessage(IPEndPoint *from, Message *msg);

    int GetNumHubs() { return m_HubVec.size(); }
    Hub *GetHubByIndex(int i) { return m_HubVec[i]; }
    Hub *GetHubByID(byte id);

    IPEndPoint& GetBootstrapIP () { return m_BootstrapIP; }
    IPEndPoint& GetAddress () { return m_Address; }

    bool IsJoined();
    void Shutdown();

    void SendAppPublication(MsgPublication *pmsg);
    void SendAppSubscription(MsgSubscription *smsg);

    void PrintSubscriptionList(ostream& stream);
    void PrintPublicationList(ostream& stream);

    void Print(FILE *stream);
 private:
    void HandleBootstrapResponse(IPEndPoint * from, MsgBootstrapResponse * bmsg);
    void RegisterHubInfo(vector<HubInitInfo *>& v);
    void StartJoin();
};

ostream& operator<<(ostream& out, Hub *hub);
ostream& operator<<(ostream& out, HubManager *hm);

#endif // __HUBMANAGER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
