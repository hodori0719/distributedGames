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
  debug.cpp

begin           : Oct 16, 2003
version         : $Id: debug.cpp 2472 2005-11-13 01:34:54Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#include <hash_map.h>
#include <pthread.h>
#include <fstream>
#include <stdexcept>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <util/types.h>
#include <util/TimeVal.h>
#include <util/OS.h>
#include <util/debug.h>
#include <util/stacktrace.h>

#define MAX_FILE_NUM 128

    char g_DebugFiles[1024];
char g_DebugFuncs[1024];
static char *g_DebugFileArray[MAX_FILE_NUM];
static char *g_DebugFuncArray[MAX_FILE_NUM];
char g_ProgramName [1024];
int g_VerbosityLevel;
bool g_SleepOnCrash;
char s_Basename[1024];

#define DEBUG_STREAM cerr

DebugStream __g_DebugStream;
static bool DebugInit = false;
static pthread_mutex_t DBG_Lock;
static ofstream devnull;

static int COLORS[] = { 31, 32, 33, 34, 35, 36, /*37 too light */ };
static int FILECOLORS [] = { 31, 32, 33, 34, 35 };

static int GetColor(const char *file) {
    static hash<const char *> strhash;
    static int ncolors = (sizeof(FILECOLORS)/sizeof(int));
    int index = strhash(file) % ncolors;
    return FILECOLORS[index];
}

void PrintLine (ostream& os, char c, int cols)
{
    for (int i = 0; i < cols; i++)
	os << c;
    os << endl;
}

static void DebugLock()
{
    if (DebugInit)
	pthread_mutex_lock(&DBG_Lock);
}

static void DebugUnlock()
{
    if (DebugInit)
	pthread_mutex_unlock(&DBG_Lock); 
}

bool __DebugThisFile(const char *file) {
    if (file && g_DebugFileArray[0] != NULL) {
	for (int i=0; g_DebugFileArray[i] != NULL; i++) {
	    if (strstr(file, g_DebugFileArray[i])) {
		return true;
	    }
	}
	return false;
    }
    return true;
}

bool __DebugThisFunc(const char *func) {
    if (func && g_DebugFuncArray[0] != NULL) {
	for (int i=0; g_DebugFuncArray[i] != NULL; i++) {
	    if (strstr(func, g_DebugFuncArray[i])) {
		return true;
	    }
	}
	return false;
    }
    return true;
}

#define DO_SHOW_TIME() do { \
    OS::GetCurrentTime(&now);    \
	DEBUG_STREAM << now << " "; } while (0)

ostream& DebugStream::operator() (int mode, int lvl, const char *file, 
				  const char *func, int line) {
    TimeVal now;

    if (mode == DBG_MODE_DEBUG && lvl > g_VerbosityLevel)
	return devnull;

    if (mode == DBG_MODE_DEBUG && !__DebugThisFile(file)) {
	return devnull;
    }

    if (mode == DBG_MODE_DEBUG && !__DebugThisFunc(func)) {
	return devnull;
    }

    //	DebugLock();

    int color = GetColor(file);
    char *bn = strrchr (file, '/');
    if (bn != NULL) 
	strcpy (s_Basename, bn + 1);
    else
	strcpy (s_Basename, file);

    switch (mode) {
    case DBG_MODE_DEBUG: {

	if (DBG_SHOWTIME) {
	    DO_SHOW_TIME();
	}
	if (DBG_SHOWLINE) {
	    DEBUG_STREAM << "[" << color << "m" << s_Basename << ":" 
			 << func << ":" << line << "[m ";
	}
	break;
    }
    case DBG_MODE_WARN:
	if (DBG_SHOWTIME) {
	    DO_SHOW_TIME();
	}
	DEBUG_STREAM << "[30;41m[EE][m "  
		     << "[" << color << "m" << s_Basename << ":" 
		     << func << ":" << line << "[m ";
	break;
    case DBG_MODE_INFO:
	if (DBG_SHOWTIME) {
	    DO_SHOW_TIME();
	}
	DEBUG_STREAM << "[30;42m[II][m "
		     << "[" << color << "m" << s_Basename << ":" 
		     << func << ":" << line << "[m ";
	break;
    default:
	Debug::die("unknown debug mode %s called at %s:%s:%d", 
		   mode, s_Basename, func, line);
    }

    //DEBUG_STREAM.flush();

    // XXX This doesn't really make it thread-safe... the returned stream
    // is probably still going to be used to put stuff on this line...
    //	DebugUnlock();

    return DEBUG_STREAM;
}

