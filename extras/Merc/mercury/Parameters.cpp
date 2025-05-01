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
#include <mercury/Parameters.h>

namespace Parameters {
    int MaxJoinAttempts                  = 25; 
    int MaxBootstrapRequestAttempts      = 20; 

    int BootstrapRequestTimeout          = 1000;                 // period to wait before being upset about the bootstrap server
    int JoinRequestTimeout               = 2000;                // period to wait before being upset about a successor
    int TCPFailureTimeout                = 10000;               // if message hasn't gone for this much time, it is lost!    
    int SuccessorMaintenanceTimeout      = 1000;                 // time to wait before checking on the successor

    int LongNeighborResponseTimeout      = 1000;                // 50ms * 20 = log n?  time to wait before assuming the long neighbor request
    // went nowhere!

    int PeerPingInterval                 = 15000;                // ping peers every <k> milliseconds
    int PeerPongTimeout                  = 45000;               // peer is dead if a pong is not received for <k> milliseconds

    int BootstrapHeartbeatInterval       = 5000;                // 1/freq of heartbeats sent to the bootstrap server
    int BootstrapHeartbeatTimeout        = 60000;               // checking interval is > 2.5 times sending interval
    int BootstrapUpdateHistInterval      = 5000;                // how often to update the histograms at the bootstrap server 

    int BootstrapSamplingInterval        = 1000;                // perform bootstrap sampling this often
    int LocalSamplingInterval            = 3000;                 // invoke local sampling operations this often

    //// XXX beware. each sampler uses its own RandomWalkInterval!
    int RandomWalkInterval               = 5000;                 // start random walks for metric sampling every so often
    int NCHistoBuildInterval             = 500;                 // how often to rebuild nodecount histogram from samples
    int LoadAggregationInterval          = 1000;                // how often to take load averages

    int CheckLoadBalanceInterval         = 8000;               // how often to check for load balance
    //// XXX beware, keep this very conservative. otherwise, unnecessary 
    //// leave-joins can happen... apparently, it may take a long time 
    //// for people for leave and re-join...
    int LeaveJoinResponseTimeout         = 60000;               // how much to wait for a "response" to leave-join request

    int KickOldPeersTimeout              = 60000;               // keep them around for a while; you can use old peers for some time...

    TransportType TransportProto         = PROTO_UDP;           // transport protocol to use for mercury
    int NSuccessorsToKeep                = 10;                  // ideal for <= 2^10 = 1K nodes
    int MaxMessageTTL                    = 1000; 

    char ConfigFile[255];
    char ConfigOpts[2048];
};

using namespace Parameters;

void ScaleMercuryParameters (float factor)
{
    ASSERT (factor > 1.0);

#undef scale_by_factor
#define scale_by_factor(var)  var = (int) (var * factor);

    scale_by_factor (BootstrapRequestTimeout);                            
    scale_by_factor (JoinRequestTimeout);                                 

    // scale_by_factor (XXX);  ideally this should not need to be scaled 
    scale_by_factor (TCPFailureTimeout);                     

    scale_by_factor (SuccessorMaintenanceTimeout);                        
    scale_by_factor (LongNeighborResponseTimeout);                       

    scale_by_factor (PeerPingInterval);                                  
    scale_by_factor (PeerPongTimeout);                                  

    scale_by_factor (BootstrapHeartbeatInterval);                        
    scale_by_factor (BootstrapHeartbeatTimeout);                        
    scale_by_factor (BootstrapUpdateHistInterval);                       

    scale_by_factor (BootstrapSamplingInterval);                        
    scale_by_factor (LocalSamplingInterval);                              
    scale_by_factor (RandomWalkInterval);                                 
    scale_by_factor (NCHistoBuildInterval);
    scale_by_factor (LoadAggregationInterval);

    scale_by_factor (CheckLoadBalanceInterval);
    scale_by_factor (LeaveJoinResponseTimeout);

    scale_by_factor (KickOldPeersTimeout);                              
#undef scale_by_factor
}

void ValidateMercuryParameters ()
{
    if (PeerPingInterval >= PeerPongTimeout) {
	Debug::die("PeerPingInterval >= PeerPongTimeout");
    }

    if (BootstrapHeartbeatInterval >= BootstrapHeartbeatTimeout) {
	Debug::die("BootstrapHeartbeatInterval >= BootstrapHeartbeatTimeout");
    }
}

