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
#include <sim-env/Simulator.h>
#include <Mercury.h>
#include <mercury/BootstrapNode.h>
#include <util/TimeVal.h>
#include <util/debug.h>
#include <mercury/ObjectLogs.h>
#include <mercury/HubManager.h>
#include <mercury/Parameters.h>
#include <mercury/Hub.h>
#include <mercury/Message.h>
#include <sim-env/SimMercuryNode.h>
#include <mercury/Sampling.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>

// drives the simulation

Simulator *g_Simulator;
IPEndPoint s_BootstrapIP;

struct _driver_prefs_t 
{
    int nodes;
    int inter_arrival_time;
    int simulation_time;
    char lfile [256];
    bool deterministic_rand;
    int spikes;
    int spike_height;
};

struct _driver_prefs_t g_DriverPrefs;
OptionType *g_TrulyAllOptions;

OptionType g_DriverOptions [] = 
    {
	{ '#', "nodes", OPT_INT, "number of mercury nodes in the simulation",
	  &g_DriverPrefs.nodes, "4" , NULL },
	{ '#', "norand", OPT_NOARG | OPT_BOOL, "use deterministic pseudo-random ness ",
	  &g_DriverPrefs.deterministic_rand, "0" , (void *) "1" },
	{ '#', "arrive-int", OPT_INT, "inter-arrival interval (milliseconds)",
	  &g_DriverPrefs.inter_arrival_time, "100" , NULL },
	{ '#', "time", OPT_INT, "time to run the simulation for (seconds)", 
	  &g_DriverPrefs.simulation_time, "10" , NULL },
	{ '#', "lfile", OPT_STR, "file to output link information to ",
	  g_DriverPrefs.lfile, "" , NULL },
	{ '#', "spikes", OPT_INT, "number of spikes in the distribution", 
	  &g_DriverPrefs.spikes, "1", NULL },
	{ '#', "spike-height", OPT_INT, "height of each spike", 
	  &g_DriverPrefs.spike_height, "1000", NULL },
	{ 0, 0, 0, 0, 0, 0, 0 }
    };

MemberHub *GetHub (SimMercuryNode *self) {
    return (MemberHub *) (self->GetHubManager ()->GetHubByID (0));
}
HistogramMaintainer *HM (SimMercuryNode *self) {
    return GetHub (self)->GetHistogramMaintainer ();
}
LinkMaintainer *LM (SimMercuryNode *self) {
    return GetHub (self)->GetLinkMaintainer ();
}
PubsubRouter *PM (SimMercuryNode *self) {
    return GetHub (self)->GetPubsubRouter ();
}

void setup_environment (int argc, char *argv[])
{
    SID foo ("gs203.sp.cs.cmu.edu:65535");

    strcpy (g_ProgramName, *argv);
    DBG_INIT (&SID_NONE);

    // merge options
    OptionType **all_options = new OptionType *[2];
    all_options[0] = g_DriverOptions;
    all_options[1] = g_BootstrapOptions;

    g_TrulyAllOptions = MergeOptions (all_options, 2);
    delete[] all_options;

    // initialize mercury
    InitializeMercury (&argc, argv, g_TrulyAllOptions, true);

    // InitObjectLogs (foo);

    if (g_DriverPrefs.deterministic_rand) 
	srand48 (42);
    else
	srand48 (getpid () ^ time (NULL));
    g_Simulator = new Simulator ();
}

// utilities to disambiguate overloaded funcs in libm
int _ilog (double f) {
    return (int) (log (f));
}

double _dlog (double f) {
    return log (f);
}

void set_parameters ()
{
    g_MeasurementParams.enabled = false;

    /*
      Parameters::SuccessorMaintenanceTimeout = 50;
      Parameters::LongNeighborResponseTimeout = 500;
      Parameters::PeerPingInterval = 100;
      Parameters::PeerPongTimeout  = 300;
      Parameters::BootstrapHeartbeatInterval = 5000;
      Parameters::BootstrapUpdateHistInterval = 1000;
      Parameters::BootstrapSamplingInterval = 1000;
      Parameters::KickOldPeersTimeout = 400;
      Parameters::NSuccessorsToKeep   = _ilog (g_DriverPrefs.nodes) + 1;

      Parameters::RandomWalkInterval = 100;
      Parameters::LocalSamplingInterval = 100;
    */

    if (g_DriverPrefs.nodes > 100) {
	Parameters::MaxMessageTTL = (int) (10 * (_dlog (g_DriverPrefs.nodes)/ _dlog (2)));
    }
}

void start_bootstrap ()
{
    strcpy (g_Preferences.bootstrap, "gs203.sp.cs.cmu.edu:0");
    SID bsid (g_Preferences.bootstrap);

    BootstrapNode *node = new BootstrapNode (g_Simulator, g_Simulator, bsid, g_BootstrapPreferences.schemaFile);

    g_Simulator->AddNode (*node);
    node->Start ();
}

// #include "LoadTest.cxx"
#include "PubTest.cxx"
// #include "SampleTest.cxx"

int main (int argc, char *argv[])
{
    setup_environment (argc, argv);
    set_parameters ();
    start_bootstrap ();           // sets up g_Preferences.bootstrap

    run_script ();
    g_Simulator->ProcessFor (g_DriverPrefs.simulation_time * 1000);

    finish_script ();
    cerr << endl << endl << endl << ">>>>>>>> ABOUT TO EXIT; time=" << g_Simulator->TimeNow () << " <<<<<<<<" << endl << endl << endl;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
