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
  BootstrapNode.cpp

begin                 : March 11, 2003
copyright            : (C) 2003-2005 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2003 Jeff Pang (jeffpang@cs.cmu.edu)

**************************************************************************/


#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#include <mercury/BootstrapNode.h>
#include <mercury/Hub.h>
#include <mercury/Histogram.h>
#include <mercury/Parameters.h>
#include <mercury/MercuryID.h>
#include <mercury/Timer.h>
#include <util/Utils.h>

    class UpdateHistoTimer : public Timer {
	BootstrapNode *m_Node;

    public:
	UpdateHistoTimer(BootstrapNode *node, u_long timeout) : Timer (timeout), m_Node (node) {}

	void OnTimeout() {
	    m_Node->UpdateHistograms ();
	    _RescheduleTimer(Parameters::BootstrapUpdateHistInterval);
	}
    };

// periodically check if all servers are joined to all hubs during the
// join process, so we know when the overlay has "stabilized"
class CheckAllJoinedTimer : public Timer {
    BootstrapNode *m_Node;

public:
    CheckAllJoinedTimer(BootstrapNode *node, u_long timeout) : Timer (timeout), m_Node (node) {}

    void OnTimeout() {
	if (!m_Node->CheckAllJoined ())
	    _RescheduleTimer (200);
    }
};

class CheckRingSanityTimer : public Timer {
    BootstrapNode *m_Node;
    u_long timeout;
public:
    CheckRingSanityTimer(BootstrapNode *node, u_long timeout) : Timer (timeout), m_Node (node), timeout (timeout) {}

    void OnTimeout() {
	m_Node->CheckRingSanity ();
	_RescheduleTimer (timeout);
    }
};

#define LINEBUF_SIZE  256
#define HUB_FIFO_SIZE 10000     // keep a long list for now;

#define IDENT_MAP (!strcmp (g_BootstrapPreferences.choosingPolicy, "ident-map"))

// preferences
struct _bootstrap_prefs_t g_BootstrapPreferences;

bool BootstrapNode::ReadSchema(FILE * fp)
{
    char line[LINEBUF_SIZE];
    int curr_line = 1;
    int attr_index = 0;
    int nbkts = g_BootstrapPreferences.nBuckets;

    while (fgets(line, LINEBUF_SIZE, fp)) {
	if (line[strlen(line) - 1] != '\n') {
	    fprintf(stderr,
		    "internal error: line %d longer than 255 characters, bailing\n",
		    curr_line);
	    return false;
	}
	//DB(0) << "processing line: " << line << endl;
	if (line[0] == '#' || line[0] == '\n') {
	    curr_line++;
	    continue;
	}

	BootstrapHubInfo *hinfo = ParseLine(line, curr_line);
	if (!hinfo)
	    return false;

	if (g_BootstrapPreferences.haveHistograms) {
	    ASSERT (nbkts > 0);

	    Histogram *hist;
	    hist = hinfo->m_Histogram = new Histogram();

	    NodeRange tmp (attr_index, hinfo->m_AbsMin, hinfo->m_AbsMax);

	    Value  span = tmp.GetSpan (hinfo->m_AbsMin, hinfo->m_AbsMax);
	    Value  incr = span;
	    incr /= nbkts;
	    Value  start, end;

	    start = hinfo->m_AbsMin;

	    // initialize the buckets with the proper range values;
	    for (int i = 0; i < nbkts; i++) {
		if (i == nbkts - 1) 
		    end = hinfo->m_AbsMax;
		else {
		    end = start;
		    end += incr;
		}

		NodeRange *r = new NodeRange (attr_index, start, end);
		hist->AddBucket (*r, 0);
		delete r;

		start = end;
	    }

	    DB(10) << "created histogram which looks like ... " << endl;
	    DB(10) << hist << endl;
	}

	m_HubInfoVec.push_back(hinfo);
	attr_index++;
	curr_line++;
    }

    if (m_HubInfoVec.size() == 0) {
	fprintf(stderr, "no hubs defined!\n");
	return false;
    }

    return true;
}

