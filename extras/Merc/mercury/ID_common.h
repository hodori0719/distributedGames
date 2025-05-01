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
  ID_common.h

begin           : Oct 16, 2003
version         : $Id: ID_common.h 2382 2005-11-03 22:54:59Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu 

***************************************************************************/

#ifndef __ID_COMMON_H__
#define __ID_COMMON_H__

// This file contains all the GUID and SID definitions that don't require
// templates. ID.h includes this plus the datastructure utilities.

#include <mercury/IPEndPoint.h>

/**
 * Globally unique object ID.
 */
struct GUID : public Serializable {
    uint32 m_IP;
    uint16 m_Port;
    uint32 m_LocalOID;

    GUID() : m_IP(0), m_Port(0), m_LocalOID(0) {}
    GUID(uint32 ip, uint16 port, uint32 localOID) :
	m_IP(ip), m_Port(port), m_LocalOID(localOID) {}
    // convenience constructor
    GUID (IPEndPoint ipport, uint32 localOID) :
	m_IP (ipport.GetIP ()), m_Port (ipport.GetPort ()), m_LocalOID (localOID) {}

    GUID(Packet *pkt);
    virtual ~GUID() {}

    inline uint32 GetIP() const {
	return m_IP;
    }
    inline uint16 GetPort() const {
	return m_Port;
    }
    inline uint32 GetLocalOID() const {
	return m_LocalOID;
    }

    const char *ToString(char *bufOut) const;
    const char *ToString() const;
    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *stream);

    static GUID CreateRandom() {
	uint32 ip   = (uint32)(drand48()*0xFFFFFFFFUL);
	uint16 port = (uint16)(drand48()*0xFFFF);
	uint32 id   = (uint32)(drand48()*0xFFFFFFFFUL);
	return GUID(ip, port, id); 
    }
};
typedef GUID guid_t;
// server UID
typedef IPEndPoint SID;
typedef SID sid_t;

extern GUID GUID_NONE;
extern SID SID_NONE;

///////////////////////////////////////////////////////////////////////////////
///// Globals

// extern sid_t g_LocalSID;

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
