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
 * CondVar.h
 *
 * Version: $Id: CondVar.h 2382 2005-11-03 22:54:59Z ashu $
 *
 */

#ifndef __CONDVAR_H__
#define __CONDVAR_H__

#include <pthread.h>
#include <util/debug.h>
#include <util/Mutex.h>

/**
 * Wrapper class for pthread_cond. Has its own lock inside.
 */
class CondVar {

 private:
    Mutex lock;
    pthread_cond_t cond;

 public:

    CondVar() { 
	int ret = pthread_cond_init(&cond, NULL) >= 0;
	ASSERT( ret );
    }

    virtual ~CondVar() {
	pthread_cond_destroy(&cond);
    }

    int Acquire() { return lock.Acquire(); }
    int Release() { return lock.Release(); }

    int Wait() { return pthread_cond_wait(&cond, &lock.mutex); }
    int Signal() { return pthread_cond_signal(&cond); }
    int Broadcast() { return pthread_cond_broadcast(&cond); }
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
