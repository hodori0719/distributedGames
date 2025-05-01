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

#ifndef __COMMON__H
#define __COMMON__H

#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <util/types.h>
#include <util/debug.h>
#include <util/TimeVal.h>

#define SIMULATION_MODE 's'
#define IMPLEMENTATION_MODE 'r'

#define STR_EQ(s1, s2)  (strcmp(s1.c_str(), s2.c_str()) == 0)
#define STR_NEQ(s1, s2) (strcmp(s1.c_str(), s2.c_str()) != 0)

typedef int socket_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t; // platform-specifc ?
typedef int type_t;

#ifdef _WIN32
// #include <WS2tcpip.h>
#endif

// Beware: SOCKET is unsigned on Win32. Careful in conditionals.
#ifdef _WIN32
// typedef SOCKET         Socket;
#else
typedef int            Socket;
typedef long int       DWORD;
#endif  // ifdef _WIN32

#ifndef _WIN32
#pragma GCC poison TODO
#endif

typedef struct {
    unsigned int ip;
    int port;
} address_t;

/* mercury preferences: really in the wrong place, but we will fix later */
typedef struct {
    char    hostname[255];
    char    bootstrap[255];
    char    schema_string[1024];
    char    join_locations[1024];
    bool    succ_wait_alljoin;

    char    logger[255];
    int	    port;
    bool    use_cache;
    int     cache_type; /* really a cache_t */
    int     cache_size; 

    bool    use_softsubs;       // use softstate subs
    bool    enable_pubtriggers; // enable publication triggers

    //  bool    enable_puboverwriting; // enable publication triggers to be overwritten -- dont store stale pubs
    //  int     pub_lifetime;   // how long softstate pubs live for (msec)
    int     sub_lifetime;       // how long softstate subs live for (msec)
    bool    send_backpub;       // send a pub back to the creator (false = no)
    char    merctrans[255];     // transport proto to use for mercury
    bool    use_poll;           // use poll instead of select for waiting

    bool    msg_compress;       // enable message compression
    int     msg_compminsz;      // min size of messages to compress
    bool    latency;            // enable artificial latency 
    char    latency_file[255];  // file with artificial latencies
    int     max_tcp_connections;// max open tcp connections (xxx: only for async realnet now)

    bool    fanout_pubs;        // enable "fanning out" of range pubs
    bool    distrib_sampling;   // perform random-walk based sampling
    bool    do_loadbal;         // perform load balancing
    bool    loadbal_routeload;  // load balance using mercury's routing load
    float   loadbal_delta;      // keep load between mean/delta and mean*delta (delta >= sqrt(2))
    bool    self_histos;        // use self-generated histograms (dont rely on bootstrap)
    bool    nosuccdebug;        // disable periodic printout of succ info
} pref_t;

extern pref_t g_Preferences;

class Packet;
class Serializable {
 public:
    virtual void    Serialize(Packet *pkt)  = 0;
    virtual uint32  GetLength()             = 0;
    virtual void    Print(FILE *stream)     = 0;

    virtual ~Serializable () {}
};

struct IPEndPoint;
struct Message;

// Does not really belong here, but I am in no mood 
class MessageHandler {
 public:
    virtual void ProcessMessage(IPEndPoint *from, Message *msg) = 0;
    virtual ~MessageHandler () {}
};

#endif // __COMMON__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