static vector<string> SplitLine(string line, string delims = " \t")
{
    // code taken from wan-env/DelayedTransport.cpp
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

struct idpair_less {
    bool operator()(const pair<IPEndPoint,MercuryID>& a, 
		    const pair<IPEndPoint,MercuryID>& b) const {
	return a.second < b.second;
    }
};

static int mod (int n, int modulus) {
    return (n % modulus + modulus) % modulus;
} 

bool BootstrapNode::ReadIdentMap(const char *filename)
{
    for (uint32 i=0; i<m_HubInfoVec.size(); i++) {
	m_IdentMap[m_HubInfoVec[i]->m_Name] = new ident_map_t();
    }

    ifstream ifs;

    do {
	ifs.open(filename);
    } while (!ifs && errno == EINTR);

    if (!ifs) {
	WARN << "can't open ident-map file: " << filename 
	     << ": " << strerror(errno) << endl;
	return false;
    }

    string line;
    int lineno = 0;
    while(getline(ifs,line)) {
	lineno++;
	vector<string> words = SplitLine(line);

	if (words.size() != 3) {
	    WARN << "bad line " << lineno << ": " << line << endl;
	    continue;
	}

	string hubname  = words[0];
	IPEndPoint addr = IPEndPoint((char *)words[1].c_str());
	MercuryID ident = MercuryID(words[2].c_str(), 16 /* xxx: some way for the config to specify this! */);

	map<string, ident_map_t *>::iterator ptr =
	    m_IdentMap.find(hubname);
	if (ptr == m_IdentMap.end()) {
	    WARN << "bad hubname on line " << lineno << ": " << line << endl;
	    continue;
	}

	if (addr == SID_NONE) {
	    WARN << "bad addr on line " << lineno << ": " << line << endl;
	    continue;
	}

	// sanity check the ident in [min,max) ?

	ident_map_t *m = ptr->second;
	m->insert(pair<IPEndPoint, MercuryID>(addr, ident));
    }

    ifs.close();

    // some sanity checks
    for (uint32 i=0; i<m_HubInfoVec.size(); i++) {
	if ( m_IdentMap[m_HubInfoVec[i]->m_Name]->size() < 
	     (uint32)g_BootstrapPreferences.nServers ) {
	    WARN << "expecting " << g_BootstrapPreferences.nServers << " nodes but hub "
		 << m_HubInfoVec[i]->m_Name << " only has "
		 << m_IdentMap[m_HubInfoVec[i]->m_Name]->size()
		 << " identifiers in ident-map" << endl;
	}
    }

    // number of nodes we wait for before starting the join process
    // xxx - assumes all nodes in first hub
    m_NumIdentMapNodes = m_IdentMap[m_HubInfoVec[0]->m_Name]->size();

    ///////////////////////////////////////////////////////////////////////////
    // Now build the hubinfo lists

    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	BootstrapHubInfo *info = m_HubInfoVec[i];
	ident_map_t *hub = m_IdentMap[info->m_Name];

	// sort the nodes in ring order
	vector<pair<IPEndPoint,MercuryID> > order;
	for (ident_map_t::iterator it = hub->begin(); it != hub->end(); it++) {
	    order.push_back(*it);
	}
	stable_sort(order.begin(), order.end(), idpair_less());

	// for each node, create its hubinfo
	for (int n=0; n<(int)order.size(); n++) {
	    const pair<IPEndPoint,MercuryID>& me = order[n];
	    const pair<IPEndPoint,MercuryID>& pred = 
		order[mod(n-1, order.size())];
	    const pair<IPEndPoint,MercuryID>& succ = 
		order[mod(n+1, order.size())];

	    HubInitInfo *newinfo = new HubInitInfo();
	    newinfo->ID       = i;
	    newinfo->name     = info->m_Name;
	    newinfo->absmin   = info->m_AbsMin;
	    newinfo->absmax   = info->m_AbsMax;
	    newinfo->isMember = true;
	    newinfo->rep = succ.first;

	    newinfo->staticjoin = true;
	    newinfo->range = NodeRange(i,pred.second,me.second);

	    //INFO << "*** me=" << me.first << " " << me.second << endl;
	    
	    for (int j=1; j<=Parameters::NSuccessorsToKeep; j++) {
		const pair<IPEndPoint,MercuryID>& pi = 
		    order[mod(n-j, order.size())];
		const pair<IPEndPoint,MercuryID>& si = 
		    order[mod(n+j, order.size())];

		NodeRange pr(i,order[mod(n-j-1, order.size())].second, 
			     pi.second);
		NodeRange sr(i,order[mod(n+j-1, order.size())].second, 
			     si.second);

		//INFO << mod(n-j, order.size()) << " pred=" << pi.first 
		//     << " " << pi.second << endl;
		//INFO << "succ=" << si.first << " " << si.second << endl;

		newinfo->preds.push_back( PeerInfo(pi.first, pr) );
		newinfo->succs.push_back( PeerInfo(si.first, sr) );

		if (pi.first == me.first) {
		    // wraped around
		    ASSERT(si.first == me.first);
		    break;
		}
	    }
	    
	    if (m_StaticInitInfo.find(order[i].first) == 
		m_StaticInitInfo.end()) {
		m_StaticInitInfo[order[n].first] = vector<HubInitInfo *>();
	    }
	    m_StaticInitInfo[order[n].first].push_back(newinfo);
	}
    }

    // sanity check
    for (static_map_t::iterator it=m_StaticInitInfo.begin();
	 it!=m_StaticInitInfo.end(); it++) {
	if (it->second.size() != m_HubInfoVec.size()) {
	    WARN << it->first << " is not a member of every hub! "
		 << "We don't support that yet!" << endl;
	    ASSERT(0);
	}
    }

    return true;
}

