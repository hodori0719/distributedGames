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
version         : $Id: Benchmark.h 2382 2005-11-03 22:54:59Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include <hash_map.h>
#include <util/debug.h>
#include <util/types.h>
#include <util/TimeVal.h>
#include <math.h>

    /*
     * Light-weight, easy to use benchmarking routines. The purpose of
     * this library is to provide very easy to use targeted profiling
     * information for *real-time* (not process time) measurements. The
     * functions should be accurate to within 1us, given that the
     * benchmarked routine is called enough times (the first time incurs
     * some start up overhead). For example, on a Pentium 4 2.4Ghz, a
     * START, STOP pair takes about 110ns to execute. This is about 45ns
     * of overhead compared with just two rdtsc instructions. In terms of
     * timing error, a null START/STOP pair measures 35ns on the same
     * configuration, 2ns more than two rdtsc instructions, so
     * approximately that much noise will be added to each measurement.
     *
     * The macros will be enabled so long as DEBUG is enabled, otherwise
     * they will be compiled out. If you want to leave them enabled then
     * you must define BENCHMARK_REQUIRED. No initialization is required,
     * though if you have more than MAX_BMARK_ENTRIES different timers or
     * samplers then you will get an assertion failure (assuming DEBUG is
     * enabled). You can increase the value at compile time.
     *
     * Initialize:
     *
     * InitCPUMHz();
     *
     * To use:
     *
     * START(My Label);
     * do stuff...
     * STOP(My Label);
     *
     * Times "do stuff..." and associate it with the token "My Label". Note
     * that the label is any arbitrary string and is not in quotes.
     *
     *
     * NOTE(My Other Label, value);
     *
     * Sample the value "value" and associate it with token "My Other Label".
     *
     * 
     * TIME( do stuff ... );
     *
     * Same as:
     *
     * START(do stuff ...);
     * do stuff...
     * STOP(do stuff ...);
     *
     * 
     * RESTART(My Label);
     * Benchmark::restart();
     *
     * Restart the averages, etc. for the timer associated with "My Label".
     * Restart all the timers and samplers;
     *
     * 
     * PRINT(My Label);
     * Benchmark::print();
     *
     * Print out statistics about the timer associated with token "My Label"
     * since the last RESTART. Print out statistics about all samplers and
     * timers since the last restart of each. The stats are formated as:
     *
     * avg total min max stddev count token
     *
     *
     * STOP_ASSIGN(var, My Label);
     * AVERAGE_ASSIGN(var, My Label);
     *
     * Same as STOP, but assign the timed result (as a double in msec) to
     * var. Assign the current average (as a double in msec) to var. If you
     * use these, you *must* enable benchmarking otherwise you will get a
     * compile error when benchmarking is turned off.
     */

#define MAX_BMARK_ENTRIES 1024

    ///////////////////////////////////////////////////////////////////////////////

#if (!defined(DEBUG) && !defined(BENCHMARK_REQUIRED))
#undef BENCHMARK
#else
#define BENCHMARK
#endif

#define BMARK_DO(func_call, token) do { \
    static int _bmark_key = _bmark_get_key(#token); \
	Benchmark::func_call; \
} while (0)
#define BMARK_ASSIGN(var, func_call, token) do { \
    static int _bmark_key = _bmark_get_key(#token); \
	var = Benchmark::func_call; \
} while (0)

#ifdef BENCHMARK
#define START(token)                BMARK_DO(start(_bmark_key), token)
#define STOP(token)                 BMARK_DO(stop(_bmark_key), token)
#define STOP_ASSIGN(var, token)     BMARK_ASSIGN(var, stop_ret(_bmark_key), token)
#define NOTE(token, value)          BMARK_DO(note(_bmark_key, value), token)
#define RESTART(token)              BMARK_DO(restart(_bmark_key), token)
#define PRINT(token)                BMARK_DO(print(_bmark_key), token)
#define AVERAGE_ASSIGN(var, token)  BMARK_ASSIGN(var, avg(_bmark_key), token)
#else
#define START(token) 
#define STOP(token) 
#define NOTE(token, value)
#define RESTART(token)
#define PRINT(token)
// the *_ASSIGN macros should cause a compile error if not defined, since
// presumably the client was actually using the benchmarking code in his/her
// application for real and not just for profiling. It us up to them to
// define BENCHMARK_REQUIRED in the appropriate places to make sure the
// benchmarking macros will remain on.
#endif

