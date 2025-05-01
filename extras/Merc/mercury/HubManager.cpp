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

#include <mercury/common.h>
#include <mercury/HubManager.h>
#include <mercury/Peer.h>
#include <mercury/Message.h>
#include <mercury/Histogram.h>
#include <mercury/PubsubData.h>
#include <mercury/Parameters.h>
#include <mercury/Utils.h>
#include <mercury/ObjectLogs.h>   // XXX
#include <mercury/LinkMaintainer.h>
#include <mercury/PubsubRouter.h>
#include <mercury/BufferManager.h>
#include <mercury/MercuryNode.h>
#include <mercury/Scheduler.h>

class BootstrapRequestTimer : public Timer {
public:
    HubManager    *m_HubManager;

    BootstrapRequestTimer(HubManager *hm, int timeout);
    void OnTimeout();
};

class BootstrapHeartbeatTimer : public Timer {
public:
    HubManager    *m_HubManager;

    BootstrapHeartbeatTimer(HubManager *hm, int timeout);
    void OnTimeout();
};

///////////////////////// ///  BootstrapRequestTimer //////////////////
//
BootstrapRequestTimer::BootstrapRequestTimer(HubManager *hm, int timeout)  : Timer(timeout)
{
    m_HubManager = hm;
}

void BootstrapRequestTimer::OnTimeout()
{
    m_NTimeouts++;
    if (m_NTimeouts > Parameters::MaxBootstrapRequestAttempts) 
	Debug::die ("bootstrap did not respond! could not contact the bootstrap node...\n");

    MsgBootstrapRequest bmsg((byte) 0, m_HubManager->GetAddress ());

    if (m_HubManager->m_Network->SendMessage(&bmsg, &m_HubManager->GetBootstrapIP (), PROTO_TCP) < 0)
	Debug::die ("Could not send message to bootstrap node...\n");

    _RescheduleTimer(Parameters::BootstrapRequestTimeout);
}

/////////////////////////////   BootstrapHeartbeatTimer  ////////////////////////
//
BootstrapHeartbeatTimer::BootstrapHeartbeatTimer(HubManager *hm, int timeout) : Timer(timeout)
{
    m_HubManager = hm;
}

void BootstrapHeartbeatTimer::OnTimeout()
{
    MsgHeartBeat *hbeat;

    HubManager *hm = m_HubManager;
    for (int i = 0, len = hm->GetNumHubs(); i < len; i++) {
	Hub *h = hm->GetHubByIndex(i);

	if (!h->IsMine())
	    continue;

	MemberHub *hub = (MemberHub *) h;
	// In the case of a static join, we want to send HBs before
	// we are technically joined (since that only happens when
	// everyone has gotten a bs response from the bootstrap)
	if (hub->GetStatus() != ST_JOINED && 
	    !(hub->StaticJoin() && 
	      !m_HubManager->m_MercuryNode->AllJoined())) {
	    continue;
	}

	hbeat = new MsgHeartBeat(hub->GetID(), hm->GetAddress(), *hub->GetRange());
	m_HubManager->m_Network->SendMessage(hbeat, &hm->GetBootstrapIP (), Parameters::TransportProto);
	delete hbeat;
    }

    _RescheduleTimer(Parameters::BootstrapHeartbeatInterval);
}

HubManager::HubManager(MercuryNode *mnode, BufferManager *bufferManager)
    : 
    m_BootstrapRequestTimer (new refcounted<BootstrapRequestTimer> (this, 0)), 
    m_BootstrapHeartbeatTimer (new refcounted<BootstrapHeartbeatTimer> (this, 0)), 
    m_MercuryNode (mnode), 
    m_Network (mnode->GetNetwork ()), 
    m_Scheduler (mnode->GetScheduler ()), 
    m_Address (mnode->GetAddress ()), m_BufferManager (bufferManager)
{
    m_MercuryNode->RegisterMessageHandler(MSG_BOOTSTRAP_RESPONSE, this);
}

void HubManager::BootstrapUsingServer(char *bootstrap)
{
    m_BootstrapIP = IPEndPoint(bootstrap);

    // msg ("bootstrap server here =%s\n", m_BootstrapIP.ToString ());
    if (m_BootstrapIP == SID_NONE)
	Debug::die ("Bad bootstrap server argument: %s\n", bootstrap);

    m_Scheduler->RaiseEvent (m_BootstrapRequestTimer, m_Address, m_BootstrapRequestTimer->GetNextDelay ());	
    m_Scheduler->RaiseEvent (m_BootstrapHeartbeatTimer, m_Address, m_BootstrapHeartbeatTimer->GetNextDelay ());	
}