void BootstrapNode::UpdateHubHistogram (BootstrapHubInfo *hinfo)
{
    Histogram *hist = hinfo->m_Histogram;

    DB(10) << "updating histogram..." << endl;

    // xxx Jeff: shouldn't the histogram buckets be modified so that
    // the density of each bucket is about the same?

    for (int j = 0; j < hist->GetNumBuckets(); j++) {
	HistElem *he = (HistElem *) hist->GetBucket(j);

	//INFO << he->GetRange() << endl;

	he->SetValue (0);
	for (NodeListIter it = hinfo->m_Nodelist.begin(); it != hinfo->m_Nodelist.end(); it++)
	{
	    NodeRange *range = it->GetRange ();

	    // got nothing from this guy?
	    if (range == NULL || range->GetAttrIndex () < 0 || range->GetAttrIndex () >= (int) m_HubInfoVec.size ())
		continue;

	    Value olap = it->GetRange ()->GetOverlap (he->GetRange ());
	    Value span = it->GetRange ()->GetSpan (hinfo->m_AbsMin, hinfo->m_AbsMax);

	    if (span == Value::ZERO)
		continue;

	    // xxx Jeff: float is *very* low percision for this... I think
	    // we want to use a double...
	    he->SetValue (he->GetValue () + (float) (olap.double_div (span)));

	    if (olap != Value::ZERO) {
		DB(1) << it->GetAddress() << endl;
		DB(1) << *range << endl;
		DB(1) << ": olap=" << olap << " span=" << span << " val=" << he->GetValue () << endl;
	    }
	}
    }

    DB(10) << hist << endl;
}

void BootstrapNode::UpdateHistograms()
{
    if (!g_BootstrapPreferences.haveHistograms)
	return;

    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) 
	UpdateHubHistogram (m_HubInfoVec[i]);
}

BootstrapHubInfo *BootstrapNode::ParseLine(char *line, int curr_line)
{
    char name[LINEBUF_SIZE], min[LINEBUF_SIZE], max[LINEBUF_SIZE];
    BootstrapHubInfo *hinfo = 0;

    /*    attribute_name type abs_min abs_max     */
    if (sscanf(line, "%s %s %s", name, min, max) != 3) {
	fprintf(stderr, "#fields on line %d != 3\n", curr_line);
	return 0;
    }

    Value minval (min, /* base=*/ 10);
    Value maxval (max, /* base=*/ 10); 
    hinfo = new BootstrapHubInfo (name, minval, maxval);
    return hinfo;
}

bool BootstrapNode::CheckAllJoined()
{
    //SIDSet servers;
    bool allJoined = true;

    if ((int)m_BootedServers.size() < g_BootstrapPreferences.nServers) {
	FREQ(10,
	     DB(10) << "Got bootstrap reqs from " << m_BootedServers.size()
	     << " servers (expecting " << g_BootstrapPreferences.nServers << ")" << endl);
	return false;
    }

    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	BootstrapHubInfo *hinfo = m_HubInfoVec[i];

	NodeList & nodeList = hinfo->m_Nodelist;
	for (NodeListIter it = nodeList.begin(); it != nodeList.end(); it++) {
	    if ( !(*it).IsJoined() ) {
		allJoined = false;
		DB(10) << "Still waiting for " << (*it).m_Addr
		       << " to finish joining hub " << hinfo->m_Name << endl;
	    }
	    //servers.insert((*it).m_Addr);
	}
    }

    if (!allJoined) return false;

    DB(1) << "All expected " << g_BootstrapPreferences.nServers << " servers have joined "
	  << "hubs! All clear to get going!" << endl;

    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	BootstrapHubInfo *info = m_HubInfoVec[i];
	INFO << " hub[" << i << "]" << endl;
	for (NodeListIter it = info->m_Nodelist.begin (); it != info->m_Nodelist.end (); ++it) {
	    INFO << "\t" << it->GetAddress () << ":";
	    if (it->GetRange () == NULL)
		cerr << "(null)" << endl;
	    else
		cerr << "(" << it->GetRange () << ")" << endl;
	}
    }

    // tell everyone that it is ok to proceed
    MsgCB_AllJoined msg (m_Address);
    for (SIDSetIter it = m_BootedServers.begin();
	 it != m_BootedServers.end(); it++) {
	IPEndPoint target = *it;
	//DB(1) << "Sending alljoined to " << target << endl;
	m_Network->SendMessage(&msg, &target, PROTO_TCP); 
    }

    m_AllJoined = true;
    return true;
}

void BootstrapNode::CleanupState()
{
    TimeVal now = m_Scheduler->TimeNow ();

    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	BootstrapHubInfo *hinfo = m_HubInfoVec[i];

	NodeList & nodeList = hinfo->m_Nodelist;
	for (NodeListIter it = nodeList.begin(); it != nodeList.end();
	     /* WATCH OUT */ ) {
	    if (now - it->m_tLastHeartBeat > (sint64) Parameters::BootstrapHeartbeatTimeout) {
		NodeListIter oit = it;
		oit++;

		INFO << merc_va("Getting rid of {%s} for attribute {%s}",
				 it->GetAddress().ToString(), hinfo->m_Name.c_str()) << endl;
		nodeList.erase(it);
		it = oit;
	    } else {
		it++;
	    }
	}
    }

    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	DB(19) << merc_va("HUB: [%s]  #entries: %d",
			  m_HubInfoVec[i]->m_Name.c_str(),
			  m_HubInfoVec[i]->m_Nodelist.size()) << endl;
    }
}

