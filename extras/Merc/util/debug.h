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
  debug.h

begin           : Oct 16, 2003
version         : $Id: debug.h 2472 2005-11-13 01:34:54Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <errno.h>
#include <util/stacktrace.h>
#include <ctype.h>
#include <cstdlib>

    /**
     * The Mercury/ObjectManagment Debugging Framework.
     *
     * Initilialization and Signal Handling
     * ====================================
     *
     * Before any of this will work, you must set g_ProgramName to argv[0]
     * and call the following function:
     *
     * DBG_INIT();
     *
     * We will catch SIGSEGV signals and use a detached gdb process to print
     * a stack trace and debugging symbols in the stack frames. The program
     * will then pause to allow you to attach to the process with gdb to
     * debug it.
     *
     * Debugging streams:
     * ==================
     *
     * DBG     - default debugging stream that is turned on iff DEBUG is set.
     *           Alias for DB(5).
     * DB(lvl) - debug stream with tunnable verbosity level.
     * WARN    - warning stream. Always on. Respects verbosity levels.
     * INFO    - info stream. Always on. Respects verbosity levels.
     *
     * Usage: same as any other C++ iostream, except each "line" usage must be
     * terminated by a newline. For example:
     *
     * DB(3) << "my message to you: " << msg << endl;
     *
     * This is because each use prefixes the line with the appropriate prefix.
     *
     * Debugging blocks:
     * =================
     *
     * DBG_DO {
     *     .
     *     .
     *     .
     * }
     *
     * DB_DO(lvl) {
     *     .
     *     .
     *     .
     * }
     *
     * This will case everything in the block { ... } to be executed iff DEBUG
     * is enabled. Otherwise it will be compile out, just as with DBG statements.
     * DBG_DO is equivalent to DB_DO(5).
     *
     * Legacy Debugging Functions:
     * ===========================
     *
     * warn(format, args...);
     *
     *     Same as the WARN stream, but with printf style calling semantics.
     *
     * die(format, args...);
     *
     *     Print a fatal error and exit.
     *
     * Defines
     * =======
     *
     * REQUIRE_ASSERTIONS - turn assertions on even if debug is off
     * WARN_ASSERTIONS    - if debug is off, this turns assertions into warnings
     *
     */

    /**
     * Debugging Compile-Time Options.
     */
#define DBG_SHOWTIME 0   // show the time with debug messages
#define DBG_SHOWLINE 1   // show the file:func:line with debug messages

#ifndef USE_ASSERTIONS
#define USE_ASSERTIONS // define to enable assertions by default
#endif

    //////////////////////////////////////////////////////////////////////////////

#include <iostream>

    enum { DBG_MODE_DEBUG, 
	   DBG_MODE_WARN,
	   DBG_MODE_INFO, };

/**
 * Debugging stream
 */
class DebugStream {
 public:
    std::ostream& operator() (int mode, int lvl = 0, const char *file = NULL, 
			      const char *func = NULL, int line = 0);
};

extern DebugStream __g_DebugStream;
extern bool __DebugThisFile(const char *file);
extern bool __DebugThisFunc(const char *func);
extern char g_DebugFiles[];
extern char g_DebugFuncs[];
extern char g_ProgramName[];
extern int g_VerbosityLevel;
extern bool g_SleepOnCrash;

#ifdef DEBUG
#define DB(lvl) if (g_VerbosityLevel >= lvl) __g_DebugStream(DBG_MODE_DEBUG, lvl, __FILE__, __FUNCTION__, __LINE__)
#else
#define DB(lvl) 1 ? cerr : cerr 
#endif

#define DBG  DB(5)
#define WARN __g_DebugStream(DBG_MODE_WARN, 0, __FILE__, __FUNCTION__, __LINE__)
#define INFO __g_DebugStream(DBG_MODE_INFO, 0, __FILE__, __FUNCTION__, __LINE__)

#ifdef DEBUG
#define DB_DO(lvl) if (g_VerbosityLevel >= (lvl) && __DebugThisFile(__FILE__) && __DebugThisFunc(__FUNCTION__))
#define DBG_DO DB_DO(5)
#else
#define DB_DO(lvl) if (0)
#define DBG_DO if (0)
#endif