void DBG_CrashHandler(int sig)
{
    char buf[255];

    DEBUG_STREAM.flush();

    //	DebugLock(); // This prevents other threads from outputting debug msgs

    fprintf(stderr, "-------------------------------------------------------------------------------\n");

    char *signame;
    switch(sig) {
    case SIGSEGV:
	signame = "SIGSEGV";
	break;
    case SIGBUS:
	signame = "SIGBUS";
	break;
    case SIGABRT:
	signame = "SIGABRT";
	break;
    case SIGFPE:
	signame = "SIGFPE";
	break;
    default:
	signame = "UNKNOWN";
	break;
    };

    // must unregister this now, since we abort() to exit
    signal(SIGABRT, SIG_DFL);

    fprintf(stderr, "[31m%s[m at:\n", signame);
    StackTrace();

    if (g_SleepOnCrash) {
	fprintf(stderr, "\nSleeping for 30 minutes. Press Ctrl-C to quit.\nTo debug, run: gdb %s %d\n", g_ProgramName, getpid());
	fprintf(stderr, "-------------------------------------------------------------------------------\n");
	sleep(30*60);
    }
    //Debug::die("Exiting");
    fprintf(stderr, "Exiting\n");
    abort();
}

void DBG_INIT(void *args) {
    if (DebugInit) 
	return;

    devnull.open("/dev/null");
    // gettimeofday(&g_StartTime, NULL);
    // g_StartTime.tv_sec = 100000000;

    StackTraceInit(g_ProgramName, 2);
    if ( signal(SIGSEGV, DBG_CrashHandler) == SIG_ERR ) {
	Debug::die("failed to install crash handler: %s", strerror(errno));
    }
    if ( signal(SIGBUS, DBG_CrashHandler) == SIG_ERR ) {
	Debug::die("failed to install crash handler: %s", strerror(errno));
    }
    if ( signal(SIGFPE, DBG_CrashHandler) == SIG_ERR ) {
	Debug::die("failed to install crash handler: %s", strerror(errno));
    }
    if ( signal(SIGABRT, DBG_CrashHandler) == SIG_ERR ) {
	Debug::die("failed to install crash handler: %s", strerror(errno));
    }

    int index = 0;
    char *str = strtok(g_DebugFiles, ",;:");
    while (str != NULL) {
	g_DebugFileArray[index++] = str;
	if (index == MAX_FILE_NUM-1) break;
	str = strtok(NULL, ",;:");
    } 
    g_DebugFileArray[index] = NULL;

    index = 0;
    str = strtok(g_DebugFuncs, ",;:");
    while (str != NULL) {
	g_DebugFuncArray[index++] = str;
	if (index == MAX_FILE_NUM-1) break;
	str = strtok(NULL, ",;:");
    }
    g_DebugFuncArray[index] = NULL;

    //	pthread_mutex_init(&DBG_Lock, NULL);

    DebugInit = true;
    //ios::sync_with_stdio(false);
    //DEBUG_STREAM.setf(ios::unitbuf);
}

namespace Debug {
    void msg (char *str, ...) 
    {
	va_list args;
	static char buf [4096];

	va_start (args, str);
	sprintf (buf, "[32m[II][m: ");
	vsprintf (buf + strlen (buf), str, args);
	DEBUG_STREAM << buf << endl;
	DEBUG_STREAM.flush ();

	va_end (args);
    }

    void warn( char* str, ... ) {
	va_list     args;
	static char buf[4096];

	va_start(args, str);
	sprintf(buf, "[31m[EE][m: ");
	vsprintf(buf + strlen(buf), str, args);
	DEBUG_STREAM << buf << endl;
	DEBUG_STREAM.flush();

	va_end(args);
    }

    void die(char *str, ... ) {
	if (str == NULL) {
	    str = "died";
	}
	va_list args;
	va_start( args, str );
	fprintf( stderr, "[31m[FF][m: " );
	vfprintf( stderr, str, args );
	fprintf(stderr, "\n");
	va_end( args );

	// _exit(2);
	// StackTrace ();
	abort();	
    }

