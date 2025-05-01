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

#include <mercury/Cache.h>
#include <mercury/Constraint.h>
#include <time.h>
#include <mercury/Hub.h>
#include <map>

const char *g_CacheTypeStrings[] = {
    "CACHE_SINGLE",
    "CACHE_LRU",
    "CACHE_UNIFORM"
};

const char *g_pubsub_strs[] = {
    "EVENT",
    "INTEREST",
    "DISINTEREST"
};

CacheEntry::CacheEntry(IPEndPoint address, NodeRange &range) :
    m_Address(address), m_Range(range), m_NumUsed(0)
{
    m_Timestamp = time(0);
}

void CacheEntry::Print(FILE * stream)
{
    fprintf(stream, "\t>>\t [%s] ", m_Address.ToString());

    // the annoying 'Value *' cast is needed; otherwise, 
    // will need to make the Print() const 

    Value& min = (Value &) m_Range.GetMin();
    Value& max = (Value &) m_Range.GetMax();

    min.Print(stream);     
    max.Print(stream);
    fprintf(stream, "time=%u\n", (unsigned int) m_Timestamp);
}

CacheEntry::~CacheEntry()
{
}

/////////////////////////////////////////////////////////////////////

struct less_Value {
    bool operator() (const Value *av, const Value *bv) const {
	return av < bv;
    }
};

typedef map<const Value *, CacheEntry *, less_Value> CacheMap;
typedef CacheMap::iterator CacheMapIter;

class CacheImpl {
    CacheMap entries;

public:
    CacheImpl() {}
    ~CacheImpl() {
	for (CacheMapIter it = entries.begin(); it != entries.end(); it++) 
	    delete it->second;
	entries.clear();
    }

    CacheMapIter begin() { return entries.begin(); }
    CacheMapIter end() { return entries.end(); }
    size_t size() { return entries.size(); }
    void erase(CacheMapIter it) { 
	delete it->second;
	entries.erase(it); 
    }

    // I own it!
    void AddEntry(CacheEntry *entry) {
	entries.insert(CacheMap::value_type(&entry->GetRange().GetMin(), entry));
    }
};

Cache::Cache(CacheType type, int maxsize, Hub *hub) :
    m_Type(type), m_Maxsize(maxsize), m_Hub(hub)
{
    m_Impl = new CacheImpl();
}

Cache::~Cache()
{
    delete m_Impl;
}

bool Cache::Less (const Value& val, const Value& other)
{
    if (val == m_Hub->GetAbsMax ()) 
	return val <= other;
    else
	return val < other;
}

// this code is similar to the code in Hub::GetNearestPeer. 
// our caller should compare whether the entry returned by 
// the Cache or the one returned by Hub::GetNearestPeer is
// closer to 'want'

CacheEntry *Cache::LookupEntry(const Value &val)
{
    if (m_Impl->size() == 0)
	return NULL;

    CacheEntry *nearest = NULL;

    for (CacheMapIter it = m_Impl->begin () ; it != m_Impl->end(); ++it)
    {
	const NodeRange& cur = it->second->GetRange ();
	DB (10) << " <<<<<< considering entry=" << it->second << endl;

	// GetMax () for a peer remains same even when other nodes join 
	// near that peer (since other nodes install themselves as 
	// predecessors) - so this way, information is less likely 
	// to be stale

	CacheMapIter nit (it);
	++nit;

	if (nit != m_Impl->end ()) {
	    const NodeRange& next = nit->second->GetRange ();

	    if (val >= cur.GetMax () && Less (val, next.GetMax ())) {
		if (val < next.GetMin ())
		    nearest = it->second;
		else
		    nearest = nit->second;

		DB (10) << " <<<<<<<<<<<<<<< found nearest " << nearest->GetRange () << endl;
		break;
	    }
	}
	else {
	    if (val >= cur.GetMax ())
		nearest = it->second;
	    else { 
		// this can only happen when val < first.GetMax ()

		const NodeRange& first = m_Impl->begin ()->second->GetRange ();
		ASSERT (val < first.GetMax ());
		if (val >= first.GetMin ())  // belongs to first guy
		    nearest = m_Impl->begin ()->second;
		else                         // between last and first
		    nearest = it->second;
	    }
	    DB (10) << " <<<<<<<<<<<<<<< found nearest " << nearest->GetRange () << endl;
	    break;
	}
    }

    ASSERT (nearest != NULL);
    return nearest;
}

void Cache::Print(FILE * stream)
{
    fprintf(stream, "********* Cache ********************* \n");
    int i = 0;
    for (CacheMapIter it = m_Impl->begin(); it != m_Impl->end(); it++, i++) 	{
	CacheEntry *entry = it->second;

	fprintf(stream, "\t>> entry(%d):\n", i);
	entry->Print(stream);
    }

    fprintf(stream, "---------- end ---------------------\n");
}

void SingleCache::InsertEntry(CacheEntry * e)
{
    if (m_Impl->size() == 0) {
	m_Impl->AddEntry(e);
	return;
    }

    m_Impl->erase(m_Impl->begin());
    m_Impl->AddEntry(e);
}


void LRUCache::InsertEntry(CacheEntry * e)
{
    // Check to see if the same entry is present or not.

    for (CacheMapIter it = m_Impl->begin(); it != m_Impl->end(); it++) 	{
	CacheEntry *entry = it->second;

	if (! (entry->GetAddress() == e->GetAddress()))
	    continue;
	if (entry->GetRange() == e->GetRange()) {
	    entry->m_Timestamp = time(0);
	    delete e;
	    return;
	}
    }

    if ((int) m_Impl->size() < m_Maxsize) {
	e->m_NumUsed = 0;
	m_Impl->AddEntry(e);
	return;
    }

    CacheMapIter to_kick = m_Impl->end();
    int min = INT_MAX;

    for (CacheMapIter it = m_Impl->begin(); it != m_Impl->end(); ++it) 	{
	CacheEntry *entry = it->second;

	if (entry->m_NumUsed < min) {
	    min = entry->m_NumUsed;
	    to_kick = it;
	}
    }

    ASSERT(to_kick != m_Impl->end());
    m_Impl->erase(to_kick);
    m_Impl->AddEntry(e);
}

CacheEntry *LRUCache::LookupEntry(const Value& want)
{
    CacheEntry *ent = Cache::LookupEntry(want);
    if (ent)
	ent->Used();     // FIXME: the caller may not use us! so really the caller should call 'Used()'

    return ent;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
