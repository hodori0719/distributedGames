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

/**************************************************************************
  TimeVal.h

begin           : Oct 16, 2003
version         : $Id: TimeVal.h 2476 2005-11-13 06:31:08Z jeffpang $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#ifndef __TIME_VAL_H__
#define __TIME_VAL_H__

#include <iostream>
#ifndef _WIN32
#include <sys/time.h>
#include <time.h>
#else
    // Guess, this will include the require timeval structures etc... - Ashwin
#include <WinSock2.h>
#endif

#include "types.h"

    using namespace std;

typedef struct timeval TimeVal;

#define MSEC_IN_SEC 1000
#define USEC_IN_SEC 1000000
#define USEC_IN_MSEC 1000
bool operator<(struct timeval a, struct timeval b);
bool operator<=(struct timeval a, struct timeval b);
bool operator>(struct timeval a, struct timeval b);
bool operator>=(struct timeval a, struct timeval b);
bool operator==(struct timeval a, struct timeval b);
bool operator!=(struct timeval a, struct timeval b);
struct timeval operator+(struct timeval a, double add_msec);
sint64 operator-(struct timeval a, struct timeval b); /* msec result */
ostream& operator<<(ostream& out, const struct timeval& a);
float timeval_to_float (struct timeval a);

class Packet;
void Serialize(const TimeVal& v, Packet *pkt);
void Deserialize(TimeVal& v, Packet *pkt);
inline uint32 GetLength(const TimeVal& v) { return 8; }

extern TimeVal TIME_NONE;
//extern TimeVal g_StartTime;
extern float g_Slowdown; 
extern unsigned int g_CPUMHz;

inline void ApplySlowdown(uint64& t) {
    if (g_Slowdown > 1.0f)
	t = (uint64)(t/(double)g_Slowdown);
}

inline void ApplySlowdown(TimeVal& t) {
    uint64 usec = ((uint64)t.tv_sec)*USEC_IN_SEC + t.tv_usec;
    ApplySlowdown(usec);
    t.tv_sec  = (uint32)(usec/USEC_IN_SEC);
    t.tv_usec = (uint32)(usec%USEC_IN_SEC);
}

// xxx: this stuff should be moved to OS.h but that creates a loopy dep!

inline unsigned long long int rdtsc()
{
    unsigned long long int x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}

void InitCPUMHz ();

#define Ticks2Usec(ticks) (((double)ticks)/g_CPUMHz)
#define Ticks2Msec(ticks) (Ticks2Usec(ticks)/USEC_IN_MSEC)
#define Usec2Ticks(usec)  ((uint64)(usec*((uint64)g_CPUMHz)))
#define Msec2Ticks(msec)  ((uint64)(Usec2Ticks(msec)*USEC_IN_MSEC))

// without time dilation
#define REAL_CurrentTimeTicks() (rdtsc())
#define REAL_CurrentTimeUsec()  ((uint64)Ticks2Usec(REAL_CurrentTimeTicks()))

// with time dilation
inline uint64 CurrentTimeTicks() {
    uint64 ret = REAL_CurrentTimeTicks();
    ApplySlowdown(ret);
    return ret;
}

// rdtsc appears to be borked on some planetlab machines?
#ifndef NO_RDTSC
#define CurrentTimeUsec() ((uint64)Ticks2Usec(CurrentTimeTicks()))
#else
inline uint64 CurrentTimeUsec() {
    TimeVal v;
    gettimeofday(&v, NULL);
    ApplySlowdown(v);
    return ((uint64)v.tv_sec*USEC_IN_SEC)+v.tv_usec;
}
#endif

//////////////////////////////////////////////////////////////////////////////

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
