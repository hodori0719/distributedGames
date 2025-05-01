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

/***************************************************************************
  Utils.cpp

begin           : Nov 6, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/

#include <mercury/common.h>
#include <sys/time.h>
#include <mercury/Utils.h>

#if 0
    float G_GenerateHarmonic(int n)
{
    float u = G_GetRandom();
    return pow(n, u - 1);
}

#endif

//
// returns a - b in ms
//
uint32 G_DiffMillis(TimeVal a, TimeVal b)
{
    return (a.tv_sec - b.tv_sec) * 1000 + (a.tv_usec - b.tv_usec) / 1000;
}

TimeVal G_AddMillis(TimeVal& a, int ms)
{
    TimeVal b = a;
    b.tv_sec += ms / 1000;   // seconds;

    int usec = (ms % 1000) * 1000; // remaining msecs converted to usecs
    usec += b.tv_usec;

    if (usec >= 1000 * 1000) {
	b.tv_sec++;
	b.tv_usec = usec - 1000 * 1000;
    }
    else {
	b.tv_usec = usec;
    }

    return b;
}

#if 0
uint32 operator-(TimeVal& a, TimeVal& b)
{
    return G_DiffMillis(a, b);
}

TimeVal operator+(TimeVal& a, int ms)
{
    TimeVal b;

    ms *= 1000;   // -> usecs
    b = a;
    b.tv_sec += add;
    return b;
}
#endif

void G_PrintTime(TimeVal a, FILE * stream)
{
    fprintf(stream, "s:%ld;u:%ld\n", a.tv_sec, a.tv_usec);
}

ostream& operator<<(ostream& os, TimeVal& t)
{
    os << merc_va("%d:%.3f ", t.tv_sec, (float) t.tv_usec/ (float) 1000.0);
    return os;
}

float G_GetRandom()
{
    return (float) rand() / (float) (RAND_MAX + 1.0);
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
