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
  Parameters.h

  One place which lists all parameters for the Mercury code.

begin           : Apr 17, 2004
copyright       : (C) 2004 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2004 Jeff Pang      ( jeffpang@cs.cmu.edu   )

***************************************************************************/
#ifndef __PARAMETERS__H
#define __PARAMETERS__H

#include <mercury/common.h>
#include <mercury/NetworkLayer.h>

    // High values for some parameters since some of the processing is slowed by 
    // lots of debug messages...
    namespace Parameters {
    extern int MaxJoinAttempts                  ; 
    extern int MaxBootstrapRequestAttempts      ; 
    extern int BootstrapRequestTimeout          ;               // period to wait before being upset about the bootstrap server
    extern int JoinRequestTimeout               ;               // period to wait before being upset about a successor
    extern int TCPFailureTimeout                ;               // if message hasn't gone for this much time, it is lost!

    extern int SuccessorMaintenanceTimeout      ;               // ping successor every so often

    extern int PeerPingInterval                 ;               // ping peers every <k> milliseconds
    extern int PeerPongTimeout                  ;               // peer is dead if a pong is not received for <k> milliseconds
    extern int LongNeighborResponseTimeout      ;               // 50ms * 20 = log n?  time to wait before assuming the long neighbor request
    // went nowhere!

    extern int BootstrapHeartbeatInterval       ;               // 1/freq of heartbeats sent to the bootstrap server
    extern int BootstrapHeartbeatTimeout        ;               // checking interval is > 2.5 times sending interval
    extern int BootstrapUpdateHistInterval      ;               // how often to update the histograms at the bootstrap server 
    extern int BootstrapSamplingInterval        ;               // perform bootstrap sampling (asking bootstrap for histogram)

    extern int LocalSamplingInterval            ;               // invoke local sampling operations this often
    extern int RandomWalkInterval               ;               // start random walks for metric sampling every so often
    extern int NCHistoBuildInterval             ;               // how often to rebuild nodecount histogram from samples
    extern int LoadAggregationInterval          ;               // how often to take load averages
    extern int CheckLoadBalanceInterval         ;               // how often to check for load balance
    extern int LeaveJoinResponseTimeout         ;               // how much to wait for a "response" to leave-join request

    extern int KickOldPeersTimeout              ;               // keep them around for a while; you can use old peers for some time...

    extern TransportType TransportProto         ;               // transport protocol to use for mercury
    extern int NSuccessorsToKeep                ;               // number of successors to maintain in the successor list
    extern int MaxMessageTTL                    ; 
    extern char ConfigFile[];
    extern char ConfigOpts[];
  };

void ConfigMercuryParameters();
void PrintMercuryParameters ();
void ValidateMercuryParameters ();
void ScaleMercuryParameters (float factor);


#endif // __PARAMETERS__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
