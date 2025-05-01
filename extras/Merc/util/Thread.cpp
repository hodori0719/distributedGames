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
 * Thread.cpp
 *
 * Initial Author: Jeff Pang <jeffpang+441@cs.cmu.edu>
 * Class: 15-441
 * Version: $Id: Thread.cpp 2382 2005-11-03 22:54:59Z ashu $
 *
 */

#include <time.h>
#include <signal.h>
#include <util/debug.h>
#include <util/Thread.h>

static void *pthread_func(void *args) {
    Thread *thread = (Thread *)args;
    thread->_Run();
    return NULL;
}

static void usr2_ignore(int sig) {
    // ignore the signal. we just want to interrupt the system call
    // so that it returns EINTR if we're blocking...
}

int Thread::Yield()
{
    // XXX FIXME: won't work in windows!

#if 0
    // It seems that this does not function correctly with the linux
    // scheduler... newsgroups and articles indicate that the yielding
    // thread is just put at the front of the thread queue again!
    return sched_yield();
#else
    // This is a hacky alternative
    static struct timespec zero = { 0 , 0 };
    return nanosleep(&zero, NULL);
#endif

}

void Thread::_Run() 
{
    signal(SIGUSR2, usr2_ignore);
    _started = 1;
    Run();
    _finished = 1;
}

Thread::Thread() :
    _started(0), _finished(0)
{
    bzero(&pthread, sizeof(pthread_t));
}

Thread::~Thread() {
}

int Thread::Start() {
    return pthread_create(&pthread, NULL, pthread_func, this);
}

bool Thread::Started() {
    return _started;
}

bool Thread::Finished() {
    return _finished;
}

int Thread::Join() {
    void *rs;
    return pthread_join(pthread, &rs);
}

int Thread::Interrupt() {
    if (Started())
	return pthread_kill(pthread, SIGUSR2);
    else
	return -1;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
