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
  Utils.h

begin           : Oct 16, 2003
version         : $Id: Utils.h 2382 2005-11-03 22:54:59Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#ifndef __UTIL_UTILS_H__
#define __UTIL_UTILS_H__

#include <algorithm>
#include <vector>
    //#include <map>
    //#include <set>
#include <mercury/AutoSerializer.h>
#include <mercury/Packet.h>
#include <mercury/common.h>
    //#include "Thread.h"

    // this is a slightly different version than in om/Manager.h 
    // it removes the FREQUENT_MAINTENANCE hack

#define PERIODIC2(time_msec, now, blurb)   \
do {  \
    static TimeVal last = TIME_NONE; \
	if ((last + (time_msec)) <= (now)) { \
	    blurb ; last = (now); \
	} \
} while (0)

#define FREQ(num, blurb)   \
do {  \
    static int blah = 0; \
	blah++; \
	if (blah % num == 1) { blurb; } \
} while (0)

///////////////////////////////////////////////////////////////////////////////
//
// Thread-safe STL containers
//

/* XXX One day fix this so we can use it :P
   template<class K, class V, class C = less<K> >
   class tsafe_map : public map<K, V, C>
   {
   private:
   Mutex lock;

   #define DECLARE(decl, funccall) void decl { \
   lock.Acquire(); \
   map<K,V,C>::funcall; \
   lock.Release(); \
   }

   #define DECLARE_RET(ret, decl, funccall) ret decl { \
   lock.Acquire(); \
   ret _tmp = map<K,V,C>::funcall; \
   lock.Release(); \
   return _tmp; \
   }

   public:
   DECLARE_RET(bool, empty() const, empty());
   DECLARE_RET(iterator<K,V,C>, insert(iterator<K,V,C> pos, const V& x), 
   insert(pos,x));
   DECLARE(erase(iterator<K,V,C> pos), erase(pos));
   DECLARE(erase(iterator<K,V,C> first, iterator<K,V,C> last), 
   erase(first, last));
   DECLARE(clear(), clear());
   DECLARE_RET(iterator<K,V,C>, find(const K& k), find(k));

   // TODO: skipped some functions here...

   V& operator[](const K& k) {
   // how to call super::operator[]?
   iterator<K,V,C> _tmp = find(k);
   return _tmp->second;
   }
   };
*/

///////////////////////////////////////////////////////////////////////////////

/**
 * Permute the elements in a vector.
 */
template <class T>
void permute(vector<T>& vec)
{
    random_shuffle(vec.begin(), vec.end());
}

/**
 * Copy (clone) the object through serialization.
 */
template<class T>
T *scopy(T *obj)
{
    Packet p(obj->GetLength());
    obj->Serialize(&p);
    p.ResetBufPosition();
    return new T(&p);
}

/**
 * Copy (clone) the auto-serialized object through serialization
 *
 * B should be the appropriate base-class that is set in the autoserializer.
 */
template<class B>
B *scopy_auto(B *obj)
{
    Packet p(obj->GetLength());
    obj->Serialize(&p);
    p.ResetBufPosition();
    return CreateObject<B>(&p);
}

/**
 * Generate a random uint32 value. Will be != 0.
 */
inline uint32 CreateNonce() 
{
    uint32 nonce;

    do {
	nonce = (uint32)(drand48()*0xFFFFFFFFUL);
    } while (nonce == 0);

    return nonce;
}

template <class T>
inline T ScaleTime (T var)
{
#if 0
    if (g_Slowdown > 1.0f)
	return (T) (var * g_Slowdown);
    else
#endif
	return var;
}

void DumpBuffer (byte *buf, int len);

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
