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
  TimeVal.cpp

begin           : Oct 16, 2003
version         : $Id: TimeVal.cpp 2477 2005-11-13 07:34:38Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#include <cstdlib>
#include <cstdio>
#include <util/TimeVal.h>
#include <mercury/Packet.h>
#include <util/debug.h>

    static bool g_Inited = false;
//TimeVal g_StartTime;
float g_Slowdown;
unsigned int g_CPUMHz;

// xxx should rename this maybe?
void InitCPUMHz ()
{
    // should only do this once so StartTime remains fixed
    if (g_Inited) return;
    g_Inited = true;

    FILE *fp = popen ("grep '^cpu MHz' /proc/cpuinfo | awk '{ print $4}'", "r");
    if (!fp)
	Debug::die ("can't open /proc/cpuinfo pipe");

    char buf[80];
    memset (buf, sizeof (buf), 0);
    char *ret;
    while ((ret = fgets (buf, sizeof (buf), fp)) == NULL) {
	if (errno == EINTR || errno == EAGAIN)
	    continue;
	else {
	    WARN << "couldn't read anything from /proc/cpuinfo pipe: " <<
		strerror(errno) << endl;
	    ASSERT(0);
	}
    }

    pclose (fp);
    ASSERT (buf[0] != '\0');

    float mhz;
    sscanf (buf, "%f", &mhz);
    g_CPUMHz = (unsigned int) mhz;
    INFO << "read cpu mhz as " << mhz << " setting CPU MHz as " << g_CPUMHz << endl;

    //gettimeofday (&g_StartTime, NULL);
}

TimeVal TIME_NONE = {0,0};

bool operator<(struct timeval a, struct timeval b) {
    return a.tv_sec < b.tv_sec ||
	a.tv_sec == b.tv_sec && a.tv_usec < b.tv_usec;
}

bool operator>(struct timeval a, struct timeval b) {
    return a.tv_sec > b.tv_sec ||
	a.tv_sec == b.tv_sec && a.tv_usec > b.tv_usec;
}

bool operator==(struct timeval a, struct timeval b) {
    return a.tv_sec == b.tv_sec && a.tv_usec == b.tv_usec;
}

bool operator<=(struct timeval a, struct timeval b) {
    return a < b || a == b;
}

bool operator>=(struct timeval a, struct timeval b) {
    return a > b || a == b;
}

bool operator!=(struct timeval a, struct timeval b) {
    return !(a == b);
}

struct timeval operator+(struct timeval a, double add_msec) {
    struct timeval ret;

    // convert into sec/usec parts
    sint32 sec_part  = (sint32)(add_msec/MSEC_IN_SEC);
    sint32 usec_part = (sint32)((add_msec - sec_part * MSEC_IN_SEC)*USEC_IN_MSEC);

    // do the initial addition
    ret.tv_sec  = a.tv_sec + sec_part;
    ret.tv_usec = a.tv_usec + usec_part;

    // perform a carry if necessary
    if (ret.tv_usec > USEC_IN_SEC) {
	ret.tv_sec++;
	ret.tv_usec = ret.tv_usec % USEC_IN_SEC;
    } else if (ret.tv_usec < 0) {
	ret.tv_sec--;
	ret.tv_usec = USEC_IN_SEC + ret.tv_usec;
    }

    // XXX When someone has free time, implement and test a more
    // efficient implementation with some bit twiddling. :P

    /*
      if (add_msec >= 0) {
      ret.tv_sec = a.tv_sec + add_msec/MSEC_IN_SEC;
      ret.tv_usec = a.tv_usec + (add_msec % MSEC_IN_SEC)*USEC_IN_MSEC;
      if (ret.tv_usec > USEC_IN_SEC) {
      ret.tv_sec++;
      ret.tv_usec = ret.tv_usec % USEC_IN_SEC;
      }
      } else {
      ret.tv_sec = a.tv_sec + add_msec/MSEC_IN_SEC;
      sint32 sub = ((-add_msec) % MSEC_IN_SEC)*USEC_IN_MSEC;
      if (a.tv_usec < sub) {
      ret.tv_sec--;
      ret.tv_usec = USEC_IN_SEC - (sub - a.tv_usec);
      } else {
      ret.tv_usec = a.tv_usec - sub;
      }
      }
    */

    return ret;
}

sint64 operator-(struct timeval a, struct timeval b) {
    return ((sint64)a.tv_sec - (sint64)b.tv_sec)*MSEC_IN_SEC + 
	((sint64)a.tv_usec - (sint64)b.tv_usec)/USEC_IN_MSEC;
}

ostream& operator<<(ostream& out, const struct timeval& a)
{
    char buf[32];
    sprintf(buf, "%.3f", (float)a.tv_sec + ((float)a.tv_usec/USEC_IN_SEC));
    return (out << buf);
}

float timeval_to_float (struct timeval a)
{
    return (float) a.tv_sec + ((float) a.tv_usec / USEC_IN_SEC);
}

void Serialize(const TimeVal& v, Packet *pkt) {
    pkt->WriteInt(v.tv_sec);
    pkt->WriteInt(v.tv_usec);
}

void Deserialize(TimeVal& v, Packet *pkt) {
    v.tv_sec = pkt->ReadInt();
    v.tv_usec = pkt->ReadInt();
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
