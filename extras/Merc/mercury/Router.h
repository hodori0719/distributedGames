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
/* -*- Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

/***************************************************************************
  Router.h

  Interface for a pubsub router. Currently, flood router and mercury router
  are the two implementations supported. In future, perhaps, we might have
  something like Narada + SIENA as well.

begin           : Nov 6, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/


#ifndef __ROUTER__H
#define __ROUTER__H

#include <mercury/common.h>
#include <mercury/Interest.h>
#include <mercury/Event.h>

    typedef enum { FLOOD_ROUTER = 0, MERCURY_ROUTER } router_t;
#define NUM_ROUTER_TYPES 2

static const int MAX_IPPPORT_LEN = 20;

// Abstract class
class Router {
 public:
    Router() {}
    virtual ~Router() {}

    virtual uint32 GetIP() = 0;
    virtual unsigned short GetPort() = 0;

    virtual void SendEvent(Event *) = 0;
    virtual void RegisterInterest(Interest *) = 0;
    //virtual void unRegisterInterest ( Interest * ) = 0;
    virtual Event *ReadEvent() = 0;

    virtual void LockBuffer() = 0;
    virtual void UnlockBuffer() = 0;

    virtual void Print(FILE *stream) = 0;

    virtual void DoWork(u_long timeout) = 0;
};

#endif // __ROUTER__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