void BootstrapNode::ReceiveMessage(IPEndPoint * from, Message * msg)
{
    MsgType t = msg->GetType ();

    if (t == MSG_HEARTBEAT)
	HandleHeartbeat(from, (MsgHeartBeat *) msg);
    else if (t == MSG_BOOTSTRAP_REQUEST) {

	if (IDENT_MAP) {
	    HandleBootstrapRequestStatic(from, (MsgBootstrapRequest *) msg);
	} else {
	    HandleBootstrapRequest(from, (MsgBootstrapRequest *) msg);
	}
    }
    else if (t == MSG_CB_ESTIMATE_REQ)
	HandleEstimateRequest(from, (MsgCB_EstimateReq *) msg);
    /// XXX HACK! 
    else if (t == MSG_PING)
	HandleBarrierPing (from, (MsgPing *) msg);
    else 
	Debug::warn ("Bootstrap server received an unknown message! %s\n", msg->TypeString ());

    m_Network->FreeMessage (msg);
}

// Use MsgPing as a "ping" to check if a barrier (all nodes joined) 
// has reached. 
void BootstrapNode::HandleBarrierPing (IPEndPoint *from, MsgPing *ping)
{
    INFO << " *** received barrier ping *** [all-joined=" << m_AllJoined << "]" << endl;
    MsgPing *resp = new MsgPing ();
    resp->pingNonce = m_AllJoined ? 0x1 : 0x0;
    m_Network->SendMessage (resp, from, PROTO_UDP);
    delete resp;
}

IPEndPoint BootstrapNode::ChooseRandom (BootstrapHubInfo *hinfo)
{
    IPEndPoint addr = SID_NONE;

    int index = (int) (drand48 () * hinfo->m_Nodelist.size());
    NodeListIter it = hinfo->m_Nodelist.begin();
    for (; index > 0; it++, index--)
	;

    addr = it->GetAddress ();
    INFO << "found node:" << addr << endl;
    return addr;
}

IPEndPoint BootstrapNode::ChooseSimulateDistributedJoin (BootstrapHubInfo *hinfo)
{
    IPEndPoint addr = SID_NONE;

    Value v = hinfo->m_AbsMax;
    v -= hinfo->m_AbsMin;

    Value w = GetRandom (v);
    w += hinfo->m_AbsMin;

    bool found = false;

    for (NodeListIter it = hinfo->m_Nodelist.begin(); it != hinfo->m_Nodelist.end (); ++it)
    {
	if (it->GetRange () == NULL)
	    continue;

	if (it->GetRange ()->Covers (w)) {
	    addr = it->GetAddress ();
	    found = true;

	    INFO << "found node:" << addr << " " << it->GetRange () << endl;
	    break;
	}
    }

    if (!found)  {
	INFO << "not found! just giving a random node now" << endl;
	return ChooseRandom (hinfo);
    }
    return addr;
}

struct less_nodestate_t {
    bool operator () (const NodeState& an, const NodeState& bn) const {
	return an.GetLevel () < bn.GetLevel ();
    }
};

IPEndPoint BootstrapNode::ChooseRoundRobin (BootstrapHubInfo *hinfo)
{
    IPEndPoint addr = SID_NONE;

    hinfo->m_Nodelist.sort (less_nodestate_t ());

    NodeListIter bit = hinfo->m_Nodelist.begin ();
    addr = bit->GetAddress ();
    bit->SetLevel (bit->GetLevel () + 1);

    DBG_DO { INFO << "choose address " << addr << "(infolist size=" << hinfo->m_Nodelist.size () << ")" << endl; }
    return addr;
}

/*
IPEndPoint BootstrapNode::ChooseFromIdentMap(IPEndPoint *from, BootstrapHubInfo * hinfo)
{

    MercuryID ident;
    bool ok = ChooseIdent(ident, from, hinfo);
    if (!ok) {
	WARN << "Failed to find ident for " << *from 
	     << "; falling back to round-robin" << endl;
	// xxx fail fast for now!
	ASSERT(0);
	return ChooseRoundRobin(hinfo);
    }

    // have to keep this sorted since we fall back on rr 
    hinfo->m_Nodelist.sort (less_nodestate_t ());

    IPEndPoint rep = SID_NONE;

    if (hinfo->m_Nodelist.size() == 0)
	return rep;

    // xxx not efficient!
    for (NodeListIter it = hinfo->m_Nodelist.begin(); 
	 it != hinfo->m_Nodelist.end(); it++) {

	if ((*it).GetRange() == NULL) {
	    continue;
	}

	if ( IsBetweenLeftInclusive( ident,
				     (*it).GetRange()->GetMin(),
				     (*it).GetRange()->GetMax() ) ) {
	    rep = (*it).GetAddress();
	    // we do rr if they aren't in the list, so keep this book keeping
	    (*it).SetLevel ((*it).GetLevel () + 1);
	    break;
	}
    }

    if (rep == SID_NONE) {
	// this is a bug! no nodes covering some range!
	WARN << "No node is currently covering " << ident << endl;
	DumpState();
	ASSERT(0);
    }

    return rep;
}
*/

IPEndPoint BootstrapNode::ChooseRepresentative(IPEndPoint *from, BootstrapHubInfo * hinfo)
{
    IPEndPoint addr = SID_NONE;

    if (hinfo->m_Nodelist.size () > 0) {
	if (!strcmp (g_BootstrapPreferences.choosingPolicy, "random"))
	    addr = ChooseRandom (hinfo);
	else if (!strcmp (g_BootstrapPreferences.choosingPolicy, "round-robin"))
	    addr = ChooseRoundRobin (hinfo);
	else if (!strcmp (g_BootstrapPreferences.choosingPolicy, "sim-distrib"))
	    addr = ChooseSimulateDistributedJoin (hinfo);
	else if (!strcmp (g_BootstrapPreferences.choosingPolicy, "ident-map"))
	    //addr = ChooseFromIdentMap(from, hinfo);
	    ASSERT(false); // should not get here!
	else {
	    WARN << "unknown choosing policy: " << g_BootstrapPreferences.choosingPolicy << endl;
	    ASSERT(0);
	}
    }

    return addr;
}

