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

#ifndef __INTEREST__H
#define __INTEREST__H

#include <string>
#include <vector>
#include <list>
#include <mercury/NetworkLayer.h>
#include <mercury/Constraint.h>
#include <mercury/AutoSerializer.h>
#include <mercury/ID.h>
#include <mercury/IPEndPoint.h>

class Event;

typedef byte InterestType;
extern InterestType INTEREST;
void RegisterInterestTypes();

/**
 * A subscription passed to Mercury. GObjects periodically produce one
 * of these to indicate their area of interest.
 */
class Interest : public virtual Serializable {
 private:
    guid_t          m_GUID;         // this is more for identifying subs	

    IPEndPoint		m_Subscriber;   // address of the subscriber
    uint32          m_LifeTime;     // how long sub should live, if softstate

    uint32          m_Nonce;        // Measurement variable

    TimeVal         m_DeathTime;    // Not serialized. Used by MercuryNode; HACK

    ConstraintVec   m_Constraints;

 protected:
    DECLARE_TYPE(Interest, Interest);

 public:
    Interest ();
    Interest (IPEndPoint& subscriber);
    Interest (IPEndPoint& subscriber, guid_t g); 
    Interest (Packet *pkt);
    virtual ~Interest() {}

    void    AddConstraint(Constraint& c);
    Constraint* GetConstraintByAttr (int attr);    
    int GetNumConstraints () { 
	return (int) m_Constraints.size ();
    }

    Constraint* GetConstraint (int index) {
	ASSERT (index < (int) m_Constraints.size());
	return &m_Constraints [index];
    }

    void SetSubscriber (IPEndPoint e) { m_Subscriber = e; }
    const IPEndPoint &GetSubscriber() const { return m_Subscriber; }

    bool Overlaps (Event *evt);

    bool IsEqual(Interest *);

    void SetLifeTime(uint32 lifetime) { m_LifeTime = lifetime; }
    uint32 GetLifeTime() const { return m_LifeTime; }

    // used by MercuryNode ONLY to manage this as softstate
    void SetDeathTime(TimeVal time) { m_DeathTime = time; }
    const TimeVal& GetDeathTime() const { return m_DeathTime; }

    // Jeff: no longer only for debugging! used as a unique ID for the 
    // Interest. HACKY but no time to do anything else for now.
    void SetGUID(guid_t g) {  m_GUID = g;	}
    const guid_t GetGUID() const { return m_GUID;	}

    ///// MEASUREMENT (see ObjectLogs.h)
    void   SetNonce(uint32 nonce) { m_Nonce = nonce; }
    uint32 GetNonce() const { return m_Nonce; }

    virtual void   Serialize(Packet *pkt);
    virtual uint32 GetLength();
    virtual void   Print(FILE *stream);
    virtual const char *TypeString() { return "INTEREST"; }

    virtual ostream& Print (ostream& os) const;

    /// other classes access constraints using this iterator 
    /// so that my implementation details are hidden. 

    class iterator;
    friend class iterator;
    class iterator {
	Interest&         in;
	ConstraintVecIter it;
    public:
	iterator (Interest& in) : in (in), it (in.m_Constraints.begin()) {}
	iterator (Interest& in, bool) 
	    : in (in), it (in.m_Constraints.end ()) {}

	iterator(const iterator& o)
	    : in (o.in), it (o.it) {}

	Constraint *operator*() const { return &*it; }

	iterator& operator++() { // Prefix
	    ASSERT (it != in.m_Constraints.end()); 
	    it++;
	    return *this;
	}
	iterator& operator++(int) { // Postfix
	    return operator++();
	}

	bool operator== (const iterator& o) const {
	    return it == o.it;
	}
	bool operator!= (const iterator& o) const {
	    return it != o.it;
	}
	friend ostream& 
	    operator<<(ostream& os, const iterator& it) {
	    return os << *it;
	}
    };

    // hmm. i am unclear as to what the best strategy here is.
    // i would like an iterator for both 'const' and 'non-const' 
    // Interest objects. at this point, i will make do with this
    // ugly cast. - Ashwin [02/13/2005]

    iterator begin() const { return iterator((Interest& ) *this); }
    iterator end() const { return iterator((Interest& ) *this, true);  }
};

ostream& operator<<(ostream& out, const Interest& sub);
ostream& operator<<(ostream& out, const Interest *sub);

#endif // __INTEREST__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
