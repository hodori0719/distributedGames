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

#ifndef __EXPLOG_H__
#define __EXPLOG_H__

#include <cstdio>
#include <map>
#include <list>
#include <fcntl.h>
#include <util/OS.h>
#include <mercury/ID_common.h>
#include <util/types.h>
#include <util/TimeVal.h>
#include <util/Benchmark.h>
#include <util/Thread.h>
#include <util/Mutex.h>
#include <util/OS.h>

    // If the application has multiple threads that could be logging to the same
    // log at the same time, you must define this to enable locking on logs...
    // Might be a performance damper unfortunately. :(
    //
    //#define HAVE_MULTIPLE_APP_THREADS
    //

    ///////////////////////////////////////////////////////////////////////////////
    //
    // MACROS used to declare new logs. These MUST be used instead of "new"
    // etc. otherwise there is no guarantee they will be flushed when 
    // ExpLogFlushAll() is called. We need this mess because of templatized
    // types, and we need templatized types for efficiency.
    //

    //
    // Put this in the header where the log is declared. Equiv to:
    //
    // extern ExpLog<Type> *g_LogName;
    //
#define DECLARE_EXPLOG(LogName, Type)                                \
extern ExpLog<Type> * g_##LogName;                              \
    extern __ExpLogFuncs  __g_##LogName##Funcs;                     \
    extern bool __g_##LogName##CreatedFlag;                         \
    extern ExpLog<Type> *__##LogName##Create(char *name, SID& sid);

    //
    // Put this in some file. It doesn't really matter. This is the implementation
    // of the log that was declared. Equiv to:
    //
    // ExpLog<Type> *g_LogName = NULL;
    //
#define IMPLEMENT_EXPLOG(LogName, Type)                          \
ExpLog<Type> *g_##LogName = NULL;                           \
    __ExpLogFuncs  __g_##LogName##Funcs;                        \
    bool __g_##LogName##CreatedFlag = false;                    \
    \
    static void __##LogName##FlushFunc() {                      \
	g_##LogName->Flush();                                  \
    }                                                           \
