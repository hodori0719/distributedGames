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
  ID.h

begin           : Oct 16, 2003
version         : $Id: ID.h 2706 2006-03-06 21:55:49Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#ifndef __ID_H__
#define __ID_H__

    // Use ID_common.h instead of ID.h if you don't need templated data structures

#include <set>
#include <vector>
#include <map>
#include <list>
#include <hash_map.h>
#include <hash_set.h>
#include <mercury/common.h>
#include <mercury/Packet.h>
#include <mercury/IPEndPoint.h>
#include <mercury/ID_common.h>

    //////////////////////////////////////////////////////////////////////////////

    ostream& operator<<(ostream& out, const GUID& guid);
istream& operator>>(istream& in, GUID& guid);
bool operator==(const GUID& g1, const GUID& g2);
bool operator!=(const GUID& g1, const GUID& g2);

struct less_GUID {
    bool operator() ( const GUID& a, const GUID& b) const {
	return (a.GetIP() < b.GetIP()) ||
	    ((a.GetIP() == b.GetIP()) && (a.GetPort() < b.GetPort())) ||
	    ((a.GetIP() == b.GetIP()) && (a.GetPort() == b.GetPort()) &&
	     (a.GetLocalOID() < b.GetLocalOID()));
    }
};

struct less_GUIDPtr {
    bool operator() ( const GUID *a, const GUID *b) const {
	return (a->GetIP() < b->GetIP()) ||
	    ((a->GetIP() == b->GetIP()) && (a->GetPort() < b->GetPort())) ||
	    ((a->GetIP() == b->GetIP()) && (a->GetPort() == b->GetPort()) &&
	     (a->GetLocalOID() < b->GetLocalOID()));
    }
};

// object UID utilities
typedef set<GUID, less_GUID> GUIDSet;
typedef GUIDSet::iterator GUIDSetIter;
typedef vector<GUID> GUIDVec;
typedef GUIDVec::iterator GUIDVecIter;

struct hash_GUID {
    hash<uint32> H;

    size_t operator() (const GUID a) const {
	return H(a.GetIP() ^ a.GetPort() ^ a.GetLocalOID());
    }
};

struct equal_GUID {
    bool operator() ( const GUID a, const GUID b ) const {
	return a == b;
    }
};

struct hash_GUIDPtr {
    hash<uint32> H;

    size_t operator() (const GUID *a) const {
	return H(a->GetIP() ^ a->GetPort() ^ a->GetLocalOID());
    }
};

struct equal_GUIDPtr {
    bool operator() ( const GUID *a, const GUID *b ) const {
	return *a == *b;
    }
};

typedef set<GUID, less_GUID> GUIDSet;
typedef GUIDSet::iterator GUIDSetIter;
typedef vector<GUID> GUIDVec;
typedef GUIDVec::iterator GUIDVecIter;
typedef list<GUID> GUIDList;
typedef GUIDList::iterator GUIDListIter;

//////////////////////////////////////////////////////////////////////////////

ostream& operator<<(ostream& out, const SID& sid);
istream& operator>>(istream& in, SID& sid);

struct less_SID {
    bool operator() (SID a, SID b) const {
	return
	    (a.GetIP() < b.GetIP()) ||
	    ((a.GetIP() == b.GetIP()) && (a.GetPort() < b.GetPort()));
    }
};

struct less_SIDPtr {
    bool operator() ( const SID *a, const SID *b) const {
	return
	    (a->GetIP() < b->GetIP()) ||
	    ((a->GetIP() == b->GetIP()) && (a->GetPort() < b->GetPort()));
    }
};

// server UID utilities
typedef set<SID, less_SID> SIDSet;
typedef SIDSet::iterator SIDSetIter;
typedef vector<SID> SIDVec;
typedef SIDVec::iterator SIDVecIter;
typedef list<SID> SIDList;
typedef SIDList::iterator SIDListIter;

struct hash_SID {
    hash<uint32> H;

    size_t operator() (const SID a) const { return H(a.GetIP() ^ a.GetPort());
    }
};

struct equal_SID {
    bool operator() ( const SID a, const SID b ) const {
	return a == b;
    }
};

struct hash_SIDPtr {
    hash<uint32> H;

    size_t operator() (const SID *a) const {
	return H(a->GetIP() ^ a->GetPort());
    }
};

struct equal_SIDPtr {
    bool operator() ( const SID *a, const SID *b ) const {
	return *a == *b;
    }
};

typedef map<GUID, SID, less_GUID> SIDMap;
typedef SIDMap::iterator SIDMapIter;

typedef map<SID, TimeVal, less_SID> SIDTimeValMap;
typedef SIDTimeValMap::iterator SIDTimeValMapIter;

/* some stuff from om/common.h */

///////////////////////////////////////////////////////////////////////////////
// hash functions
struct hash_ptr {
    hash<uint32> H;

    size_t operator() (const void *a) const {
        return H((uint32)a);
    }
};

struct equal_ptr {
    bool operator() ( const void *a, const void *b ) const {
        return a == b;
    }
};

struct less_ptr {
    bool operator() ( const void *a, const void *b ) const {
        return (uint32)a < (uint32)b;
    }
};

struct hash_string {
    hash<const char *> h;

    int operator()(const string& str) const {
        return h(str.c_str());
    }
};

////////////////////////////////////////////////////////////////
#endif // __ID_H__
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
