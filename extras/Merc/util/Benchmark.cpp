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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

/**************************************************************************
  Benchmark.h

begin           : Nov 8, 2003
version         : $Id: Benchmark.cpp 2507 2005-12-21 22:05:36Z jeffpang $
copyright       : (C) 2003-2005 Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2005      Ashwin Bharambe  ( ashu@cs.cmu.edu )

***************************************************************************/

#include <algorithm>
#include "Benchmark.h"

    int _bmark_next_key = 0;
_bmark_timer _bmark_timers[MAX_BMARK_ENTRIES];
hash_map<const char *, int> _bmark_key_map(MAX_BMARK_ENTRIES);

struct _bmark_print_more {
    bool operator()(const pair<int, uint64>& a, 
		    const pair<int, uint64>& b) {
	return a.second > b.second;
    }
};

static void print_sorted_keys (vector< pair<int, uint64> >& keys, ostream& out)
{
    stable_sort(keys.begin(), keys.end(), _bmark_print_more());
    for (vector< pair<int, uint64> >::iterator i = keys.begin (); 
	 i != keys.end (); ++i) {
	Benchmark::print ( i->first, out);
    }
}

// this just here to make sure macros compile...
static void _bmark_dummy_test()
{
    START(FOO);
    STOP(FOO);
    NOTE(GOO, 1);
    TIME( _bmark_dummy_test() );
    RESTART(FOO);
    PRINT(FOO);
    START(BOO);
    double v;
    /*
    STOP_ASSIGN(v, BOO);
    AVERAGE_ASSIGN(v, BOO);
    */
}

///////////////////////////////////////////////////////////////////////////////

void Benchmark::print(int key, ostream& out) {
    _print(key, out);
}

void Benchmark::print(ostream &out)  {
    out << "Benchmark: [" << Debug::GetFormattedTime() << "]" << endl; 

    vector< pair<int, uint64> > keys;
    for (int i=0; i<_bmark_next_key; i++) {
	if (_bmark_timers[i].count > 0)
	    keys.push_back ( pair<int, uint64>(i, _bmark_timers[i].total) );
    }
    print_sorted_keys (keys, out);
}

void Benchmark::printpat(const char *pat, ostream &out)  {
    out << "Benchmark: [" << Debug::GetFormattedTime() << "]" << endl;

    vector< pair<int, uint64> > keys;
    for (int i=0; i<_bmark_next_key; i++) {
	if (_bmark_timers[i].count > 0 && strstr(_bmark_timers[i].name, pat))
	    keys.push_back ( pair<int, uint64>(i, _bmark_timers[i].total) );
    }
    print_sorted_keys (keys, out);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
