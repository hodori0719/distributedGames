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
#include <mercury/Sampling.h>
#include <util/ExpLog.h>
#include <util/debug.h>
#include <util/Options.h>
#include <util/TimeVal.h>
#include <Mercury.h>

extern char g_ProgramName[];
extern int g_VerbosityLevel;

bool g_SetSchedulerPolicy;
bool g_WANRecvSleeps;

OptionType g_MercuryOptions[] = {

    ///// HOST PARAMS

    { 'H', "hostname", OPT_STR,
      "hostname - required on emulab machines", 
      (g_Preferences.hostname), "127.0.0.1", NULL},
    { 'p', "port", OPT_INT,
      "mercury port", 
      &(g_Preferences.port), "7000", NULL},

    ///// NETWORK PARAMS

    { '#', "compress", OPT_NOARG | OPT_BOOL, 
      "enable msg compression", &(g_Preferences.msg_compress),
      "0", (void *) "1"},
    { '#', "compress-minsize", OPT_INT,
      "min size to compress", &(g_Preferences.msg_compminsz), "128", NULL},
    { '#', "latency", OPT_NOARG | OPT_BOOL, 
      "enable artificial latency graph", &(g_Preferences.latency),
      "0", (void *) "1"},
    { '#', "latency-file", OPT_STR,
      "artificial latency file", g_Preferences.latency_file,
      "", NULL},
    { '#', "max-tcp-connections", OPT_INT,
      "max open tcp connections (xxx only for async realnet now)", 
      &g_Preferences.max_tcp_connections,
      "0", NULL},
    { '#', "slowdown-factor", OPT_FLT, 
      "slow down the entire system by this factor (> 1)", &(g_Slowdown), 
      "0.0", NULL},
    { '#', "nosleep", OPT_NOARG | OPT_BOOL, 
      "disable sleeping in WANMercuryNode::Recv", &(g_WANRecvSleeps),
      "1", (void *) "0"},
    { '#', "use-poll", OPT_NOARG | OPT_BOOL,
      "Use poll instead of select",
      &g_Preferences.use_poll, "0", (void *) "1" },


    ///// MERCURY PARAMS

    // bootstrap related parameters
    { 'B', "bootstrap", OPT_STR,
      "address:port of bootstrap server", 
      (g_Preferences.bootstrap), "", NULL},
    { '#', "schema-str", OPT_STR,
      "comma-separated [name:min:max:is_member_of_hub], e.g, x:0:10000:true", 
      (g_Preferences.schema_string), "", NULL },
    { '#', "join-locations", OPT_STR,
      "comma-separated [attribute:ip:port], possible successors for each hub",
      (g_Preferences.join_locations), "", NULL },
    { '#', "succ-wait-alljoin", OPT_NOARG | OPT_BOOL,
      "don't start building successor lists until everyone is joined", 
      &g_Preferences.succ_wait_alljoin, "0", (void *) "1"},
    

    // routing related parameters
    { 'b', "backpub", OPT_NOARG | OPT_BOOL,
      "send pub back to creator", 
      &(g_Preferences.send_backpub), "0", (void *) "1"},
    { '#', "fanoutpubs", OPT_NOARG | OPT_BOOL, 
      "enable \"fanning out\" of range pubs", &(g_Preferences.fanout_pubs), 
      "0", (void *) "1"},
    { 's', "softsubs", OPT_NOARG | OPT_BOOL,
      "use softstate subscriptions", 
      &(g_Preferences.use_softsubs), "1", (void *) "1"},
    { 'l', "sublife", OPT_INT,
      "soft-sub lifetime (msec)", 
      &(g_Preferences.sub_lifetime), "10000", NULL },
    { '#', "pubtriggers", OPT_NOARG | OPT_BOOL, 
      "enable publication triggers", &(g_Preferences.enable_pubtriggers),
      "0", (void *) "1"},

    // other mercury parameters
    { '#', "cache", OPT_NOARG | OPT_BOOL, 
      "enable caching", &(g_Preferences.use_cache),
      "0", (void *) "1"},
    { '#', "cachesize", OPT_INT, 
      "cache size", &(g_Preferences.cache_size),
      "8", NULL},
    { '#', "rconfig", OPT_STR,
      "Routing config file", Parameters::ConfigFile,
      "", NULL},
    { '#', "mercopts", OPT_STR,
      "same as config but a comma separated list of VAR=VAL pairs", Parameters::ConfigOpts,
      "", NULL},
      
    { '#', "merctrans", OPT_STR,
      "Transport protocol to use for mercury {TCP, UDP, CBR}", 
      g_Preferences.merctrans, "UDP", NULL},
    { '#', "maxttl", OPT_INT, 
      "Maximum message TTL", &(Parameters::MaxMessageTTL), 
      "20", NULL },

    // sampling related parameters
    { '#', "sampling", OPT_NOARG | OPT_BOOL, 
      "enable random walk based sampling", &(g_Preferences.distrib_sampling),
      "0", (void *) "1"},
    { '#', "selfhistos", OPT_NOARG | OPT_BOOL, 
      "use self-generated histograms (ignore histograms sent by bootstrap)", 
      &(g_Preferences.self_histos), "0", (void *) "1"},
      
    // LOAD BALANCING PARAMS
    { '#', "loadbal-routeload", OPT_NOARG | OPT_BOOL, 
      "load balance based on mercury's routing load", &(g_Preferences.loadbal_routeload),
      "0", (void *) "1"},
    { '#', "load-balance", OPT_NOARG | OPT_BOOL, 
      "enable mercury-level load balancing", &(g_Preferences.do_loadbal),
      "0", (void *) "1"},
    { '#', "load-balance-delta", OPT_FLT,
      "keep load between mean/delta and mean*delta (delta >= sqrt(2))",
      &(g_Preferences.loadbal_delta), "2.0", NULL},


    { '#', "nosuccdebug", OPT_NOARG | OPT_BOOL, 
      "don't print out periodic succ maintenance info", &(g_Preferences.nosuccdebug), 
      "0", (void *) "1"},

    ///// MEASUREMENT PARAMS

    { '#', "measurement", OPT_NOARG | OPT_BOOL,
      "enable measurement log", 
      &(g_MeasurementParams.enabled), "0", (void *) "1"},
    { '#', "log-enable-only", OPT_STR,
      "only enable logs in this comma separated list", 
      (g_MeasurementParams.selEnable), "", NULL},
    { '#', "log-sample-params", OPT_STR,
      "comma separated list of logname=sample_rate/sample_size", 
      (g_MeasurementParams.sampleRates), "", NULL},
    { '#', "log-buffer-size", OPT_INT,
      "in memory log buffer", 
      &(g_MeasurementParams.bufferSize), "1024", NULL},
    { '#', "log-flush-interval", OPT_INT,
      "time between log flush (msec)", 
      &(g_MeasurementParams.flushInterval), "30000", NULL},
    { '#', "log-rotate-size", OPT_INT,
      "max size before rotation (bytes)", 
      &(g_MeasurementParams.maxLogSize), "536870912" /* 512MB */, NULL},
    { '#', "log-gzip", OPT_NOARG | OPT_BOOL,
      "gzip rotated logs", 
      &(g_MeasurementParams.gzipLog), "0", (void *) "1"},
    { '#', "log-dir", OPT_STR,
      "directory to place lots",
      g_MeasurementParams.dir, "", NULL},
    { '#', "log-binary", OPT_NOARG | OPT_BOOL,
      "write some logs in binary", 
      &(g_MeasurementParams.binaryLog), "0", (void *) "1"},
    { '#', "log-aggregate", OPT_NOARG | OPT_BOOL,
      "report only aggregates for message and discovery logs",
      &(g_MeasurementParams.aggregateLog), "0", (void *) "1"},
    { '#', "log-aggregate-interval", OPT_INT,
      "time between log aggregations",
      &(g_MeasurementParams.aggregateLogInterval), "1000", NULL},

    ///// GENERAL PARAMS

    { '#', "sched-rr", OPT_NOARG | OPT_BOOL,
      "set scheduler priority to round-robin (requires root priviledges)", 
      &(g_SetSchedulerPolicy), "0", (void *) "1"},
    { 'v', "verbosity", OPT_INT,
      "debug verbosity level", 
      &(g_VerbosityLevel), "-1", NULL},
    { 'D', "debugfiles", OPT_STR,
      "show debug for these files", 
      (g_DebugFiles), "", NULL},
    { '#', "debugfuncs", OPT_STR,
      "show debug for these functions", 
      (g_DebugFuncs), "", NULL},
    { '#', "nosleep", OPT_BOOL, 
      "do not sleep on crash",
      &g_SleepOnCrash, "1", (void *) "0" },

    {0, 0, 0, 0, 0, 0, 0}
};