/*
bool BootstrapNode::ChooseIdent(MercuryID& ret, IPEndPoint *from, BootstrapHubInfo *hinfo)
{
    // only choose identifiers if using the ident-map policy
    if (!IDENT_MAP)
	return false;

    map<string, ident_map_t *>::iterator ptr = m_IdentMap.find(hinfo->m_Name);
    if (ptr == m_IdentMap.end()) {
	WARN << "No ident map for hub " << hinfo->m_Name << endl;
	return false;
    }

    ident_map_t *m = ptr->second;
    ident_map_t::iterator entry = m->find(*from);
    if (entry == m->end()) {
	WARN << "No ident entry for " << *from << " in hub " << hinfo->m_Name
	     << endl;
	return false;
    }

    ret = entry->second;
    return true;
}
*/

void BootstrapNode::DumpState()
{
    //DB_DO(10) {
	for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	    BootstrapHubInfo *info = m_HubInfoVec[i];

	    INFO << "hub: " << info->m_Name << endl;
	    NodeList & nodeList = info->m_Nodelist;
	    for (NodeListIter it = nodeList.begin(); it != nodeList.end();
		 it++) {

		INFO << " addr: " << it->m_Addr << " range : " << it->m_Range << endl;
	    }
	    INFO << endl;
	}
	//}
}

/*
void BootstrapNode::FixKnownRanges(NodeState& state,
				   HubInitInfo& newinfo,
				   BootstrapHubInfo *info)
{
    if (newinfo.usedesired) {
	// if we are giving the node an id, then we know what its range
	// should be (doesn't have to be eventually correct, but this
	// is required because when we choose reps, we assume that 
	// someone is covering every range...)

	if (newinfo.rep == SID_NONE) {
	    // this is the only node in the ring so far, its range 
	    // should be everything!
	    NodeRange newrange;

	    if (newinfo.desiredmax == info->m_AbsMax) {
		newrange = NodeRange(newinfo.ID, info->m_AbsMin, info->m_AbsMax);
	    } else {
		newrange = NodeRange(newinfo.ID, newinfo.desiredmax, newinfo.desiredmax);
	    }
	    state.SetRange(&newrange);
	} else {
	    NodeListIter rep_node = info->m_Nodelist.end();

	    // xxx: hacky hacky icky icky yuck
	    for (NodeListIter it = info->m_Nodelist.begin(); 
		 it != info->m_Nodelist.end(); it++) {
		if (it->GetAddress() == newinfo.rep) {
		    rep_node = it;
		    break;
		}
	    }
	    ASSERT(rep_node != info->m_Nodelist.end());

	    NodeRange newrange(newinfo.ID, rep_node->GetRange()->GetMin(),
			       newinfo.desiredmax);
	    ASSERT(newrange.GetMin() != newrange.GetMax());
	    NodeRange reprange(newinfo.ID, newinfo.desiredmax,
			       rep_node->GetRange()->GetMax());
	    ASSERT(reprange.GetMin() != reprange.GetMax());
	    state.SetRange(&newrange);
	    rep_node->SetRange(&reprange);
	}
    }
}
*/

/*
struct idpair_more {
    bool operator()(const pair<IPEndPoint,MercuryID>& a, 
		    const pair<IPEndPoint,MercuryID>& b) const {
	return a.second > b.second;
    }
};
*/

/*
void BootstrapNode::StartProcessingBootReqs()
{
    if (m_ProcessingBootReqs) {
	return;
    }
    INFO << "Starting to process boot reqs" << endl;

    m_ProcessingBootReqs = true;

    ident_map_t *hub = m_IdentMap[m_HubInfoVec[0]->m_Name];

    vector<pair<IPEndPoint,MercuryID> > order;
    for (ident_map_t::iterator it = hub->begin(); it != hub->end(); it++) {
	order.push_back(*it);
    }
    stable_sort(order.begin(), order.end(), idpair_more());
    
    for (uint32 i=0; i<order.size(); i++) {
	MsgBootstrapRequest *req = m_PendingBootReqs[order[i].first];
	ASSERT(req != NULL);
	req->sender = order[i].first;
	m_ToProcessBootReqs.push_back(req);
    }

    ProcessNextBootReq();
}

void BootstrapNode::ProcessNextBootReq() {
    m_PendingJoin = SID_NONE;
    if (m_ToProcessBootReqs.size() > 0) {
	MsgBootstrapRequest *req = m_ToProcessBootReqs.front();
	m_PendingJoin = req->sender;
	m_ToProcessBootReqs.pop_front();
	HandleBootstrapRequest(&req->sender, req);
	m_Network->FreeMessage(req);
    }
}
*/