ExpLog<Type> *__##LogName##Create(char *name, SID& sid) {   \
    ASSERT(! __g_##LogName ##CreatedFlag );                \
	__g_##LogName ##CreatedFlag = true;                    \
	ExpLog<Type> *ptr = new ExpLog<Type>(sid, name);       \
	__g_##LogName##Funcs.Flush = __##LogName##FlushFunc;   \
	__ExpLogRegister(#LogName, &__g_##LogName##Funcs);     \
	return ptr;                                            \
}                                                

//
// Put this where you would have called "new" to create the log. Equiv to:
//
// g_LogName = new ExpLog<Type>("LogName", Sid)
//
#define INIT_EXPLOG(LogName, Sid)   \
(g_##LogName = __##LogName##Create(#LogName, Sid))

//
// Call this to log a new entry to the log. Equiv to:
//
// g_LogName->Log(Entry)
//
#define LOG(LogName, args...)   \
if (g_MeasurementParams.enabled) (g_##LogName->Log(args))

//
// Force the flush of the log. Equiv to:
//
// g_LogName->Flush()
//
#define FLUSH(LogName)   \
if (g_MeasurementParams.enabled) (g_##LogName->Flush())

//
// Flush all logs. Call before exiting.
//
#define FLUSH_ALL()   \
if (g_MeasurementParams.enabled) (ExpLogFlushAll())

//
// Flush rotate all logs. Implies flushing all pending todo in the dumper.
// Returns true if succeeded. Otherwise some logs failed to rotate.
//
#define ROTATE_ALL()   \
(ExpLogRotateAll())

///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    void (*Flush)(void);
} __ExpLogFuncs;

extern void __ExpLogRegister(char *name, __ExpLogFuncs *funcs);

//
// Flush all logs. Should call this before exit!
//
extern void ExpLogFlushAll();
extern bool ExpLogRotateAll();

///////////////////////////////////////////////////////////////////////////////

// This command is used to gzip rotated logs if enabled
#define GZIP_COMMAND "nice -n 19 gzip -f"

typedef struct {
    bool enabled;         /* enable logging? */
    uint32 bufferSize;    /* num ExpLogEntries */
    uint32 flushInterval; /* ms */

    uint32 maxLogSize;    /* bytes before rotation */
    bool   gzipLog;       /* gzip rotated logs; requires gzip program */
    char   dir[255];      /* directory to place logs; "" = . */
    bool   binaryLog;     /* write some high overhead logs in binary format */

    bool   aggregateLog;
    uint32 aggregateLogInterval;
    char   selEnable[255]; // list of selectively enabled logs
    char   sampleRates[255]; // log=rate/size list of sample rates + sizes
} ExpLogParams;

//const int EXPLOG_OUTPUT_BUFFER_SIZE = 16384; /* 16K */

extern ExpLogParams g_MeasurementParams;

///////////////////////////////////////////////////////////////////////////////

/**
 * Superclass of all "entries" for measurement logs. Each measurement log
 * has one subtype of these which parameterize what it records and how it
 * gets written out to disk.
 */
struct ExpLogEntry {
    TimeVal time;
    // very important that this is virtual! we delete this in-place without
    // knowledge of the subclass
    virtual ~ExpLogEntry() {}

    /** 
     * @param rate the configured sample rate [0,1.0]
     * @param size the configured sample size (semantics defined by each log)
     * @return true if we should sample this entry 
     */
    virtual bool Sample(float rate, float size) {
	if (rate >= 1) return true;
	return drand48() <= rate;
    }
 
    /** @return number of bytes written */
    virtual uint32 Dump(FILE *fp) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct ExpLogTodo {
    ExpLogEntry  *log;  // array of entries
    uint32        size; // size of each entry
    uint32        len;  // number of entries in array
};

struct ExpLogInfo {
    Mutex m_Lock;
    list< ExpLogTodo > m_Todo;

    char    m_Filename[255];
    TimeVal m_StartTime;
    FILE   *m_Out;
    uint32  m_BytesWritten;

    inline void Lock() {
	ASSERT( m_Lock.Acquire() == 0 );
    }

    inline void Unlock() {
	ASSERT( m_Lock.Release() == 0 );
    }

    ExpLogInfo(char *filename);
    virtual ~ExpLogInfo();

    bool HasTodo();
    void Flush(ExpLogTodo& todo);
    void Flush();
    bool OpenLog();
    bool RotateLog();
    void Add(ExpLogTodo& todo);
};

typedef map<char *, ExpLogInfo *> ExpLogInfoMap;
typedef ExpLogInfoMap::iterator ExpLogInfoMapIter;

class ExpLogDumperThread : public Thread
{
 private:

    Mutex m_Lock;
    ExpLogInfoMap m_LogInfo;

    inline void Lock() {
	ASSERT( m_Lock.Acquire() == 0 );
    }

    inline void Unlock() {
	ASSERT( m_Lock.Release() == 0 );
    }

 public:

    ExpLogDumperThread();
    ~ExpLogDumperThread();

    void Run();

    //
    // These methods are thread-safe
    //
    void BeginLog(char *filename);
    void Add(char *filename, ExpLogEntry *log, uint32 size, uint32 len);
    void Flush();
    bool Rotate();
};

extern ExpLogDumperThread *g_ExpLogDumper;

extern bool ExpLogIsEnabled(const char *logname);
extern pair<float,float> ExpLogSampleParams(const char *logname);

///////////////////////////////////////////////////////////////////////////////

/**
 * This class represents one measurement log which comprises elements of
 * subclass ExpEntryLog. The class is parameterized so that it can preallocate
 * a buffer of elements. ExpEntryLog subclasses dictate how the results
 * eventually get written to disk.
 *
 * Flushing occurs in two stages:
 *
 * (1) Every flushInterval/2, we swap the in-memory buffer to the dumper
 *     thread.
 *
 * (2) Then every flushInterval, the dumper thread writes the pending
 *     output of every log to disk.
 */
template <class EntryT /* ExpLogEntry */>
class ExpLog {
 private:

    bool          m_Enabled;
    float         m_SampleRate;
    float         m_SampleSize;

    char          m_Filename[255];
    EntryT       *m_ExpLogBuffer;
    uint32        m_ExpLogIndex;
    Mutex         m_Lock;

    TimeVal       m_ExpLogLastTime;

    void Flush(TimeVal& time)
	{
	    if (!m_Enabled)
		return;
	    
	    ASSERT(g_MeasurementParams.enabled);
	    ASSERT(m_ExpLogIndex <= g_MeasurementParams.bufferSize);

	    // pass to other thread for flushing to disk
	    g_ExpLogDumper->Add(m_Filename, m_ExpLogBuffer, 
				sizeof(EntryT), m_ExpLogIndex);

	    // alloc new buffer for writing
	    m_ExpLogBuffer    = 
		(EntryT *)new byte[sizeof(EntryT)*g_MeasurementParams.bufferSize];
	    m_ExpLogIndex     = 0;
	    m_ExpLogLastTime  = time;
	}

    inline void Lock() {
#ifdef HAVE_MULTIPLE_APP_THREADS
	m_Lock.Acquire();
#endif
    }

    inline void Unlock() {
#ifdef HAVE_MULTIPLE_APP_THREADS
	m_Lock.Release();
#endif
    }

 public:

    ExpLog(SID sid, char *logname)
	{
	    ASSERT(g_MeasurementParams.enabled);

	    if (!ExpLogIsEnabled(logname)) {
		m_Enabled = false;
		return;
	    } else {
		m_Enabled = true;
	    }

	    pair<float,float> param = ExpLogSampleParams(logname);
	    m_SampleRate = param.first;
	    m_SampleSize = param.second;

	    Lock();

	    // start global dumper thread if not started
	    if (g_ExpLogDumper == NULL) {
		g_ExpLogDumper = new ExpLogDumperThread();
		g_ExpLogDumper->Start();
	    }

	    if (!strcmp(g_MeasurementParams.dir, "")) {
		sprintf(m_Filename, "%s.%s.log", logname, sid.ToString());
	    } else {
		sprintf(m_Filename, "%s/%s.%s.log", g_MeasurementParams.dir,
			logname, sid.ToString());
	    }

	    g_ExpLogDumper->BeginLog(m_Filename);

	    m_ExpLogBuffer   = 
		(EntryT *)new byte[sizeof(EntryT)*g_MeasurementParams.bufferSize];
	    m_ExpLogIndex    = 0;
	    m_ExpLogLastTime = TIME_NONE;

	    Unlock();
	}

    virtual ~ExpLog() {
	if (!m_Enabled)
	    return;
	delete[] m_ExpLogBuffer;
    }

    /**
     * Force flush of the remainder of this log to disk. This method blocks.
     */
    void Flush()
	{
	    if (!m_Enabled)
		return;
	    TimeVal now;
	    OS::GetCurrentTime(&now);
	    Flush(now);
	    g_ExpLogDumper->Flush();
	}

    /**
     * Log a new entry.
     */
    inline void Log(EntryT& ent, TimeVal t = TIME_NONE)
	{
	    if (!m_Enabled)
		return;

	    ASSERT(g_MeasurementParams.enabled);

	    TimeVal now;

	    OS::GetCurrentTime(&now);
	    if (t == TIME_NONE)
		ent.time = now;
	    else
		ent.time = t;

	    if (!ent.Sample(m_SampleRate, m_SampleSize))
		return;

	    Lock(); // Jeff: Yuck!
	    if ((m_ExpLogLastTime + 
		 g_MeasurementParams.flushInterval/2) <= now ||
		m_ExpLogIndex >= g_MeasurementParams.bufferSize) {
		Flush(now);
	    }
	    // not exactly sure why this is required since the copy constructor
	    // should copy the vtable ptr.... right? apparently not?
	    new(m_ExpLogBuffer + m_ExpLogIndex) EntryT;
	    m_ExpLogBuffer[m_ExpLogIndex] = ent;
	    ++m_ExpLogIndex;
	    Unlock();
	}
};

///////////////////////////////////////////////////////////////////////////////

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
