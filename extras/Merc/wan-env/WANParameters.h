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
  WANParameters.h

  Parameters for the WAN engine

begin           : Apr 17, 2004
copyright       : (C) 2004 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2004 Jeff Pang      ( jeffpang@cs.cmu.edu   )

***************************************************************************/
#ifndef __WANPARAMETERS__H
#define __WANPARAMETERS__H

#include <mercury/common.h>
#include <mercury/NetworkLayer.h>

    // High values for some parameters since some of the processing is slowed by 
    // lots of debug messages...
    namespace WANParameters {
    extern int NumMessagesPerCycle              ;               // number of pending messages to process in one call 
    // of DoWork() for the router; the router DoWork() routine 
    // returns control when there are lesser number of messages 
    // to process.
  };

#endif // __WANPARAMETERS__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