#define TDB(lvl) DB(lvl) << "[" << Debug::GetFormattedTime() << "] "
#define TINFO INFO << "[" << Debug::GetFormattedTime() << "] "
#define TWARN WARN << "[" << Debug::GetFormattedTime() << "] "

void DBG_INIT(void *args = NULL);

#ifdef warn
#undef warn
#endif
#ifdef die
#undef die
#endif
namespace Debug {
    void die(char *str = NULL, ...);
    void die_no_abort(char *str = NULL, ...);
    void warn(char *str = NULL, ...);
    void msg (char *str = NULL, ...);
    char *GetFormattedTime ();
    unsigned int FreeMem(); 
    void PrintUDPStats (ostream& os = cerr);
};
#define _die(str,args...) Debug::die(str,args)
#define _warn(str,args...) Debug::warn(str,args)
#define _msg(str,args...) Debug::msg(str,args)

#include <util/pvartmpl.h>

class space_sep : public unary_function<char, bool>
{
 public:
    bool operator()(char c) const { return isspace (c); }
};

class comma_sep : public unary_function<char, bool>
{
 public:
    bool operator()(char c) const { return (c == ','); }
};

class string_sep : public unary_function<char, bool>
{
    string str;
 public:
    string_sep (string const& str) : str (str) {}
    bool operator() (char c) const {
	unsigned int f = str.find (c);
	return (f != string::npos);
    }	    
};

template <class pred=space_sep>
class tokenizer
    {
	public:
	//The predicate should evaluate to true when applied to a separator.
	static void tokenize (vector<string>& result, const string& instr,
			      const pred& predicate = pred())
	{
	    result.clear();
	    string::const_iterator it = instr.begin();
	    string::const_iterator token_end = instr.begin();

	    while (it != instr.end())
	    {
		while (predicate (*it))
		    it++;

		token_end = find_if (it, instr.end(), predicate);

		if (it < token_end)
		    result.push_back (string (it, token_end));
		it = token_end;
	    }
	}

    };

#define print_vars(os, list...)  do { \
    vector<string> tokens; \
	tokenizer<string_sep>::tokenize (tokens, #list, string_sep (", ")); \
	_print_vars (os, tokens, list); } while (0)

//////////////////////////////////////////////////////////////////////////////
// ashwin - some useful additions (taken from the quake code)

char	*merc_va(char *format, ...);

//////////////////////////////////////////////////////////////////////////////

#ifdef ASSERT
#undef ASSERT
#endif

#if ((defined USE_ASSERTIONS && defined DEBUG) || defined REQUIRE_ASSERTIONS)
#define ASSERT(e) \
	do { \
	    if (!(e)) { \
		assertion(__FILE__, __FUNCTION__, __LINE__, #e); \
	    } \
	} while (0)
#define ASSERTM(e, msg, args...) \
do { \
    if (!(e)) { \
	assertion(__FILE__, __FUNCTION__, __LINE__, msg, ## args); \
    } \
} while (0)
#else
#ifdef WARN_ASSERTIONS
#define ASSERT(e) \
do { \
    if (!(e)) { \
	WARN << "ASSERTION failure: " << #e << endl; \
    } \
} while (0)
#define ASSERTM(e, msg, args...) \
do { \
    if (!(e)) { \
	char buf[512]; \
	    snprintf(buf, 512, msg, ##args); \
	    WARN << "ASSERTION failure: " << buf << endl; \
    } \
} while (0) 
#else
#define ASSERT(unused) do {} while (false)
#define ASSERTM(unused, m, args...) do {} while (false)
#endif
#endif

#define ASSERTDO(e, blurb) \
do { if (!(e)) { do { blurb; } while (0); ASSERT(e); } } while (0)

void assertion(const char *file, const char *func, 
	       int line, const char *cond, ...);

void PrintLine (ostream& os, char c, int cols = 80);

#endif // __DEBUG__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
