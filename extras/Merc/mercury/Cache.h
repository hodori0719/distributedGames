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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:nil -*- */

#ifndef __CACHE__H
#define __CACHE__H

#include <string>

#include <mercury/IPEndPoint.h>
#include <mercury/Constraint.h>
#include <time.h>

// make sure this matches g_cache_strs in global_strings.cpp
typedef enum { CACHE_SINGLE, CACHE_LRU, CACHE_UNIFORM } CacheType;

extern const char* g_CacheTypeStrings[];
extern const char* g_pubsub_strs[];

class Hub;

struct CacheEntry {
    IPEndPoint m_Address;
    NodeRange  m_Range;
    int        m_NumUsed;
    time_t     m_Timestamp;

    CacheEntry(IPEndPoint addr, NodeRange &range);
    ~CacheEntry();

    const IPEndPoint &GetAddress() { return m_Address; }
    const NodeRange  &GetRange()   { return m_Range;   }
    void Used() { 
	if (m_NumUsed != INT_MAX)     // avoid overflow
	    m_NumUsed++; 
    }

    void Print(FILE *stream);
};

class CacheImpl;

class Cache {
 protected:
    CacheType     m_Type;
    int           m_Maxsize;
    CacheImpl    *m_Impl;
    Hub          *m_Hub;
 public:
    Cache(CacheType type, int maxsize, Hub *hub);
    virtual ~Cache();    
    CacheType GetType() { return m_Type; }

    void Print(FILE *stream);
    virtual CacheEntry *LookupEntry(const Value &want);
    virtual void InsertEntry(CacheEntry *) = 0;
    virtual void Expire() {}
 private:
    bool Less (const Value& val, const Value& other);
};

class SingleCache : public Cache {
 public:
    SingleCache(Hub *hub) : Cache(CACHE_SINGLE, 1, hub) {}
    void InsertEntry(CacheEntry *);
};

class LRUCache : public Cache {
 public:
    LRUCache(int maxsize, Hub *hub) : Cache(CACHE_LRU, maxsize, hub) {}
    void InsertEntry(CacheEntry *);
    CacheEntry *LookupEntry(const Value &want); 
};
#endif // __CACHE__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
