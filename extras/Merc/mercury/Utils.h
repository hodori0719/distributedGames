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

/***************************************************************************
  Utils.h 

begin           : Nov 6, 2002
copyright       : (C) 2002-2003 Ashwin R. Bharambe ( ashu@cs.cmu.edu   )
(C) 2002-2003 Justin Weisz       ( jweisz@cs.cmu.edu )

***************************************************************************/


#ifndef __UTILS__H
#define __UTILS__H

#include <mercury/common.h>
#include <mercury/Message.h>

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

    void   G_UpdateCurrentTime();
uint32 G_DiffMillis(TimeVal a, TimeVal b);
void   G_PrintTime(TimeVal a, FILE *stream = stderr);
float  G_GetRandom();
TimeVal G_AddMillis(TimeVal &a, int ms);
ostream& operator<<(ostream& os, TimeVal &t);

#if 0
uint32  operator-(TimeVal &a, TimeVal &b);
TimeVal operator+(TimeVal &a, int add);
#endif
#endif  // __UTILS__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
