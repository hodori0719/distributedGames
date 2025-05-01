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

#ifndef __APPLICATION__H
#define __APPLICATION__H   

#include <map>
#include <list>
//#include <om/common.h>
#include <mercury/Event.h>
#include <mercury/Neighbor.h>
#include <mercury/Interest.h>
#include <mercury/Message.h>       // I hate including this here; but we need MsgPublication. Sucks. - Ashwin
#include <util/callback.h>

class PubsubStore {
 public:
    virtual void StoreTrigger (MsgPublication *pmsg) = 0;
    virtual void StoreSub (Interest *in) = 0;

    // somebody can come up with better names for these.
    // basically if the predicate returns false always,
    // it becomes a way of iterating through the list.

    virtual void DeleteTriggers (callback<bool, MsgPublication *>::ref delpred) = 0;
    virtual void DeleteSubs (callback<bool, Interest *>::ref delpred) = 0;

    virtual void GetOverlapSubs (MsgPublication *pmsg, list<Interest *> *pmatch) = 0;
    virtual void GetOverlapTriggers (Interest *in, list<MsgPublication *> *pmatch) = 0;

    virtual void Clear () = 0;
};


typedef enum {
    EV_NUKE,
    EV_MATCH,
    EV_STORE,
    EV_MATCH_AND_STORE
} EventProcessType;

typedef enum {
    IN_NUKE,
    IN_STORE,
    IN_TRIGGER,
    IN_STORE_AND_TRIGGER
} InterestProcessType;

/**
 * This is the interface that the application implements to handle callbacks
 * from merc.
 */
class Application {
 public:
    /**
     * Returns the pubsub storage container for a hub with this
     * hubId. If NULL is returned, Mercury uses its default pubsub
     * storage (linear-time matching).
     **/
    virtual PubsubStore* GetPubsubStore (int hubId) = 0;
    /**
     * Called when the join process begins. At this point, only 
     * the IP address of the successor is known. Any other 
     * needed information can be queried via the MercuryNode 
     * interface.
     **/
    virtual void JoinBegin (const IPEndPoint& succ) = 0;

    /**
     * Called when a join-reponse is received. At this point, 
     * the node has a successor (but may not yet have a 
     * predecessor). However, the node is ready to route pubs/subs
     * in the system. Any other needed information (ranges, etc.)
     * can be queried via the MercuryNode interface.
     **/
    virtual void JoinEnd (const IPEndPoint& succ) = 0;

    /**
     * Called when Mercury decides to leave the ring for 
     * load balancing purposes. No extra parameters are provided.
     * If the application so desires, it can query the details from 
     * Mercury within the LeaveBegin () call. 
     **/
    virtual void LeaveBegin () = 0;

    /**
     * Called when Mercury has left the ring. This happens almost 
     * immediately after LeaveBegin (). The application should do 
     * most state transfer if needed within LeaveBegin (). 
     **/
    virtual void LeaveEnd () = 0;

    /**
     * Called when Mercury does 'local' range adjustment for 
     * balancing load locally (with predecessor and successor).
     **/
    virtual void RangeExpanded (const NodeRange& oldrange, const NodeRange& newrange) = 0;
    virtual void RangeContracted (const NodeRange& oldrange, const NodeRange& newrange) = 0;

    // can we get a Neighbor structor (MercID+IPEndPoint) instead of
    // just the neighbor address? Would the MercID be useful?

    /** State diagram for processing:
     *
     *   Receive -> (AtRendezvous -> Match(es) -> StoreTrigger)
     *      -> Route OR Linear
     *
     **/

    /**
     * Called during normal routing of the event. The Event is routed
     * further if the application returns > 0.
     **/
    virtual int EventRoute(Event *ev, const IPEndPoint& lastHop) = 0;

    /**
     * Called when a range-event @ev is being routed in a linear mode. 
     * The event can be in a "fanned" out mode also, so it is incorrect
     * to assume that @lasthop is the previous node on the ring. 
     *
     * The event is routed further if the application returns > 0.
     **/
    virtual int EventLinear(Event *ev, const IPEndPoint& lastHop) = 0;

    /**
     * Called when the event arrives at the rendezvous point. Depending
     * on the return value, further processing is performed for the
     * event.  See the definition of the EventProcessType enum. @nhops
     * denotes the number of app-level hops it took for the event to 
     * arrive.
     **/
    virtual EventProcessType EventAtRendezvous (Event *ev, const IPEndPoint& lastHop, int nhops) = 0;

    /**
     * Called when an event is matched against an interest, and is 
     * going to be sent off to the subscribers.
     **/
    virtual void EventInterestMatch (const Event *ev, const Interest *in, const IPEndPoint& subscriber) = 0;

    /**
     * Called during normal routing of the Interest. The interest is 
     * routed further if the application returns > 0.
     **/
    virtual int InterestRoute (Interest *in, const IPEndPoint& lastHop) = 0;

    /**
     * Called when an interest @in is being routed in linear mode.
     * The interest can be in a "fanned out" mode, so it is
     * incorrect to assume that @lasthop is the previous node on the ring.
     *
     * The interest is routed further is application returns > 0
     **/
    virtual int InterestLinear (Interest *in, const IPEndPoint& lastHop) = 0;

    /**
     * Called when the interest arrives at the rendezvous
     * point. Depending on the return value, further processing is
     * performed for the interest.  See the definition of the
     * InterestProcessType enum.
     **/
    virtual InterestProcessType InterestAtRendezvous (Interest *in, const IPEndPoint& lastHop) = 0;

    /**
     * Called (may be called very often) to check if the node
     * (Mercury layer) can perform a leave-join for load-balancing
     **/
    virtual bool IsLeaveJoinOK () = 0;
};

class DummyApp : public Application {
 public:
    virtual PubsubStore* GetPubsubStore (int hubId) { return NULL; }

    virtual void JoinBegin (const IPEndPoint& succ) {}
    virtual void JoinEnd (const IPEndPoint& succ) {}
    virtual void LeaveBegin () {}
    virtual void LeaveEnd () {}
    virtual void RangeExpanded (const NodeRange& oldrange, const NodeRange& newrange) {}
    virtual void RangeContracted (const NodeRange& oldrange, const NodeRange& newrange) {}

    virtual int EventRoute(Event *ev, const IPEndPoint& lastHop) { return 1; }
    virtual int EventLinear(Event *ev, const IPEndPoint& lastHop) { return 1; }
    virtual EventProcessType EventAtRendezvous (Event *ev, const IPEndPoint& lastHop, int nhops) { return EV_MATCH_AND_STORE; }  

    virtual void EventInterestMatch (const Event *ev, const Interest *in, const IPEndPoint& subscriber) {}

    virtual int InterestRoute (Interest *in, const IPEndPoint& lastHop) { return 1; }
    virtual int InterestLinear (Interest *in, const IPEndPoint& lastHop) { return 1; }
    virtual InterestProcessType InterestAtRendezvous (Interest *in, const IPEndPoint& lastHop) { return IN_STORE_AND_TRIGGER; }
    virtual bool IsLeaveJoinOK () { return true; }
};

#endif /* __APPLICATION__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
