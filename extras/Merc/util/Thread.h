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
 * Thread.h
 *
 * Initial Author: Jeff Pang <jeffpang+441@cs.cmu.edu>
 * Class: 15-441
 * Version: $Id: Thread.h 2382 2005-11-03 22:54:59Z ashu $
 *
 */

#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

/**
 * A Thread class similar to Java threads semantics, implemented ontop of
 * pthreads. See Mutex for locks.
 *
 * To create a thread, subclass Thread and implement the Run() method.
 * After construction of the Thread subclass, calling Start() begins
 * the thread.
 */
class Thread {

 private:
    pthread_t pthread;
    bool _started, _finished;
 public:

    // HACK: don't use this externally
    void _Run();

    /**
     * Cause the current thread to yeild processing.
     */
    static int Yield();

    Thread();
    virtual ~Thread();

    /**
     * Begin the new thread.
     */
    int Start();

    /**
     * True if the new thread has started.
     */
    bool Started();

    /**
     * True if the new thread has finished (Run() returned).
     */
    bool Finished();

    /**
     * Join the current thread with this thread.
     */
    int Join();

    /**
     * Interrupt this thread. This is implemented by routing SIGUSR2 to
     * the underlying pthread. The signal handler doesn't do anything,
     * but any blocking system calls in this thread will return EINTR.
     */
    int Interrupt();

    /**
     * Subclasses implement this method. This is the entry point in the
     * thread.
     */
    virtual void Run() = 0;

};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