void BootstrapNode::HandleBootstrapRequestStatic(IPEndPoint * from,
						 MsgBootstrapRequest * bmsg)
{
    INFO << "Received bootstrap request form: " << from->ToString() << endl;

    MsgBootstrapResponse *resp =
	new MsgBootstrapResponse((byte) 0, m_Address);

    TimeVal now = m_Scheduler->TimeNow ();

    const vector<HubInitInfo *>& hubvec = m_StaticInitInfo[*from];
    if (hubvec.size() == 0) {
	WARN << from << " was not a member of any hub! dropping!" << endl;
	return;
    }

    for (uint32 i=0; i<hubvec.size(); i++) {
	HubInitInfo *newinfo = hubvec[i];
	BootstrapHubInfo *info = m_HubInfoVec[newinfo->ID];
	
	NodeState state (bmsg->sender, now);
	info->m_Nodelist.push_back (state);
	
	resp->AddHubInitInfo (newinfo); 
    }
    
    m_BootedServers.insert(bmsg->sender);
    
    DB(1) << "Sending response to: " << bmsg->sender << ": " << resp << endl;
    m_Network->SendMessage(resp, &(bmsg->sender), PROTO_TCP);
    delete resp;
}

//
// Figure out which hubs a node should join; construct
// BootstrapResponse Message and send it. 
//
void BootstrapNode::HandleBootstrapRequest(IPEndPoint * from,
					   MsgBootstrapRequest * bmsg)
{

    //
    // suppose no hub is populated - then, we must give ALL hubs to this guy
    // - register this guy as a member of all hubs in other words, go thru'
    // the list of hubs first. all hubs containing less than a threshold
    // #guys SHOULD take this new guy in. if no such hub exists, then select
    // a random/pseudo-random/deterministic *single* hub for this guy to
    // join.
    //
    // when do we ask the nodes to "split" the hubs? first, there will be
    // HUB_SPARSENESS_THRESHOLD nodes who will be doing routing for all the
    // hubs. then, as time progresses, as the number of live nodes in a hub
    // crosses a certain threshold, each node can decide which hub it is
    // going to stay on in and perform a "graceful" leave from that hub. For
    // now, this graceful leave is unneeded and our protocols/experiments
    // should run fine.
    //

    // XXX FIXME: 6/23/2004 Jeff: Because BootstrapRequests are unreliable
    // we could get duplicate messages. In addition, softstate failures
    // could result in this too. This will severely screw up the node state.

    // bootstrap requests are sent using TCP now...	but still, this is 
    // fucked up. - Ashwin 

    /*
    // HACK: retransmitted bs requests in this case should be ignored
    // xxx: should they always be ignored? what the heck is the correct
    // behavior? seems like there is no correct behavior.
    if (IDENT_MAP &&
	m_BootedServers.find(bmsg->sender) != m_BootedServers.end()) {
	WARN << "Ignoring duplicate bootstrap request from: " << *from << endl;
	return;
    }
    */

    INFO << "Received bootstrap request form: " << from->ToString() << endl;

    MsgBootstrapResponse *resp =
	new MsgBootstrapResponse((byte) 0, m_Address);

    uint32 hubSparenessThreshold = HUB_SPARSENESS_THRESHOLD;

    // Jeff: added debugging option
    if (g_BootstrapPreferences.oneNodePerHub)
	hubSparenessThreshold = 1;

    TimeVal now = m_Scheduler->TimeNow ();
    bool memberOfSomeHub = false;
    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	HubInitInfo newinfo;
	BootstrapHubInfo *info = m_HubInfoVec[i];

	// HACK: since we could get duplicate bootstrap requests, if we
	// get a new one, we have to remote the state for the old one
	NodeList & nodeList = info->m_Nodelist;
	for (NodeListIter it = nodeList.begin(); it != nodeList.end(); it++) {
	    if (it->GetAddress() == *from) {
		nodeList.erase(it);
		break;
	    }
	}

	newinfo.ID = i;
	newinfo.name = info->m_Name;
	newinfo.absmin = info->m_AbsMin;
	newinfo.absmax = info->m_AbsMax;
	newinfo.isMember = false;
	newinfo.rep = ChooseRepresentative (from, info);

	if (info->m_Nodelist.size() < hubSparenessThreshold) {	/* this hub is still sparse, new node MUST join it! */
	    newinfo.isMember = true;
	    memberOfSomeHub = true;

	    NodeState state (bmsg->sender, now);
	    //FixKnownRanges(state, newinfo, info);

	    // this is hacky here. but i dont have time to re-factor 
	    // this whole stupid code - Ashwin [03/19/2005]
	    if (!strcmp (g_BootstrapPreferences.choosingPolicy, "round-robin") || IDENT_MAP) {
		NodeListIter bit = info->m_Nodelist.begin ();
		if (bit != info->m_Nodelist.end ())
		    state.SetLevel (bit->GetLevel ());
		else
		    state.SetLevel (0);
	    }
	    info->m_Nodelist.push_back (state);
	}
	resp->AddHubInitInfo (&newinfo); 
    }

    if (!memberOfSomeHub && !g_BootstrapPreferences.oneNodePerHub) {
	int hub_index = 0;

	// XXX FIXME: REMOVE THIS FOR DEPLOYMENT, obviously :P
#if 0
	hub_index = (int) (m_HubInfoVec.size() * G_GetRandom());
#else
	/* easier for testing other stuff! otherwise, things become unpredictable! */
	static int last_hub_index = 0;
	hub_index = last_hub_index;
	last_hub_index = (last_hub_index + 1) % (m_HubInfoVec.size());
#endif
	BootstrapHubInfo *info = m_HubInfoVec[hub_index];

	resp->hubInfoVec[hub_index]->isMember = true;
	resp->hubInfoVec[hub_index]->rep = ChooseRepresentative (from, info);

	NodeState state (bmsg->sender, now);
	//FixKnownRanges(state, *resp->hubInfoVec[hub_index], info);

	// this is hacky here. but i dont have time to re-factor 
	// this whole stupid code - Ashwin [03/19/2005]
	if (!strcmp (g_BootstrapPreferences.choosingPolicy, "round-robin")) {
	    NodeListIter bit = info->m_Nodelist.begin ();
	    state.SetLevel (bit->GetLevel ());
	}
	info->m_Nodelist.push_back (state);
    }

    //CheckRingSanity();

    // XXX FIXME: This number never goes down when nodes fail...
    m_BootedServers.insert(bmsg->sender);

    DB(1) << "Received bootstrap request form: " << from->ToString() << endl;
    //	resp->Print(stderr);
    DB(1) << "Sending response to: " << bmsg->sender << ": " << resp << endl;
    m_Network->SendMessage(resp, &(bmsg->sender), PROTO_TCP);
    delete resp;

    // No reason to do this... we'll just open it up again for HBs
    //m_Network->CloseConnection(&(bmsg->sender), NetworkLayer::PROTO_UNRELIABLE);
}