#ifdef BENCHMARK
#define TIME(blurb) \
do { \
    static int _bmark_key = _bmark_get_key(#blurb); \
	Benchmark::start(_bmark_key); \
	blurb; \
	Benchmark::stop(_bmark_key); \
} while (0);
#else
#define TIME(blurb) blurb;
#endif

///////////////////////////////////////////////////////////////////////////////

extern struct token_val _token_vals[];

struct  _bmark_timer {
    const char *name;
    uint64 start; // in clock ticks
    uint64 total; // in clock ticks

    uint64 sigma_sq;
    uint32 min, max;
    uint32 count;

    _bmark_timer() {}
    _bmark_timer(const char *token, uint64 time) :
	name(token), start(time), total(0), sigma_sq (0), count(0),
	 min(0xFFFFFFFF), max(0) {}
};

extern int _bmark_next_key;
extern _bmark_timer _bmark_timers[MAX_BMARK_ENTRIES];
extern hash_map<const char *, int> _bmark_key_map;

inline int _bmark_get_key(const char *name)
{
    hash_map<const char *, int>::iterator p = _bmark_key_map.find(name);
    if (p == _bmark_key_map.end()) {
	ASSERT(_bmark_next_key < MAX_BMARK_ENTRIES);
	int key = _bmark_next_key++;
	_bmark_key_map.insert(pair<const char *, int>(name, key));
	_bmark_timers[key] = _bmark_timer(name, 0);
	return key;
    } else {
	return p->second;
    }
}

///////////////////////////////////////////////////////////////////////////////

class Benchmark {
 private: 

    inline
	static void _print(int key, ostream& out=std::cout) {
	char buf[255];
	sprintf(buf, "%10.3f %8.2f %8.2f %8.2f %10.3f %6d", 
		avg(key), total(key), min(key), max(key), stddev(key),
		count(key));

	out << buf << " " << name(key) << endl;
    }

    inline
	static unsigned long long get_time()
	{
	    // don't do time dilation for benchmarking numbers
	    return REAL_CurrentTimeTicks ();
	}

 public:

    inline
	static void start(int key) {
	_bmark_timers[key].start = get_time();
    }

    inline
	static void restart(int key) {
	_bmark_timer *t = &_bmark_timers[key];

	t->total    = 0;
	t->sigma_sq = 0;
	t->count    = 0;
	t->min      = 0xFFFFFFFF;
	t->max      = 0;
    }

#define _STOP_COMMON \
    uint64 now  = get_time(); \
	_bmark_timer *t    = &_bmark_timers[key]; \
	uint64 time = now - t->start; \
	t->total    += time; \
	t->sigma_sq += time*time; \
	t->count++; \
	t->start     = (uint32)-1; \
	t->min       = MIN(t->min, time); \
	t->max       = MAX(t->max, time);

    inline
	static void stop(int key) {
	_STOP_COMMON;
	// avoid the expensive division at the end
    }

    inline
	static double stop_ret(int key) {
	_STOP_COMMON;
	return Ticks2Msec(time);
    }

    inline
	static void note(int key, int value) {
	_bmark_timer *t    = &_bmark_timers[key];
	t->total    += Msec2Ticks(value);
	t->sigma_sq += (uint64) value * (uint64) value * 
	    USEC_IN_MSEC * USEC_IN_MSEC;
	t->count++;
	t->min       = MIN(t->min, (uint32)Msec2Ticks(value));
	t->max       = MAX(t->max, (uint32)Msec2Ticks(value));
    }

    inline
	static const char *name(int key)  {
	return _bmark_timers[key].name;
    }

    inline
	static double total(int key)  {
	return Ticks2Msec(_bmark_timers[key].total);
    }

    inline
	static uint32 count(int key) {
	return _bmark_timers[key].count;
    }

    inline
	static double avg(int key) {
	_bmark_timer *t = &_bmark_timers[key];
	return Ticks2Msec(t->total/t->count);
    }

    inline 
	static double stddev(int key) {
	_bmark_timer *t = &_bmark_timers[key];
	uint32 count = t->count;
	if (count == 0) 
	    return 0;

	double ret = (double) t->sigma_sq / count;
	double avg = (double) t->total / count;

	ret -= avg * avg;
	if (ret < 0) // should never be!
	    ret = 0;
	else 
	    ret = sqrt (ret);
	return Ticks2Msec(ret);
    }

    inline
	static double min(int key) {
	return Ticks2Msec(_bmark_timers[key].min);
    }

    inline
	static double max(int key) {
	return Ticks2Msec(_bmark_timers[key].max);
    }

    static void init() {
	restart();
    }
    static void restart() { 
	for (int i=0; i<_bmark_next_key; i++)
	    restart(i);
    }
    static void print(int key, ostream& out=std::cerr);
    static void print(ostream& out=std::cerr);
    static void printpat(const char *, ostream& out=std::cerr);
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