#include <sched.h>

// If some options are set, certain other options should also be set.
// Do such fixes here.
static void FixMercuryOptions()
{
    if (g_Preferences.schema_string[0] != '\0' || g_Preferences.do_loadbal) {
	g_Preferences.distrib_sampling = true;  // --sampling
	g_Preferences.self_histos = true;      // --selfhistos
    }
}

void InitializeMercury(int *pArgc, char *argv[], OptionType appOptions[], bool printOptions)
{
    memset(&g_Preferences, 0, sizeof(g_Preferences));
    OptionType *opts;
    if (appOptions == NULL) {
	opts = g_MercuryOptions;
    } else {
	int mlen = 0, alen = 0;
	for ( ; g_MercuryOptions[mlen].longword ; mlen++)
	    ;
	for ( ; appOptions[alen].longword ; alen++)
	    ;
	opts = new OptionType[mlen + alen + 1];
	memcpy(opts, g_MercuryOptions, mlen*sizeof(OptionType));
	memcpy(opts+mlen, appOptions, alen*sizeof(OptionType));
	bzero(opts+mlen+alen, sizeof(OptionType));
    }

    strcpy(g_ProgramName, argv[0]);
    *pArgc = ProcessOptions(opts, *pArgc, argv, true);
    FixMercuryOptions();
#if 0
    msg ("Argc finally :%d\n", *pArgc);
    for (int i = 0; i < *pArgc; i++) {
	fprintf(stderr, "argv[%d]=%s\n", i, argv[i]);
    }
#endif

    ConfigMercuryParameters();
    if (printOptions) {
	PrintOptionValues (opts);
	PrintMercuryParameters ();
    }
    ValidateMercuryParameters ();

    // Jeff: I believe this should be commented out now that we modified
    // GetCurrentTime to do scaling automatically, correct? For some reason
    // this was still here so I removed it...
#if 0
    if (g_Slowdown > 1.0) {
	ScaleMercuryParameters (g_Slowdown);
    }
#endif 

    if (appOptions != NULL) delete[] opts;

    InitCPUMHz();
    Benchmark::init();

    RegisterMessageTypes ();
    RegisterEventTypes ();
    RegisterInterestTypes ();
    RegisterMetricTypes ();


    // try setting our priority -- need root priviledges for this
    if (g_SetSchedulerPolicy) {

	struct sched_param p;
	p.sched_priority = 1;

	if (sched_setscheduler (0, SCHED_RR, &p) < 0) {
	    Debug::die ("could not set scheduler policy");
	}
	INFO << "set scheduler policy to ROUND ROBIN" << endl;
    }


}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