//
// Main UDP message: heartbeat from nodes. they include their hub_name, their "standard" ip:port
// in that heartbeat. we update our m_HubInfoVec with this info. */
//
void BootstrapNode::HandleHeartbeat(IPEndPoint * from,
				    MsgHeartBeat * hmsg)
{
    NodeRange *range = &hmsg->GetRange ();
    TimeVal now = m_Scheduler->TimeNow ();

    BootstrapHubInfo *hinfo = GetHubInfoByID(hmsg->hubID);
    ASSERT(hinfo);

    DB(1) << "received heartbeat from " << from << " for hub: " << (int) hmsg->hubID << endl;
    // Add only if not present!

    /// this is ultra-inefficient; was I on crack? - Ashwin [03/19/2005]

    bool done = false;
    NodeList & nodeList = hinfo->m_Nodelist;
    for (NodeListIter it = nodeList.begin(); it != nodeList.end(); it++) {
	IPEndPoint addr = it->GetAddress ();
	
	if (addr == hmsg->sender) {
	    (*it).m_tLastHeartBeat = now;
	    (*it).SetRange(range);
	    (*it).SetIsJoined(); // HBs are only sent after node is ST_JOINED
	    done = true;
	    break;
	}
    }
    if (!done) {
	NodeState state = NodeState (hmsg->sender, now);
	state.SetRange (range);
	state.SetIsJoined();
	hinfo->m_Nodelist.push_back(state);
	
	if (hinfo->m_Nodelist.size() > HUB_FIFO_SIZE)
	    hinfo->m_Nodelist.pop_front();
    }

    /*
    if (IDENT_MAP) {
	// we are performing a lock-step join, so startup any pending joins
	if (*from == m_PendingJoin) {
	    INFO << "joined!: " << m_PendingJoin << endl;
	    ProcessNextBootReq();
	}
    }
    */
}

BootstrapHubInfo *BootstrapNode::GetHubInfoByID(byte hubID)
{
    for (int i = 0, len = m_HubInfoVec.size(); i < len; i++) {
	if (i == hubID) {
	    return m_HubInfoVec[i];
	}
    }
    return NULL;
}

void BootstrapNode::HandleEstimateRequest(IPEndPoint *from, MsgCB_EstimateReq *req)
{
    // are we supposed to maintain histograms?
    if (!g_BootstrapPreferences.haveHistograms) {
	WARN << " I am not supposed to maintain histograms; ignoring this request " << endl;
	return;
    }

    DB(1) << "received estimate request from: " << from->ToString() << endl;
    BootstrapHubInfo *hinfo = GetHubInfoByID(req->hubID);
    ASSERT(hinfo);

    MsgCB_EstimateResp *resp = new MsgCB_EstimateResp(req->hubID, m_Address);
    resp->hist = hinfo->m_Histogram;

    //INFO << resp->hist << endl;

    DB_DO(10) {
	DB (1) << resp << endl;
    }
    // Jeff: using tcp here because for reasonable sized networks, the
    // histogram is too big to fit in a udp packet. should fix that?
    int ret = m_Network->SendMessage(resp, &(req->sender), PROTO_TCP);
    if (ret < 0)
	WARN << "sending histo resp to " << req->sender << " failed!" << endl;
    delete resp;
}

