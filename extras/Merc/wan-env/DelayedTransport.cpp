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

/*
 * Topology loading code derived from:
 *
 * Copyright (c) 2003 [Jinyang Li]
 *                    Massachusetts Institute of Technology
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string>
#include <fstream>
#include <wan-env/DelayedConnection.h>
#include <wan-env/DelayedTransport.h>

bool       DelayedTransport::m_LatMapInited = false;    
LatencyMap DelayedTransport::m_LatMap(512);

// returns a vector of words in line separated by all characters in delims,
// defaulting to whitespace
vector<string>
split(string line, string delims = " \t")
{
    string::size_type bi, ei;
    vector<string> words;

    bi = line.find_first_not_of(delims);
    while(bi != string::npos) {
	ei = line.find_first_of(delims, bi);
	if(ei == string::npos)
	    ei = line.length();
	words.push_back(line.substr(bi, ei-bi));
	bi = line.find_first_not_of(delims, ei);
    }

    return words;
}

void DelayedTransport::InitLatMap(const SID& me, const char *filename)
{
    if (m_LatMapInited)
	return;
    m_LatMapInited = true;

    //return;

    ASSERT(me != SID_NONE);

    ifstream ifs;

    do {
	ifs.open(filename);
    } while (!ifs && errno == EINTR);

    if (!ifs) {
	WARN << "couldn't open latency graph file: " << filename << endl;
	Debug::die("");
    }

    string line;
    int myid = -1;
    multimap<int, SID> idmap;
    char name[255];

    int i = 0;
    while(getline(ifs,line)) {
	if (i++ % 500 == 0) { 
	    DB_DO (-1) { cerr << "loading latfile, #lines done=" << i << endl; }
	}
	vector<string> words = split(line);
	// skip empty lines and commented lines
	if(words.empty() || words[0][0] == '#')
	    continue;

	// skip empty lines and commented lines
	if (words.empty() || words[0][0] == '#')
	    continue;

	if (words[0] == "node") {

	    ASSERT(words.size() >= 3);
	    int id = atoi(words[1].c_str());
	    for (int i=2; i<(int)words.size(); i++) {
		strncpy(name, words[i].c_str(), 255);
		SID sid(name);

		if (sid == SID_NONE) {
		    WARN << "can't resolve hostname: " << name << endl;
		    Debug::die("");
		}
		idmap.insert(pair<int,SID>(id, sid));

		if (sid == me) {
		    ASSERT(myid == -1);
		    myid = id;
		}
	    }

	} else {

	    vector<string> pair = split(words[0], ",");
	    int id1 = atoi(pair[0].c_str());
	    int id2 = atoi(pair[1].c_str());

	    int other;

	    if (myid == -1) {
		WARN << "Didn't find myself in latency graph! "
		     << me << endl;
		Debug::die("");
	    }

	    if (id1 == myid)
		other = id2;
	    else if (id2 == myid)
		other = id1;
	    else
		continue;

	    // file contains RTTs -- so div by 2 to get 1-way delay
	    float lat = atof(words[1].c_str())/2;

	    multimap<int,SID>::iterator start = idmap.lower_bound(other);
	    multimap<int,SID>::iterator end   = idmap.upper_bound(other);
	    if (start == end) {
		WARN << "line with node id = " << other
		     << " but never declared as a node" << endl;
		Debug::die("");
	    }
	    for ( ; start != end; start++) {
		m_LatMap[start->second] = lat;
	    }
	}
    }
    // now insert myself
    multimap<int,SID>::iterator start = idmap.lower_bound(myid);
    multimap<int,SID>::iterator end   = idmap.upper_bound(myid);
    ASSERT(start != end);
    for ( ; start != end; start++) {
	m_LatMap[start->second] = 0;
    }

    INFO << "loaded latency map from: " << filename << endl;
    DB_DO(1) {
	INFO << "latency map:" << endl;
	for (LatencyMapIter it = m_LatMap.begin(); 
	     it != m_LatMap.end(); it++) {
	    INFO << "<- " << it->first << "\t= " << it->second << endl;
	}
    }

#if 0
    if (g_Preferences.slowdown_factor > 1.0f) {
	for (LatencyMapIter it = m_LatMap.begin(); it != m_LatMap.end(); it++) {
	    it->second *= g_Preferences.slowdown_factor;
	}	
    }
#endif
}

float DelayedTransport::GetLat(const SID& peer)
{
    ASSERT(m_LatMapInited);

    //return 100;

    // XXX TODO: add some gaussian noise?
    LatencyMapIter p = m_LatMap.find(peer);
    if (p == m_LatMap.end()) {
	DB(1) << "No artificial latency from " << peer << endl;
	return 0;
    }

    return p->second;
}

///////////////////////////////////////////////////////////////////////////////

DelayedTransport::DelayedTransport(Transport *t) : m_Trans(t) 
{
    ASSERT(g_Preferences.latency);

    // HACK: do this in some way that makes more sense?
    InitLatMap(m_Trans->GetAppID (), g_Preferences.latency_file);

    m_Network = m_Trans->GetNetwork();
    m_ID      = m_Trans->GetAppID();
    m_Proto   = m_Trans->GetProtocol();
}

DelayedTransport::~DelayedTransport() 
{ 
    delete m_Trans; 
}

void  DelayedTransport::StartListening()
{
    m_Trans->StartListening();
}

void  DelayedTransport::StopListening()
{
    m_Trans->StopListening();
}

int   DelayedTransport::FillReadSet(fd_set *tofill)
{
    return m_Trans->FillReadSet(tofill);
}

void  DelayedTransport::DoWork(fd_set *isset, u_long timeout_usecs)
{
    // XXX: damn, we now need to estimate how much time to 
    // split between us and the actual transport layer

    unsigned long long stoptime = CurrentTimeUsec() + (unsigned long long) timeout_usecs;
    float SELF_FRAC = 0.2;
    m_Trans->DoWork (isset, (u_long) ((1 - SELF_FRAC) * timeout_usecs));

    Connection *connection;
    DelayedConnection *my_connection;
    ConnStatusType status;
    Packet *pkt;
    PacketAuxInfo aux;

    // XXX: this is dangerous; we dont care for timeout_usecs.    
    while (true) { 
	status = m_Trans->GetReadyConnection(&connection);

	if (status == CONN_NOMSG)
	    break;

	my_connection = GetMyConnection(connection->GetAppPeerAddress());

	switch (status) {
	case CONN_CLOSED:
	    my_connection->Insert( CONN_CLOSED, NULL, TIME_NONE /* irrelevant */ );
	    break;
	case CONN_ERROR:
	    my_connection->Insert( CONN_ERROR, NULL, TIME_NONE /* irrelevant */ );
	    break;
	case CONN_NEWINCOMING:
	case CONN_OK: {
	    pkt = connection->GetNextPacket(&aux);
	    ASSERT(pkt);
	    connection->SetStatus( status );

	    // determine the time we should have recived the packet...
	    TimeVal time = aux.timestamp + 
		GetLat(*connection->GetAppPeerAddress());
	    my_connection->Insert(status, pkt, time);
	    break;
	}
	default:
	    WARN << "BUG: should never be here..." << endl;
	    ASSERT(0);
	}
    }
}