template <char cp>
class char_sep : public unary_function<char, bool>
{
public:
    bool operator() (char c) const { return c == cp; }
};

void HubManager::PerformBootstrap()
{
    char *bootstrap = g_Preferences.bootstrap;

    if (bootstrap[0] != '\0') {
	BootstrapUsingServer(bootstrap);
	return;
    }
    
    if (!g_Preferences.schema_string || !g_Preferences.join_locations) {
	Debug::die("either --bootstrap or [--schema-str and --join-locations] must be given");
    }
    uint32 n_member_hubs = 0;
	
    // process schema
    vector<HubInitInfo *> hv;
    vector<string> hs;
    tokenizer<comma_sep>::tokenize(hs, g_Preferences.schema_string);
    
    for (int i = 0, n = hs.size(); i < n; i++) {
	vector<string> inf;
	tokenizer<char_sep<':'> >::tokenize(inf, hs[i]);
	if (inf.size() != 4) { 
	    Debug::die("wrong schema specification (%s), must be [name:min:max:is_member]", hs[i].c_str());
	}	
	DB(1) << " name=" << inf[0] << " min=" << inf[1] << " max=" << inf[2] << " is_member=" << inf[3] << endl;
	
	HubInitInfo *hif = new HubInitInfo();
	hif->ID = i;
	hif->name = inf[0];
	hif->absmin = Value(inf[1].c_str());
	hif->absmax = Value(inf[2].c_str());
	hif->isMember = (inf[3] == "true" ? true : false);
	if (hif->isMember) 
	    n_member_hubs++;
	
	hv.push_back(hif);
    }
    
    // process join-locations
    vector<string> js;
    tokenizer<comma_sep>::tokenize(js, g_Preferences.join_locations);
    if (js.size() != n_member_hubs) {
	Debug::die("schema-str (%s) has (%d) member hubs, but join locations are given only for (%d) hubs",
		   g_Preferences.schema_string, n_member_hubs, js.size());
    }
    
    for (uint32 i = 0; i < js.size(); i++) {
	vector<string> inf;
	tokenizer<char_sep<':'> >::tokenize(inf, js[i]);
	if (inf.size() != 3) {
	    Debug::die("wrong specification for join location (%s), must be [name:ip:port]", js[i].c_str());
	}
	for (uint32 j = 0; j < hv.size(); j++) { 
	    if (hv[j]->name != inf[0])
		continue;
	    string ipport = inf[1] + ":" + inf[2];
	    hv[j]->rep = IPEndPoint((char *) ipport.c_str());
	}
    }
    
    RegisterHubInfo(hv);
    for (int i = 0, n = hv.size(); i < n; i++)
	delete hv[i];
    
    StartJoin();
}

void HubManager::ProcessMessage(IPEndPoint *from, Message *msg)
{
    if (msg->GetType () == MSG_BOOTSTRAP_RESPONSE)
	HandleBootstrapResponse(from, (MsgBootstrapResponse *) msg);
    else
	MWARN << merc_va("HubManager:: received some idiotic message [%s]", msg->TypeString()) << endl;
}

void HubManager::HandleBootstrapResponse(IPEndPoint * from, MsgBootstrapResponse * bmsg)
{
    MDB (1) << " received bootstrap response " << endl;
    if (GetNumHubs() > 0) { // i had received a bootstrap message earlier; throw this away!
	MWARN << "Got duplicate bootstrap message; already have " << GetNumHubs() << " hubs" << endl; 
	return;
    }
    RegisterHubInfo(bmsg->hubInfoVec);
    m_BootstrapRequestTimer->Cancel();
    StartJoin();
}

