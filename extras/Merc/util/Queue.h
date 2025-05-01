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
/**
 * Queue.h
 *
 * Version: $Id: Queue.h 2382 2005-11-03 22:54:59Z ashu $
 *
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <list>
#include <pthread.h>
#include <util/types.h>
#include <util/Mutex.h>
#include <util/CondVar.h>

/**
 * A producer/consumer queue
 */
template<class T>
class Queue {
 private:
    CondVar cond;
    list<T> elems;
 public:
    Queue() {}
    virtual ~Queue() {}

    uint32 Size() {
	uint32 ret;
	cond.Acquire();
	ret = elems.size();
	cond.Release();
	return ret;
    }

    void Insert(T& elem) {
	int ret;
	ret = cond.Acquire();
	ASSERT(ret >= 0);
	elems.push_back(elem);
	ret = cond.Signal();
	ASSERT(ret >= 0);
	ret = cond.Release();
	ASSERT(ret >= 0);
    }

    T Remove() {
	int ret;
	ret = cond.Acquire();
	while (elems.size() == 0) {
	    ret = cond.Wait();
	    ASSERT(ret >= 0);
	}
	T elem = elems.front();
	elems.pop_front();
	ret = cond.Release();
	ASSERT(ret >= 0);
	return elem;
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
