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

  test.cpp

begin		   : November, 2003
copyright	   : (C) 2003-2004 Ashwin Bharambe    (ashu@cs.cmu.edu)

***************************************************************************/

#include <pthread.h>
#include <cstdlib>
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

#include <wan-env/RealNet.h>
// om/ because we do testing for tags as well!
#include <om/OMEvent.h>
#include <om/OMMessage.h>
#include <om/OMInterest.h>

#include <mercury/Parameters.h>
#include <mercury/HubManager.h>
#include <mercury/Hub.h>
#include <mercury/PubsubRouter.h>
#include <Mercury.h>
#include <wan-env/WANMercuryNode.h>
#include <util/ExpLog.h>

typedef map<string, guid_t> StrGUIDMap;
typedef StrGUIDMap::iterator StrGUIDMapIter;
typedef map<guid_t, string, less_GUID> StringMap;
typedef StringMap::iterator StringMapIter;

void InitApp();
void RunApp();
int HandleInput(FILE *file, struct timeval *now, bool echoInput = false);
void handleHelp(char **args);
void handlePublish(char **args);
void handleTrigger(char **args);
void handleGuidTrigger(char **args);
void handleRangePublish(char **args);
void handleSubscribe(char **args);
void handleGetSubs(char **args);
void handleDumpRouteTable(char **args);
void handleGetRange(char **args);
void handleGetAllRanges(char **args);
void handleIdle(char **args);

guid_t parse_guid(char *str);
sid_t parse_sid(char *str);
char *extract_str(char *buf, int *len);
char **extract_args(char *buf);
void CheckTimeouts(struct timeval *now);
void Cleanup(int sig);

extern char g_ProgramName[];
static bool g_NoColorize;
static bool g_NoUseReadLine;
static char g_Script[512];

///////////////////////////////////////////////////////////////////////////////

OptionType g_AppOptions[] = {
    {'R', "noreadline", OPT_BOOL | OPT_NOARG,
     "don't use GNU readline for input", 
     &(g_NoUseReadLine), "0", (void *)"1"},
    {'C', "nocolor", OPT_BOOL | OPT_NOARG,
     "don't colorize output", 
     &(g_NoColorize), "0", (void *)"1"},
    {'S', "script", OPT_STR,
     "run a series of scripted commands", g_Script, "", NULL},
    {0, 0, 0, 0, 0, 0, 0}
};

typedef struct {
    const char *cmd;
    const char *abbrev;
    void (*handler)(char **args);
    const char *usage;
    const char *help;
} InputHandler;

static InputHandler g_Handlers[] = {
    { "HELP",   "H",  handleHelp,   "",	"Show this help message" },
    { "PUBLISH",   "P", handlePublish,   "", "Call MercuryNode::SendEvent()" },
    { "RANGE_PUBLISH",   "/", handleRangePublish,   "", "Call MercuryNode::SendEvent() with a range publication" },
    { "TRIGGER",   "T", handleTrigger,   "" , "publish a trigger; MercuryNode::SendEvent() with a TTL" },
    { "GUID_TRIGGER",   "/", handleGuidTrigger,   "" , "publish a trigger with a guid given; MercuryNode::SendEvent() with a TTL" },
    { "SUBSCRIBE",   "S", handleSubscribe,   "", "Call MercuryNode::RegisterInterest()" },
    { "GETSUBS",   "G", handleGetSubs,   "", "Get current subscriptions at a node" },
    { "DUMP_RT",   "D", handleDumpRouteTable,   "", "Get the routing table of the Mercury node" },
    { "GETRANGE",   "R", handleGetRange,   "", "Get the range corresponding to an attribute" },
    { "GET_ALLRANGES",   "A", handleGetAllRanges,   "", "Get all ranges for member hubs" },
    { "IDLE", "I",  handleIdle, "", "call MercuryNode::Idle()" },
    { NULL, NULL, NULL, NULL }
};

static WANMercuryNode *g_WANMercNode;
static StrGUIDMap  g_Names;
static StringMap g_GUIDs;

static pthread_mutex_t g_ManagerLock;

#define NOT_ENOUGH_ARGS(x) cout << "ERROR: need " << x << " arguments" << endl

///////////////////////////////////////////////////////////////////////////////