OptionType g_BootstrapOptions[] =
{
    { '#', "histograms", OPT_NOARG | OPT_BOOL, "maintain histograms about hubs;",
      &(g_BootstrapPreferences.haveHistograms), "0",   (void *) "1"},
    { '#', "schema", OPT_STR, "schema file to read",
      g_BootstrapPreferences.schemaFile, "" , NULL }, 
    { '#', "policy", OPT_STR, "representative choosing policy",
      g_BootstrapPreferences.choosingPolicy, "round-robin" , NULL }, 
    { '#', "ident-map", OPT_STR, "forced node identity map file",
      g_BootstrapPreferences.identMapFile, "" , NULL }, 
    { '#', "buckets", OPT_INT, "number of buckets in the histogram",
      &g_BootstrapPreferences.nBuckets, "100" , NULL },
    { '#', "nservers", OPT_INT, "number of nodes to wait for in each hub before broadcasting \"all-joined\" signal (0 = never)",
      &g_BootstrapPreferences.nServers, "0" , NULL },
    { '#', "onenodehub", OPT_NOARG | OPT_BOOL, "one node per hub only! (debugging)",
      &(g_BootstrapPreferences.oneNodePerHub), "0",   (void *) "1"},
    { 0, 0, 0, 0, 0, 0, 0 }
};

class PeriodicTimer : public Timer {
    BootstrapNode *m_Node;

public:
    PeriodicTimer (BootstrapNode *node, u_long timeout) : Timer (timeout), m_Node (node) {}

    void OnTimeout () {
	m_Node->CleanupState ();
	_RescheduleTimer (1000);
    }
};

BootstrapNode::BootstrapNode (NetworkLayer *network, Scheduler *scheduler, IPEndPoint& address, char *schema_file) 
    : Node (network, scheduler, address), m_NumAssigned (0), m_AllJoined (false)
{
    FILE *fp = NULL;
    if ((fp = fopen(schema_file, "r")) == NULL) {
	Debug::die ("Error opening file %s for reading, terminating\n",
		    schema_file);
    }

    bool ok = ReadSchema(fp);
    fclose(fp);

    if (!ok)
	Debug::die ("Error reading/parsing the schema file, terminating\n");

    gmp_randinit_default (m_GMPRandState);
    INFO << "read schema file successfully..." << endl;

    if ( strcmp(g_BootstrapPreferences.identMapFile, "") ) {
	ASSERT(IDENT_MAP);
	
	ok = ReadIdentMap(g_BootstrapPreferences.identMapFile);

	if (!ok)
	    Debug::die("Error reading/parsing ident map file\n");
    }
}

Value BootstrapNode::GetRandom (const Value& max)
{
    Value ret = VALUE_NONE;
    mpz_urandomm ((MP_INT *) &ret, m_GMPRandState, &max);
    return ret;
}

struct cmp_ns_range_t { 
    bool operator () (const NodeState* sa, const NodeState* sb) const {
	return sa->GetRange ()->GetMin () < sb->GetRange ()->GetMin ();
    }
};

void BootstrapNode::CheckRingSanity ()
{
    if (!m_AllJoined)
	return;

    for (int i = 0, len = m_HubInfoVec.size (); i < len; i++) {
	BootstrapHubInfo *hinfo = m_HubInfoVec[i];

	NodeList& nodelist = hinfo->m_Nodelist;
	list<NodeState *> nlist;
	for (NodeList::iterator it = nodelist.begin (); it != nodelist.end (); ++it) {
	    ASSERT((*it).GetRange() != NULL);
	    nlist.push_back (&(*it));
	}

	nlist.sort (cmp_ns_range_t ());
	//		sort (nlist.begin (), nlist.end (), cmp_ns_range_t ());

        DB(-1) << "sorted ranges for joined nodes for hub=" << hinfo->m_Name << endl;
	for (list<NodeState *>::iterator it = nlist.begin (); it != nlist.end (); ++it) {
	    DB(-1) << (*it)->GetAddress () << " " << (*it)->GetRange () << endl;
	}

	for (list<NodeState *>::iterator it = nlist.begin (); it != nlist.end (); ++it) {
	    list<NodeState *>::iterator nit (it);
	    ++nit;

	    // wrap around
	    if (nit == nlist.end ()) 
		nit = nlist.begin ();

	    if (it == nit) 
		continue;

	    NodeRange *cur = (*it)->GetRange(), *nxt = (*nit)->GetRange();
	    if ((cur->GetMax () == hinfo->m_AbsMax && nxt->GetMin () == hinfo->m_AbsMin)
		|| (cur->GetMax () == nxt->GetMin ())) {
		// okay
	    }
	    else {
		WARN << "ranges do NOT abut: " << cur << " and " << nxt 
		     << " (" << (*it)->GetAddress() << "," 
		     << (*nit)->GetAddress() << ")" << endl;
	    }
	}
	DB(-1) << "==========================================================" << endl;
    }
}

void BootstrapNode::Start ()
{
    ref<UpdateHistoTimer> u = new refcounted<UpdateHistoTimer>(this, 0);
    m_Scheduler->RaiseEvent (u, m_Address, u->GetNextDelay ());

    ref<PeriodicTimer> p = new refcounted<PeriodicTimer>(this, 0);
    m_Scheduler->RaiseEvent (p, m_Address, p->GetNextDelay ());

    if (g_BootstrapPreferences.nServers > 0) {
	ref<CheckAllJoinedTimer> c = new refcounted<CheckAllJoinedTimer>(this, 200);
	m_Scheduler->RaiseEvent (c, m_Address, c->GetNextDelay ());

	m_Scheduler->RaiseEvent (new refcounted<CheckRingSanityTimer> (this, 5000), m_Address, 5000);
    }
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