void PrintMercuryParameters ()
{
    fprintf (stderr, "\tMaxJoinAttempts=%d\n", MaxJoinAttempts);                   
    fprintf (stderr, "\tMaxBootstrapRequestAttempts=%d\n", MaxBootstrapRequestAttempts);       

    fprintf (stderr, "\tBootstrapRequestTimeout=%d\n", BootstrapRequestTimeout);                           
    fprintf (stderr, "\tJoinRequestTimeout=%d\n", JoinRequestTimeout);                                
    fprintf (stderr, "\tTCPFailureTimeout=%d\n", TCPFailureTimeout);                                
    fprintf (stderr, "\tSuccessorMaintenanceTimeout=%d\n", SuccessorMaintenanceTimeout);                       

    fprintf (stderr, "\tLongNeighborResponseTimeout=%d\n", LongNeighborResponseTimeout);                      


    fprintf (stderr, "\tPeerPingInterval=%d\n", PeerPingInterval);                                 
    fprintf (stderr, "\tPeerPongTimeout=%d\n", PeerPongTimeout);                                 

    fprintf (stderr, "\tBootstrapHeartbeatInterval=%d\n", BootstrapHeartbeatInterval);                       
    fprintf (stderr, "\tBootstrapHeartbeatTimeout=%d\n", BootstrapHeartbeatTimeout);                       
    fprintf (stderr, "\tBootstrapUpdateHistInterval=%d\n", BootstrapUpdateHistInterval);                      

    fprintf (stderr, "\tBootstrapSamplingInterval=%d\n", BootstrapSamplingInterval);                       
    fprintf (stderr, "\tLocalSamplingInterval=%d\n", LocalSamplingInterval);                             
    fprintf (stderr, "\tRandomWalkInterval=%d\n", RandomWalkInterval);                                
    fprintf (stderr, "\tCheckLoadBalanceInterval=%d\n", CheckLoadBalanceInterval);
    fprintf (stderr, "\tLeaveJoinResponseTimeout=%d\n", LeaveJoinResponseTimeout);                                

    fprintf (stderr, "\tKickOldPeersTimeout=%d\n", KickOldPeersTimeout);                             

    fprintf (stderr, "\tTransportProto=%s\n", PROTO_UDP ? "PROTO_UDP" : "OTHER");
    fprintf (stderr, "\tNSuccessorsToKeep=%d\n", NSuccessorsToKeep);                         
    fprintf (stderr, "\tMaxMessageTTL=%d\n", MaxMessageTTL);                     
}

///////////////////////////////////////////////////////////////////////////

#include <util/Options.h>
typedef struct {
    int flags;
    void *varptr;
    char *name;
} Param;

#define P(flags, var) { flags, &(var), # var }

static Param params[] = {
    P(OPT_INT, MaxJoinAttempts),
    P(OPT_INT, BootstrapRequestTimeout),
    P(OPT_INT, JoinRequestTimeout),

    P(OPT_INT, PeerPingInterval),
    P(OPT_INT, PeerPongTimeout),

    P(OPT_INT, BootstrapHeartbeatInterval),
    P(OPT_INT, BootstrapHeartbeatTimeout),
    P(OPT_INT, BootstrapUpdateHistInterval),

    P(OPT_INT, BootstrapSamplingInterval),
    P(OPT_INT, KickOldPeersTimeout),

    P(OPT_INT, SuccessorMaintenanceTimeout),
    P(OPT_INT, LocalSamplingInterval),
    P(OPT_INT, RandomWalkInterval),
    P(OPT_INT, CheckLoadBalanceInterval),
    P(OPT_INT, LeaveJoinResponseTimeout),

    {0, 0, 0}
};

static void _AssignValue(Param * ptr, char *value)
{
    if (ptr->flags & OPT_CHR) {
	*(char *) ptr->varptr = *value;
    } else if (ptr->flags & OPT_INT) {
	*(int *) ptr->varptr = atoi(value);
    } else if (ptr->flags & OPT_FLT) {
	*(float *) ptr->varptr = atof(value);
    } else if (ptr->flags & OPT_STR) {
	strcpy((char *) ptr->varptr, value);
    } else if (ptr->flags & OPT_BOOL) {
	bool toSet = (strcmp(value, "1") == 0 ? true : false);
	*((bool *) ptr->varptr) = toSet;
    }
}

#define LINEBUF_SIZE 256

static void _ParseLine(char *line, int curr_line)
{
    char name[LINEBUF_SIZE], val[LINEBUF_SIZE];

    if (sscanf(line, "%s %s", name, val) != 2) {
	Debug::die("#fields on line %d != 2 (%s)\n", curr_line, line);
    }

    bool found = false;
    Param *ptr = params;
    while (ptr->name) {
	if (!strcmp(name, ptr->name)) {
	    //
	    // Process this param now.
	    //
	    _AssignValue(ptr, val);
	    found = true;
	    break;
	} else {
	    //DBG << "'" << name << "' DOESN'T MATCH '" << ptr->name << "'" << endl;
	}
	ptr++;
    }

    if (!found) {
	Debug::die("unknown parameter name %s on line %d", name, curr_line);
    }
}

void ConfigMercuryParameters()
{
    if ( !strcmp(ConfigFile, "") )
	return;

    FILE *fp = fopen(ConfigFile, "r");
    if (! fp) {
	Debug::die("Couldn't open routing config file: %s", ConfigFile);
    }

    char line[LINEBUF_SIZE];
    int curr_line = 1;

    while (fgets(line, LINEBUF_SIZE, fp)) {
	if (line[strlen(line) - 1] != '\n') {
	    Debug::die("internal error: line %d longer than 255 characters, bailing",
		       curr_line);
	}
	if (line[0] == '#' || line[0] == '\n') {
	    curr_line++;
	    continue;
	}
	line[strlen(line) - 1] = '\0';

	_ParseLine(line, curr_line);
	curr_line++;
    }
    
    // options override config file
    char *str = strtok(ConfigOpts, ",;");
    while (str != NULL) {
	_ParseLine(str, -1);
	str = strtok(NULL, ",;");
    }
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
