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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

/***************************************************************************
  FloodRouter.h 

begin           : Sat Nov 2 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/


#ifndef __FLOODROUTER__H
#define __FLOODROUTER__H

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <assert.h>
#include <list>


class Connection;
class Peer;

#include <mercury/Router.h>
#include <mercury/BufferManager.h>
#include <mercury/Message.h>
#include <mercury/RealNet.h>

extern IPEndPoint g_FloodIP;

class FloodRouter : public Router {
 private:
    pthread_t              m_Thread;
    BufferManager         *m_BufferManager;
    NetworkLayer          *m_Network;

    static bool            m_bCreated;
    static MercuryNode*  m_Instance;

    vector<Peer *>         m_Peers;

    // bootstrap server
    IPEndPoint            *m_BootstrapNode;

    // functions in here called whenever we match a pub or sub
    Handler*               m_MatchHandler;

 public:
    static FloodRouter*  GetInstance(unsigned short port, Handler* match_handler);
    static FloodRouter*  GetInstance();

    static void mainthread_signal_handler(int signal);

    ~FloodRouter();

    void    FireUp();
    void    ShutDown();
    uint32  GetIP() { return g_FloodIP.GetIP(); }
    uint16  GetPort() { return g_FloodIP.GetPort(); }

    void    SendEvent(Event *pub);
    void    RegisterInterest( Interest *sub );
    Event*  ReadEvent();

    void    LockBuffer() { assert(0); }
    void    UnlockBuffer() { assert(0); }

    void    Print(FILE *stream);
    void    StartThread();
 private:
    // bootload of private "helper" methods for the router.
    // might move bunch of this functionality to a separate class.
    FloodRouter( unsigned short port, Handler* match_handler );

    void    SetMyAddress( unsigned short port );
    void    PerformBootstrap();
    void    SendHeartBeat();

    void    ProcessMessage(IPEndPoint *from, Message *msg);
    void    HandleConnectionError(ConnStatusType type, IPEndPoint *from);

    void    HandleMercPub( IPEndPoint *from, MsgPublication *msg );
    void    HandleBootstrapResponse(IPEndPoint *from, MsgBootstrapResponse *bsd);
    void    HandleMercSub( IPEndPoint *from, MsgSubscription *msg );
    void    HandleAck( IPEndPoint *from, MsgAck *amsg);

    void    DeliverPubToSubscribers(MsgPublication *pmsg);
    void    SendApplicationPackets( void );
    void    SendControlPackets( void );
    void    CheckTimerExpirations();

    void    PrintPeerList(FILE *stream);
    void    PrintSubscriptionList(FILE *stream);

    Peer   *_LookupPeer(IPEndPoint *from);
};

#endif // __FLOODROUTER__H

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