void HubManager::StartJoin()
{
    m_MercuryNode->SetStartTime (m_Scheduler->TimeNow ());

    for (int i = 0, len = GetNumHubs(); i < len; i++) {
	Hub *h = m_HubVec[i];
	if (!h->IsMine())
	{
	    MDB(10) << "hub " << h->GetName() << " is not mine. no join process to perform!" << endl;
	    continue;
	}

	MemberHub *hub = (MemberHub *) h;

	if (hub->StaticJoin()) {
	    // on static join, bootstrap tells us our initial ring config
	    hub->SetRange(hub->m_Initinfo.range);
	    hub->SetSuccessorList(&hub->m_Initinfo.succs);
	    for (PeerInfoList::iterator it = hub->m_Initinfo.preds.begin();
		 it != hub->m_Initinfo.preds.end(); it++) {
		hub->AddPredecessor(it->GetAddress(), it->GetRange());
	    }
	    hub->StartBootstrapHbeater();
	    continue;
	}

	IPEndPoint hubRep = hub->GetBootstrapRep();

	if (hubRep == SID_NONE)
	{
	    // I am the first person to join the hub!!

	    MDB(10) << ": setting complete range=" << hub->GetAbsMin() << "->" 
		    << hub->GetAbsMax () << endl;

	    NodeRange r = NodeRange(hub->GetID(), hub->GetAbsMin(), hub->GetAbsMax ());
	    hub->SetRange (&r);

	    // hub->SetPredecessor (m_Address, r);
	    hub->SetSuccessor (m_Address, r);

	    TINFO << "one-node-loop! current range=" << hub->GetRange () << endl;
	    m_MercuryNode->GetApplication ()->JoinBegin (m_Address);
	    hub->OnJoinComplete();
	} 
	else 
	{
	    TINFO << "starting join; hubrep is " << hubRep << endl;
	    hub->StartJoin();
	    //
	    // now, we will receive the join response when we process our packets from our
	    // successor ... if an error occured, we can handle it there
	    // and, now we have a successor, our predecessor will connect to us
	    // when he receives the UPDATE_SUCCESSOR from our successor
	    //
	}
    }
}

AttrInfo *g_MercuryAttrRegistry = NULL;
int g_NumHubs = 0;

// Gain some knowledge about what other hubs exist in my system.
// And, create a registry of information about the various attributes
// This includes [attrindex, type, name]

void HubManager::RegisterHubInfo(vector<HubInitInfo *> &infovec)
{
    Hub *hub;
    int nhubs = infovec.size();

    ASSERT(nhubs > 0);

    g_MercuryAttrRegistry = new AttrInfo[nhubs];
    g_NumHubs = nhubs;

    for (int i = 0; i < nhubs; i++) {
	HubInitInfo *info = infovec[i];

	g_MercuryAttrRegistry[i].index = i;
	g_MercuryAttrRegistry[i].name  = info->name;

	if (info->isMember) {
	    hub = new MemberHub(m_MercuryNode, m_BootstrapIP, m_BufferManager, *info);
	}
	else {
	    ASSERT(info->rep.GetIP() != 0);
	    hub = new NonMemberHub(m_MercuryNode, m_BufferManager, *info);
	}
	m_HubVec.push_back(hub);
    }
}

bool HubManager::IsJoined()
{
    int len = m_HubVec.size();

    if (len == 0)
	return false;

    for (int i = 0; i < len; i++) {
	Hub *hub = m_HubVec[i];

	if (!hub->IsMine())
	    continue;
	if (((MemberHub *) hub)->GetStatus() != ST_JOINED)
	    return false;
    }

    return true;
}

void HubManager::Shutdown()
{
    for (int i = 0, len = m_HubVec.size(); i < len; i++) {
	if (m_HubVec[i]->IsMine())
	{
	    ((MemberHub *) m_HubVec[i])->PrepareLeave();
	}
    }
}

Hub *HubManager::GetHubByID(byte id)
{
    for (int i = 0, len = m_HubVec.size(); i < len; i++) {
	if (m_HubVec[i]->GetID() == id)
	    return m_HubVec[i];
    }
    return 0;
}

void HubManager::Print(FILE * stream)
{
    for (int i = 0, len = m_HubVec.size(); i < len; i++) {
	// m_HubVec[i]->Print(stream);
	if (m_HubVec[i]->IsMine ()) {
	    cerr << (MemberHub *) m_HubVec[i];
	}
    }
}

