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

/**************************************************************************
  TimedStruct.h

  An interface for objects that timeout (generic soft state management).

begin           : Nov 6, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/

#ifndef __TIMED_STRUCT_H__
#define __TIMED_STRUCT_H__

#include <util/types.h>
#include <util/OS.h>
#include <mercury/ID.h>
#include <mercury/common.h>

    /**
     * Interface to Objects that "timeout"
     */
    class TimedStruct : virtual public Serializable {
    TimeVal m_Start;
    TimeVal m_Timeout;
    TimeVal m_LastRefresh;
    uint32  m_TTL;
    public:

    /**
     * Create an object with a timeout.
     *
     * @param ttl_msec the time-to-live of the object
     * @param now the current time (optional)
     */
    TimedStruct(uint32 ttl_msec, TimeVal now = TIME_NONE)
	{
	    if (now == TIME_NONE) {
		OS::GetCurrentTime(&now);
	    }
	    m_TTL = ttl_msec;
	    m_Start = now;
	    m_LastRefresh = now;
	    m_Timeout = now + m_TTL;
	}

    virtual ~TimedStruct() {}

    /**
     * Refresh the timeout on this object. The new timeout is the
     * MAX of the old timeout and the new one.
     *
     * @param new_ttl change the TTL in msec (optional)
     * @param now the current time (optional)
     */
    inline void Refresh(uint32 new_ttl = 0, TimeVal now = TIME_NONE) 
	{
	    if (new_ttl > 0) {
		m_TTL = new_ttl;
	    }
	    if (now == TIME_NONE) {
		OS::GetCurrentTime(&now);
	    }
	    // only do the refresh if it makes the timeout later
	    if (now + m_TTL > m_Timeout) {
		m_Timeout = now + m_TTL;
	    }
	    m_LastRefresh = now;
	}

    /**
     * Immediately expire this object (set TTL == 0).
     *
     * @param now the current time (optional)
     */
    inline void Expire(TimeVal now = TIME_NONE)
	{
	    if (now == TIME_NONE) {
		OS::GetCurrentTime(&now);
	    }
	    m_TTL = 0;
	    m_Timeout = now;
	}

    /**
     * Returns true if this object timed out
     *
     * @param now the current time (optional)
     */
    inline bool TimedOut(TimeVal now = TIME_NONE) const
	{
	    if (now == TIME_NONE) {
		OS::GetCurrentTime(&now);
	    }
	    return now >= m_Timeout;
	}

    /**
     * Return the time when this object was initially created.
     */
    inline TimeVal StartTime() const
	{
	    return m_Start;
	}

    /**
     * Return the time left until the timeout.
     *
     * @param now the current time (optional)
     */
    inline uint32 TimeLeft(TimeVal now = TIME_NONE) const
	{
	    if (now == TIME_NONE) {
		OS::GetCurrentTime(&now);
	    }
	    return m_Timeout < now ? 0 : m_Timeout - now;
	}

    /**
     * Return the amount of time from the StartTime until the current time
     * (in msec)
     */
    inline uint32 LifeTime(TimeVal now = TIME_NONE) const
	{
	    if (now == TIME_NONE) {
		OS::GetCurrentTime(&now);
	    }
	    return now - m_Start;
	}

    /**
     * Return the current TTL of the object (this is NOT the time left until
     * timeout; it is the last assigned TTL from a Refresh).
     */
    inline uint32 GetTTL() const
	{ 
	    return m_TTL; 
	}

    ////////// Implements Serializable

    TimedStruct(Packet *pkt) 
	{
	    pkt->ReadBuffer((byte *)&m_Start, sizeof(m_Start));
	    pkt->ReadBuffer((byte *)&m_Timeout, sizeof(m_Timeout));
	    pkt->ReadBuffer((byte *)&m_LastRefresh, sizeof(m_Timeout));
	    m_TTL = pkt->ReadInt();
	}

    void Serialize(Packet *pkt)
	{
	    pkt->WriteBuffer((byte *)&m_Start, sizeof(m_Start));
	    pkt->WriteBuffer((byte *)&m_Timeout, sizeof(m_Timeout));
	    pkt->WriteBuffer((byte *)&m_LastRefresh, sizeof(m_Timeout));
	    pkt->WriteInt(m_TTL);
	}

    uint32 GetLength()
	{
	    return sizeof(m_Start) + 
		sizeof(m_Timeout) + 
		sizeof(m_LastRefresh) +
		sizeof(m_TTL);
	}

    void Print(FILE *stream)
	{
	    ASSERT(0);
	}

    ////////// Debugging

    inline void SetLifeTime(uint32 life) {
	m_Timeout = m_Start + (m_TTL + life);
    }
  };

///////////////////////////////////////////////////////////////////////////////
///// Basic Implementations

class TimedGUID : public TimedStruct, public GUID {
 public:
    TimedGUID() : TimedStruct((uint32)0), GUID() {}
    TimedGUID(uint32 ttl_msec) : TimedStruct(ttl_msec), GUID() {}
    TimedGUID(Packet *pkt) : TimedStruct(pkt), GUID(pkt) {}
    virtual ~TimedGUID() { }

    inline void Copy(GUID& guid) {
	m_IP   = guid.GetIP();
	m_Port = guid.GetPort();
	m_LocalOID = guid.GetLocalOID();
    }

    inline GUID ToGUID() {
	GUID ret = GUID(GetIP(), GetPort(), GetLocalOID());
	return ret;
    }

    void Serialize(Packet *pkt) { 
	TimedStruct::Serialize(pkt);
	GUID::Serialize(pkt);
    }

    uint32 GetLength() {
	return TimedStruct::GetLength() + GUID::GetLength();
    }

    void Print(FILE *stream) {
	TimedStruct::Print(stream);
	GUID::Print(stream);
    }
};

class TimedSID : public TimedStruct, public SID {
 public:
    TimedSID() : TimedStruct((uint32)0), SID() {}
    TimedSID(uint32 ttl_msec) : TimedStruct(ttl_msec), SID() {}
    TimedSID(Packet *pkt) : TimedStruct(pkt), SID(pkt) {}
    virtual ~TimedSID() { }

    inline void Copy(SID& sid) {
	m_IP = sid.GetIP();
	m_Port = sid.GetPort();
    }

    inline SID ToSID() {
	SID ret = SID(GetIP(), GetPort());
	return ret;
    }

    void Serialize(Packet *pkt) { 
	TimedStruct::Serialize(pkt);
	SID::Serialize(pkt);
    }

    uint32 GetLength() {
	return TimedStruct::GetLength() + SID::GetLength();
    }

    void Print(FILE *stream) {
	TimedStruct::Print(stream);
	SID::Print(stream);
    }
};

///////////////////////////////////////////////////////////////////////////////
///////// Utility

typedef set<TimedGUID *, less_GUIDPtr> TimedGUIDSet;
typedef TimedGUIDSet::iterator         TimedGUIDSetIter;
typedef vector<TimedGUID *>            TimedGUIDVec;
typedef TimedGUIDVec::iterator         TimedGUIDVecIter;

typedef set<TimedSID *, less_SIDPtr>   TimedSIDSet;
typedef TimedSIDSet::iterator          TimedSIDSetIter;
typedef vector<TimedSID *>             TimedSIDVec;
typedef TimedSIDVec::iterator          TimedSIDVecIter;

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
