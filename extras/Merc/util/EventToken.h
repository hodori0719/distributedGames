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

/**************************************************************************
  EventHandler.h

begin           : Nov 6, 2002
copyright       : (C) 2002-2005 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2005 Jeffrey Pang       ( jeffpang@cs.cmu.edu )

***************************************************************************/

#ifndef __EVENT_TOKEN_H__
#define __EVENT_TOKEN_H__

#include <util/types.h>
#include <util/Utils.h>
#include <util/CondVar.h>

    typedef void (*EventTokenHandler)(uint32 error_code, void *);

/**
 * A token that can be saved in order to wait for some event to occur.
 * When the event occurs, either someone Signal()s the token. Then the
 * listener(s) can either provide a callback function to be called when
 * the event occurs using InstallHandler(). Or it can just poll the
 * token to see if it is ready. Each handler function will be called
 * exactly once.
 */
class EventToken {
 private:
    CondVar lock;
    bool ready;
    uint32 error;
    list<EventTokenHandler> handlers;
    list<void *> args;
 public:
    EventToken() : ready(false), error(0) {}
    ~EventToken() {}

    /**
     * Indicate that the event this token is waiting for is ready.
     * Should only be called once.
     */
    void Signal(uint32 error_code = 0) {
	lock.Acquire();
	ready = true;
	error = error_code;
	list<EventTokenHandler>::iterator it = handlers.begin();
	list<void *>::iterator it2 = args.begin();
	for ( ; it != handlers.end() && it2 != args.end(); it++, it2++) {
	    EventTokenHandler handler = *it;
	    ASSERT(handler);
	    void *arg = *it2;
	    handler(error, arg);
	}
	handlers.clear();
	args.clear();
	lock.Broadcast();
	lock.Release();
    }

    void Wait() {
	lock.Acquire();
	if (!ready) {
	    lock.Wait();
	}
	lock.Release();
    }

    /**
     * Check if the event that this token was waiting for is ready.
     */
    bool IsReady() {
	bool ret;
	lock.Acquire();
	ret = ready;
	lock.Release();
	return ret;
    }

    /**
     * Check the error code signaled.
     */
    uint32 GetError() {
	uint32 ret;
	lock.Acquire();
	ret = error;
	lock.Release();
	return ret;
    }

    /**
     * Install a handler to be run when the event this token is waiting for
     * is ready.
     */
    void InstallHandler(EventTokenHandler handler, void *arg) {
	ASSERT(handler);
	lock.Acquire();
	// if already ready, run handler now
	if (ready) {
	    handler(error, arg);
	} else {
	    // else save it for later.
	    handlers.push_back(handler);
	    args.push_back(arg);
	}
	lock.Release();
    }

};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