//
// this is the first place the pub comes from the app. Now, we need to
// send this to all attribute hubs.
//
void HubManager::SendAppPublication(MsgPublication * pmsg)
{
    ///// MEASUREMENT
    uint32 orig_nonce = pmsg->GetEvent ()->GetNonce();
    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	DiscoveryLatEntry ent(DiscoveryLatEntry::PUB_SEND, 0, orig_nonce);
	LOG(DiscoveryLatLog, ent);
    }
    ///// MEASUREMENT

    MDB (1) << "routing publication " << pmsg << endl;

    for (int i = 0, len = m_HubVec.size(); i < len; i++) {
	Hub *hub = m_HubVec[i];

	if (pmsg->GetEvent ()->GetConstraintByAttr (hub->GetID()) == NULL)
	    continue;

	MsgPublication *npmsg = pmsg->Clone ();

	///// MEASUREMENT
	if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	    // one pub becomes multiple, so we alias them
	    uint32 new_nonce = CreateNonce();
	    DiscoveryLatEntry ent(DiscoveryLatEntry::ALIAS, 
				  0, orig_nonce, new_nonce);
	    LOG(DiscoveryLatLog, ent);
	    npmsg->GetEvent ()->SetNonce(new_nonce);
	}
	///// MEASUREMENT

	npmsg->hubID = hub->GetID();

	if (hub->IsMine()) 
	{
	    ((MemberHub *) hub)->HandleMercPub(0, npmsg);
	} 
	else 
	{
	    // FIXME: utilize the hub cache here.
	    NonMemberHub *nhub = (NonMemberHub *) hub;
	    IPEndPoint *rep = nhub->GetRepAddress();

	    m_Network->SendMessage(npmsg, rep, Parameters::TransportProto);
	}

	delete npmsg;
    }
}

//
// this is the first place a subscription comes from the app. Now, we need to
// send this to one random attribute hub.
//
void HubManager::SendAppSubscription(MsgSubscription * smsg)
{
    Interest *sub = smsg->GetInterest();
    int len = m_HubVec.size();

    vector < int >indexes;

    if (len == 0) {
	DB_DO (10) { MWARN << "Ignoring attempt to subscribe when no hubs were present.\n"; }
	return;
    }

    MDB (10) << " :::::::::: sending app subscription " << smsg << endl;

    /* fish out { sub-attributes \intersect hub_names } */
    for (int i = 0; i < len; i++) {
	Hub *hub = m_HubVec[i];

	for (Interest::iterator it = sub->begin (); it != sub->end (); it++) {
	    Constraint *cst = *it; 

	    if (cst->GetAttrIndex() == hub->GetID()) {
		indexes.push_back(i);
		break;
	    }
	}
    }

    if (indexes.size() == 0) {
	MWARN << "No hub attributes in sub! Dropping!\n";
	return;
    }

    //////////////////////////////////////////////////////////////////////////
    // TODO: This is where subscription selectivity can be done using histograms
    int r = (int) ((float) indexes.size() * G_GetRandom());

    ///// MEASUREMENT
    if (g_MeasurementParams.enabled /* && !g_MeasurementParams.aggregateLog */) {
	DiscoveryLatEntry ent(DiscoveryLatEntry::SUB_SEND, 
			      0, sub->GetNonce());
	LOG(DiscoveryLatLog, ent);
    }
    ///// MEASUREMENT

    Hub *rand_hub = m_HubVec[indexes[r]];
    smsg->hubID = rand_hub->GetID();
    if (rand_hub->IsMine()) {
	((MemberHub *) rand_hub)->HandleMercSub(0, smsg);
	return;
    }

    NonMemberHub *nhub = (NonMemberHub *) rand_hub;
    m_Network->SendMessage(smsg, nhub->GetRepAddress(), Parameters::TransportProto);
}

ostream& operator<<(ostream& out, HubManager *hm)
{
    out << "(HubManager:" << endl;
    for (int i = 0, len = hm->m_HubVec.size(); i < len; i++) {
	out << "  " << hm->m_HubVec[i] << endl; 
    }
    out << ")";
    return out;
}

void HubManager::PrintPublicationList(ostream& stream)
{
    for (int i = 0, len = m_HubVec.size(); i < len; i++) {
	Hub *h = m_HubVec[i];

	if (!h->IsMine())
	    continue;

	MemberHub *hub = (MemberHub *) h;
	hub->m_PubsubRouter->PrintPublicationList(stream);
    }
}

void HubManager::PrintSubscriptionList(ostream& stream)
{
    for (int i = 0, len = m_HubVec.size(); i < len; i++) {
	Hub *h = m_HubVec[i];

	if (!h->IsMine())
	    continue;

	MemberHub *hub = (MemberHub *) h;
	hub->m_PubsubRouter->PrintSubscriptionList(stream);
    }
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