Connection *DelayedTransport::GetConnection(IPEndPoint *target)
{
    // when requesting a connection to send from, delegate to actual trans
    return m_Trans->GetConnection(target);
}

ConnStatusType DelayedTransport::GetReadyConnection(Connection **connp)
{
    // when geting a ready connection, return our own connections
    // because they are delayed.
    ConnStatusType ret = CONN_NOMSG;
    DelayedConnection *connection = NULL;

    TimeVal now = TimeNow ();

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); iter++) {
	connection = (DelayedConnection *)(*iter);

	ConnStatusType next = connection->ReadyStatus(now);
	if (next != CONN_NOMSG) {
	    connection->SetStatus(next);
	    ret = connection->GetStatus();

	    // we serviced this fellow now - let the other guys have a chance!
	    // put this connection at the END of the list.
	    m_ConnectionList.erase(iter);
	    m_ConnectionList.push_back(connection);

	    *connp = connection;
	    //INFO << "here: " << connection << endl;

	    break;
	}
    }

    //INFO << "here: " << g_ConnStatusStrings[ret] 
    //     << " listsize=" << m_ConnectionList.size() << endl;

    return ret;
}

void DelayedTransport::CloseConnection(IPEndPoint *target)
{
    //Lock();
    IPEndPoint copy = *target;

    for (ConnectionListIter iter = m_ConnectionList.begin(); 
	 iter != m_ConnectionList.end(); /* !!! */) {
	ConnectionListIter oit = iter;
	oit++;

	if (*(*iter)->GetAppPeerAddress() == *target) {
	    delete *iter;
	    m_ConnectionList.erase(iter);
	}
	iter = oit;
    }
    m_AppConnHash.Flush(&copy);
    //Unlock();

    m_Trans->CloseConnection(&copy);
}

DelayedConnection *DelayedTransport::GetMyConnection(SID *peer)
{
    DelayedConnection *conn = (DelayedConnection *)m_AppConnHash.Lookup(peer);
    if (!conn) {
	conn = new DelayedConnection(this, -1, peer);
	//Lock();
	m_ConnectionList.push_back(conn);
	m_AppConnHash.Insert(peer, conn);
	//Unlock();
    }
    return conn;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