    void die_no_abort(char *str, ...) {
	if (str == NULL) {
	    str = "died";
	}
	va_list args;
	va_start(args, str);
	fprintf(stderr, "[31m[FF][m: ");
	vfprintf(stderr, str, args);
	fprintf(stderr, "\n");
	va_end(args);

	_exit(2);
    }
    
    char *GetFormattedTime () { 
	static char buf[16];
	static struct tm * timeinfo ;
	static time_t tval;

	time (&tval);	
	timeinfo = localtime (&tval);
	strftime (buf, sizeof (buf), "%H:%M:%S", timeinfo);
	return buf;
    }

    // report available memory in megabytes
    unsigned int FreeMem() {
	unsigned long long free_mem;
	FILE *fp = fopen("/proc/meminfo", "r");
	if (!fp) { ASSERT(false); }

	char buf[80];
	fgets (buf, sizeof(buf), fp);
	fgets (buf, sizeof(buf), fp);

	int ret = sscanf (buf, "Mem: %*d %*d %lld", &free_mem);
	if (ret > 0) {
	    free_mem /= 1024;
	    free_mem /= 1024;
	} else {
	    // Jeff: 2.6 kernels don't appear to have this line.
	    // Fortunately, the second line is still 'MemFree'
	    ret = sscanf (buf, "MemFree: %lld kB", &free_mem);
	    if (ret > 0) {
		free_mem /= 1024;
	    } else {
		free_mem = 0;
	    }
	}
	fclose (fp);

	return (unsigned int) free_mem;
    }

    void PrintUDPStats (ostream& os) {
	static int rcvd_old = -1, dropped_old = -1;
	static int rcvd = -1, dropped = -1;
	// static TimeVal last, now;
	// static sint64  elapsed;

	static char buf[80];

	FILE *fp = fopen ("/proc/net/snmp", "r");
	while (fgets (buf, sizeof (buf), fp) != NULL) {
	    // os << "buf is " << buf << endl;
	    if (!strstr (buf, "Udp:")) 
		continue;
	    if (strstr (buf, "InData"))
		continue;

	    // OS::GetCurrentTime (&now);

	    sscanf (buf, "Udp: %d %*d %d %*d", &rcvd, &dropped);
	    if (rcvd_old != -1) { 

		// elapsed = now - last;

		os << (rcvd - rcvd_old) << " pkts received; "
		   << (dropped - dropped_old) << " pkts dropped." << endl;
	    }

	    dropped_old = dropped;
	    rcvd_old = rcvd;
	    // last = now;
	}
	fclose (fp);
    }
};

/////////////////////////////////////////////////////////////////////
// ashwin
char *merc_va(char *format, ...)
{
    va_list		argptr;
    static char	string[8192];

    va_start (argptr, format);
    vsprintf (string, format,argptr);
    va_end (argptr);

    return string;	
}

/////////////////////////////////////////////////////////////////////
void assertion(const char *file, const char *func, 
	       int line, const char *cond, ...) {
    va_list     args;
    char buf[255];

    // must unregister this now, since we abort() to exit
    signal(SIGABRT, SIG_DFL);

    DEBUG_STREAM.flush();

    //	DebugLock(); // This prevents other threads from outputting debug msgs

    fprintf(stderr, "-------------------------------------------------------------------------------\n");
    va_start( args, cond );
    vsprintf( buf, cond, args );
    va_end( args );

    fprintf(stderr, "Assertion ([31m%s[m) failed at %s:%s:%d\n", buf, file, func, line);
    StackTrace();
    if (g_SleepOnCrash) {
	fprintf(stderr, "\nSleeping for 30 minutes. Press Ctrl-C to quit.\nTo debug, run: gdb %s %d\n", g_ProgramName, getpid());
	fprintf(stderr, "-------------------------------------------------------------------------------\n");

	// loop since sleep may be interrupted by attaching a debugger
	TimeVal now;
	OS::GetCurrentTime(&now);
	TimeVal stop_time = now + 30*60*MSEC_IN_SEC;
	do {
	    sleep(MAX(stop_time - now, 0));
	    OS::GetCurrentTime(&now);
	} while (now < stop_time);
    }
    //Debug::die("Exiting");
    fprintf(stderr, "Exiting\n");
    abort();
    /*
      fprintf(stderr, "-------------------------------------------------------------------------------\n");
      abort();
    */
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