void InitApp()
{
    // init rand seed
    srand((int) getpid());

    // much smaller timeout values because we are running these over one machine
    Parameters::BootstrapRequestTimeout = 400;
    Parameters::SuccessorMaintenanceTimeout  = 100;                 // time to wait before checking on the successor
    Parameters::JoinRequestTimeout = 400;
    Parameters::PeerPingInterval = 500;
    Parameters::PeerPongTimeout = 300000;
    Parameters::BootstrapHeartbeatInterval = 500;
    Parameters::BootstrapHeartbeatTimeout  = 1800;
    Parameters::BootstrapUpdateHistInterval = 100;
    Parameters::BootstrapSamplingInterval = 500;
    Parameters::KickOldPeersTimeout = 2000;

    g_WANMercNode = WANMercuryNode::GetInstance(g_Preferences.port);
    g_WANMercNode->FireUp();
}

void RunApp()
{
    struct timeval now;

    while(true) {
	gettimeofday(&now, NULL);
	if ( HandleInput(stdin, &now) < 0 ) {
	    Debug::warn("stdin was closed!");
	    Cleanup(0);
	}
	cout.flush();
	fflush(stdout);
    }	
}

static int _localOID = 1;
void handlePublish(char **args)
{
    g_WANMercNode->DoWork (100);
    if (!args[1]) {
	cout << "ERROR: missing publication" << endl;
	return;
    }

    IPEndPoint ipport = g_WANMercNode->GetAddress ();
    guid_t g( ipport.GetIP (), ipport.GetPort (), _localOID++);
    OMEvent *ev = new OMEvent(g, SID_NONE);

    int index = 1;

    while (1) {
	if (! args[index]) {
	    break;
	}
	if (! args[index+1]) {
	    cout << "ERROR: missing value for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}

	int attr_index = atoi (args[index]);
	Value val (args[index + 1], 10);

	Tuple t (attr_index, val);
	ev->AddTuple (t);

	index += 2;
    }
    g_WANMercNode->SendEvent(ev);
    cout << "OK: Publish() called guid=" << g << endl;
    return;    // don't delete; mercury will...
 error:
    delete ev;
    return;
}

void handleRangePublish(char **args)
{
    g_WANMercNode->DoWork (100);
    if (!args[1]) {
	cout << "ERROR: missing publication" << endl;
	return;
    }

    IPEndPoint ipport = g_WANMercNode->GetAddress ();
    guid_t g( ipport.GetIP (), ipport.GetPort (), _localOID++);
    OMEvent *ev = new OMEvent(g, SID_NONE);
    int index = 1;

    while (1) {
	if (! args[index]) {
	    break;
	}
	if (! args[index+1]) {
	    cout << "ERROR: missing constraint min for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}
	if (! args[index+2]) {
	    cout << "ERROR: missing constraint max for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}

	int attr_index = atoi (args[index]);
	Value min (args[index + 1], 10);
	Value max (args[index + 2], 10);

	Constraint c (attr_index, min, max);
	ev->AddConstraint (c);
	index += 3;
    }
    g_WANMercNode->SendEvent(ev);
    cout << "OK: RangePublish() called guid=" << g << endl;
    return;    // don't delete; mercury will...
 error:
    delete ev;
    return;
}

// Almost a copy of handlePublish() ; perhaps I should re-factor, but
// what the heck!
void handleTrigger(char **args)
{
    g_WANMercNode->DoWork (100);
    if (!args[1]) {
	cout << "ERROR: missing TTL" << endl;
	return;
    }

    if (!args[2]) {
	cout << "ERROR: missing publication" << endl;
	return;
    }

    IPEndPoint ipport = g_WANMercNode->GetAddress ();
    guid_t g( ipport.GetIP (), ipport.GetPort (), _localOID++);
    OMEvent *ev = new OMEvent(g, SID_NONE);

    ev->SetLifeTime(atoi(args[1]) * 1000);
    int index = 2;

    while (1) {
	if (! args[index]) {
	    break;
	}
	if (! args[index+1]) {
	    cout << "ERROR: missing value for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}

	int attr_index = atoi (args[index]);
	Value val (args[index + 1], 10);
	Tuple t (attr_index, val);

	ev->AddTuple (t);
	index += 2;
    }
    g_WANMercNode->SendEvent(ev);
    cout << "OK: Trigger() called guid=" << g << endl;
    return;    // don't delete; mercury will...
 error:
    delete ev;
    return;
}

// Yet another copy of handlePublish() ; perhaps I should re-factor, but
// what the heck!
void handleGuidTrigger(char **args)
{
    g_WANMercNode->DoWork (100);
    if (!args[1]) {
	cout << "ERROR: missing guid" << endl;
	return;
    }
    guid_t guid = parse_guid(args[1]);
    if (guid == GUID_NONE) {
	cout << "ERROR: invalid guid=" << args[1] << endl;
	return;
    }

    if (!args[2]) {
	cout << "ERROR: missing seqnum" << endl;
	return;
    }

    if (!args[3]) {
	cout << "ERROR: missing TTL" << endl;
	return;
    }


    if (!args[4]) {
	cout << "ERROR: missing publication" << endl;
	return;
    }

    OMEvent *ev = new OMEvent(guid, sid_t((unsigned int) 0, 0));
    ev->SetSeqno (atoi(args[2]));
    ev->SetLifeTime (atoi(args[3]) * 1000);
    int index = 4;

    while (1) {
	if (! args[index]) {
	    break;
	}
	if (! args[index+1]) {
	    cout << "ERROR: missing value for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}

	int attr_index = atoi (args[index]);
	Value val (args[index + 1], 10);
	Tuple t (attr_index, val);

	ev->AddTuple (t);
	index += 2;
    }
    g_WANMercNode->SendEvent(ev);
    cout << "OK: GuidTrigger() called guid=" << guid << endl;
    return;    // don't delete; mercury will...
 error:
    delete ev;
    return;
}
void handleSubscribe(char **args)
{
    g_WANMercNode->DoWork (100);
    if (!args[1]) {
	cout << "ERROR: missing subscription" << endl;
	return;
    }

    IPEndPoint ipport = g_WANMercNode->GetAddress ();
    guid_t g( ipport.GetIP (), ipport.GetPort (), _localOID++);
    cout << g << endl;
    OMInterest *in = new OMInterest(g);
    int index = 1;

    while (1) {
	if (! args[index]) {
	    break;
	}
	if (args[index] && !strcmp(args[index], "tag")) {
	    if (!args[index + 1]) {
		cout << "ERROR: missing tag" << endl;
		return;
	    }

	    guid_t tag = parse_guid(args[index + 1]);
	    if (tag == GUID_NONE) {
		cout << "ERROR: invalid guid=" << args[index + 1] << endl;
		return;
	    }
	    in->AddTag(tag);
	    index += 2;
	    continue;
	}
	if (! args[index+1]) {
	    cout << "ERROR: missing constraint min for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}
	if (! args[index+2]) {
	    cout << "ERROR: missing constraint max for attribute key=" 
		 << args[index] << endl;
	    goto error;
	}

	int attr_index = atoi (args[index]);
	Value min (args[index + 1], 10);
	Value max (args[index + 2], 10);

	Constraint c (attr_index, min, max);
	in->AddConstraint (c);
	index +=3;
    }
    g_WANMercNode->RegisterInterest(in);
    cout << "OK: Subscribe() called; guid=" << g << endl;
    return;    // don't delete; mercury will...
 error:
    delete in;
    return;
}

void handleGetSubs(char **args)
{
    cout << "OK: GetSubs() called" << endl;

    /* -- DISABLED FOR NOW
       HubManager *hm = g_WANMercNode->GetHubManager();
       int nhubs = hm->GetNumHubs();

       cout << "OK: GetSubs() called" << endl;
       //	cout << "nhubs " << nhubs << endl;
       for (int i = 0; i < nhubs; i++) {
       InterestList *int_list = hm->GetHubByIndex(i)->GetSubList();

       cout << "hub " << hm->GetHubByIndex(i)->GetName() << " ::";
       for (InterestListIter it = int_list->begin(); it != int_list->end(); it++) 
       {
       if (it != int_list->begin()) 
       cout << "##";
       Interest *in = *it;
       cout << in;
       }
       cout << "::" << endl;
       }
    */
}

void handleDumpRouteTable(char **args)
{
    HubManager *hm = g_WANMercNode->GetHubManager();
    int nhubs = hm->GetNumHubs();

    cout << "OK: DUMP_RT() called" << endl;
    // cout << "nhubs " << nhubs << endl;
    for (int i = 0; i < nhubs; i++) {
	Hub *h = hm->GetHubByIndex(i);
	if (!h->IsMine())
	    continue;

	MemberHub *hub = (MemberHub *) h;
	cout << "hub " << hub->GetName() << " ";
	/*
	  for (MemberHub::iterator it = hub->begin(); it != hub->end(); it++, i++)
	  {
	  if (it != hub->begin()) cout << ",";
	  Peer *peer = *it;
	  if (peer)
	  cout << peer;
	  else
	  cout << "nopeer";
	  }
	*/
	cout << endl;
    }
}

void handleGetRange(char **args) 
{
    if (!args[1]) {
	cout << "ERROR: handleGetRange called without an attribute" << endl;
	return;
    }

    DB(1) << "received argument " << args[1] << endl;
    HubManager *hm = g_WANMercNode->GetHubManager();
    int nhubs = hm->GetNumHubs();
    for (int i = 0; i < nhubs; i++) {
	Hub *h = hm->GetHubByIndex(i);

	DB(1) << " checking argument " << h->GetName() << endl;
	if (strcmp(h->GetName().c_str(), args[1]) != 0) 
	    continue;

	if (!h->IsMine())
	    continue;

	MemberHub *hub = (MemberHub *) h;
	NodeRange *range = hub->GetRange();
	if (range)
	    cout << "OK: " << range << endl;
	else
	    cout << "OK: not joined null range" << endl;
	return;
    }
    cout << "ERROR: handleGetRange attribute not found" << endl;
    return;
}

void handleGetAllRanges(char **args) 
{
    HubManager *hm = g_WANMercNode->GetHubManager();
    int nhubs = hm->GetNumHubs();
    for (int i = 0; i < nhubs; i++) {
	Hub *h = hm->GetHubByIndex(i);

	if (!h->IsMine())
	    continue;

	MemberHub *hub = (MemberHub *) h;

	NodeRange *range = hub->GetRange();
	if (range)
	    cout << "OK: " << range << " abs_min=" << hub->GetAbsMin () << " abs_max=" << hub->GetAbsMax () << endl;
	else
	    cout << "OK: not joined null range " << hub->GetName() << endl;
    }
    return;
}

#if 0
void handleGetRecentPubs(char **args)
{
}
#endif

void handleIdle(char **args)
{
#ifndef ENABLE_REALNET_THREAD
    RealNet::DoWork();
#endif
    g_WANMercNode->DoWork (100);
#ifndef ENABLE_REALNET_THREAD
    RealNet::DoWork();
#endif
    cout << "OK: DoWork() called" << endl;
}

int HandleInput(FILE *in /* only works without readline! */, 
		struct timeval *now,
		bool echoInput)
{
    char _line_buf[512];
    bool free_cmdline = false; // HACK!
    char *cmdline;

    if (!g_NoUseReadLine) {
	HISTORY_STATE *state;

	cmdline = readline("> ");

	if (cmdline == NULL) {
	    return -1;
	}

	state = history_get_history_state();
	if (state->length > 1000) {
	    HIST_ENTRY *ent = remove_history(0);
	    free(ent->line);
	    free(ent);
	}
	HIST_ENTRY *last = history_get(MAX(history_base + state->length-1, 0));
	if (last == NULL || strcmp(cmdline, last->line)) {
	    add_history(cmdline);
	} else {
	    free_cmdline = true;
	}
    } else {
	errno = 0;
	if ( fgets(_line_buf, 512, in) == NULL ) {
	    if (errno) {
		Debug::warn("fgets error on input: %s", strerror(errno));
	    }
	    return -1;
	}
	_line_buf[strlen(_line_buf)-1] = '\0'; // remove trailing newline
	cmdline = _line_buf;
    }

    if (echoInput)
	cout << "> " << cmdline << endl;
    DBG << "> " << cmdline << endl;

    char **args = extract_args(cmdline);

    cout << (!g_NoColorize?"[31m":"") 
	 << "==BEGIN_RESP" 
	 << (!g_NoColorize?"[m":"")  << endl;
    if (args[0] == '\0') {
	cout << "ERROR: no command provided" << endl;
    } else {
	bool done = false;

	for (int i=0; g_Handlers[i].cmd != NULL; i++) {
	    if ( !strcasecmp(args[0], g_Handlers[i].cmd) ||
		 !strcasecmp(args[0], g_Handlers[i].abbrev) ) {

		pthread_mutex_lock(&g_ManagerLock);
		g_Handlers[i].handler(args);
		pthread_mutex_unlock(&g_ManagerLock);

		done = true;
		break;
	    }
	}

	if (!done) {
	    cout << "ERROR: unknown command: " << args[0] << endl;
	}
    }
    cout << (!g_NoColorize?"[31m":"") 
	 << "==END_RESP" 
	 << (!g_NoColorize?"[m":"")  << endl;

    if (free_cmdline) free(cmdline);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

#define NOT_ENOUGH_ARGS(x) cout << "ERROR: need " << x << " arguments" << endl
#define DOES_NOT_EXIST(x) cout << "ERROR: object does not exist" << endl;


void handleHelp(char **args) {
#if 0
    cout << "Definitions:" << endl;
    cout << "  [35m<ID>[m       = a name you gave an object or its GUID" << endl;
    cout << "  [35m<attr_key>[m = a string (no spaces) for an attribute" << endl;
    cout << "  [35m<attr_val>[m = a string <type>:<value> string for value" << endl;
    cout << "  [35m<type>[m     = {int,char,byte,real,string,guid,ID}" << endl;
    cout << "  [35m<guid>[m     = <ip>:<port>:<localOID>, where <ip> is in dotted-tail format" << endl;
    cout << "  [35m<op>[m       = {==,!=,<,<=,>,>=,*}, where * means 'any'" << endl;
#endif
    cout << "Commands:" << endl;

    for (int i=0; g_Handlers[i].cmd != NULL; i++) {
	cout << "  [35m[" << g_Handlers[i].abbrev << "] "
	     << g_Handlers[i].cmd << " " 
	     << g_Handlers[i].usage  << "[m" << endl;
	cout << "    " << g_Handlers[i].help << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////

guid_t parse_guid(char *str)
{
    char *ip   = strtok(str, ":");
    char *port = strtok(NULL, ":");
    char *localOID = strtok(NULL, ":");
    if (ip == NULL || port == NULL || localOID == NULL) {
	return GUID_NONE;
    }
    uint32 _ip   = inet_addr(ip);
    uint16 _port = (uint16)atoi(port);
    uint32 _localOID = (uint32)atoi(localOID);
    if (_ip == INADDR_NONE || _port == 0) {
	return GUID_NONE;
    }

    guid_t guid(_ip, _port, _localOID);

    return guid;
}

sid_t parse_sid(char *str)
{
    char *ip   = strtok(str, ":");
    char *port = strtok(NULL, ":");
    if (ip == NULL || port == NULL) {
	return SID_NONE;
    }
    uint32 _ip   = inet_addr(ip);
    uint16 _port = (uint16)atoi(port);
    if (_ip == INADDR_NONE || _port == 0) {
	return SID_NONE;
    }

    sid_t sid(_ip, _port);

    return sid;
}

///////////////////////////////////////////////////////////////////////////////

static char str_buf[512];
static char *arg_buf[512];

char *extract_str(char *buf, int *len) 
{
    int i;
    bool found = 0;

    // parse one command terminated by a newline into the static buf
    for (i=0; i<*len; i++) {
	str_buf[i] = buf[i];
	if (str_buf[i] == '\n') {
	    str_buf[i] = '\0';
	    // if client writes \r\n, then need to get rid of two chars
	    if (str_buf[i-1] == '\r')
		str_buf[i-1] = '\0';
	    found = 1;
	    i++;
	    break;
	}
    }

    // if there is stuff left in the buffer, move it to the front
    int j = 0;
    for ( ; i<*len; i++, j++) {
	buf[j] = buf[i];
    }
    *len = j;

    if (found) {
	return str_buf;
    } else {
	return NULL;
    }
}

char **extract_args(char *buf) {
    int i;
    char *last = buf;
    int index = 0;

    // separate command string into an array of string args
    for (i=0; i<512; i++) {
	bool end = (buf[i] == '\0');

	if (end || isspace(buf[i])) {
	    buf[i] = '\0';
	    if (last < buf+i) {
		arg_buf[index] = last;
		++index;
		if (index > 512)
		    break;
	    }
	    last = buf + i + 1;
	}

	if (end) break;
    }
    arg_buf[index] = NULL;

    return arg_buf;
}

///////////////////////////////////////////////////////////////////////////////

void Cleanup(int sig)
{
    exit(0);
}

int main(int argc, char **argv)
{
    sid_t sid(1, 5);
    DBG_INIT(&sid);

    ////////// set mercury preferences
    InitializeMercury(&argc, argv, g_AppOptions);
    OM_RegisterEventTypes ();
    OM_RegisterInterestTypes ();
    OM_RegisterMessageTypes ();

    // forcibly disable measurement -- g_LocalSID is not initialized by us; only manager does it.
    g_MeasurementParams.enabled = false;

    ////////// construct the mercury router now
    InitApp();

    ////////// run the mercury router according to commands you get from 
    //         the controlling test suite as well as from the network
    //         (other mercury peers)
    RunApp();

    return 0;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
