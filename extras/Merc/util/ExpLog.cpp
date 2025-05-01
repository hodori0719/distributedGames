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

/**************************************************************************
  ExpLog.h

begin           : Nov 8, 2003
version         : $Id: ExpLog.cpp 2457 2005-11-09 21:20:10Z jeffpang $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu )
(C) 2003      Justin Weisz     (  jweisz@cs.cmu.edu  )

***************************************************************************/

#include <list>
#include <set>
#include <map>
#include <string>
#include <util/ExpLog.h>

ExpLogDumperThread *g_ExpLogDumper = NULL;

ExpLogParams g_MeasurementParams;

///////////////////////////////////////////////////////////////////////////////

typedef map<string, __ExpLogFuncs *, less<string> > ExpLogFuncMap;
typedef ExpLogFuncMap::iterator ExpLogFuncMapIter;

static ExpLogFuncMap g_LogFuncMap;

void __ExpLogRegister(char *name, __ExpLogFuncs *funcs)
{
    string key(name);
    ASSERT(g_LogFuncMap.find(name) == g_LogFuncMap.end());
    g_LogFuncMap[name] = funcs;
}

void ExpLogFlushAll()
{
    for (ExpLogFuncMapIter it = g_LogFuncMap.begin();
	 it != g_LogFuncMap.end(); it++) {
	it->second->Flush();
    }
}

bool ExpLogRotateAll()
{
    // dump all existing stuff in todo and flush
    return g_ExpLogDumper->Rotate();
}

bool ExpLogIsEnabled(const char *logname)
{
    static bool inited = false;
    static set<string> enabled;
    if (!inited) {
	char *str = strtok(g_MeasurementParams.selEnable, ",;");
	while (str != NULL) {
	    enabled.insert(string(str));
	    str = strtok(NULL, ",;");
	}
	inited = true;
    }

    if (!g_MeasurementParams.enabled) return false;

    if (enabled.size() == 0) return true;

    bool ret = enabled.find(logname) != enabled.end();

    //INFO << logname << " = " << ret << endl;

    return ret;
}

static vector<string>
split(string line, string delims = " \t")
{
    string::size_type bi, ei;
    vector<string> words;

    bi = line.find_first_not_of(delims);
    while(bi != string::npos) {
	ei = line.find_first_of(delims, bi);
	if(ei == string::npos)
	    ei = line.length();
	words.push_back(line.substr(bi, ei-bi));
	bi = line.find_first_not_of(delims, ei);
    }

    return words;
}

pair<float,float> ExpLogSampleParams(const char *logname)
{
    static bool inited = false;
    static map<string, pair<float,float> > val;
    if (!inited) {
	char *str = strtok(g_MeasurementParams.sampleRates, ";,");
	while (str != NULL) {
	    vector<string> comp = split(str, "=");
	    if (comp.size() != 2) {
		WARN << "bad sample params format: " << str << endl;
		ASSERT(0);
	    }
	    string name = comp[0];
	    vector<string> parts = split(comp[1], "/");
	    float rate, size;
	    if (parts.size() == 1) {
		rate = atof(parts[0].c_str());
		size = 1.0;
	    } else {
		rate = atof(parts[0].c_str());
		size = atof(parts[1].c_str());
	    }
	    val[name] = pair<float,float>(rate,size);

	    str = strtok(NULL, ";,");
	}
	inited = true;
    }

    if (val.find(string(logname)) == val.end()) {
	return pair<float,float>(1.0,1.0);
    } else {
	INFO << "params: " << logname << " = " << val[string(logname)].first << "," << val[string(logname)].second << endl;
	return val[string(logname)];
    }
}

///////////////////////////////////////////////////////////////////////////////

ExpLogInfo::ExpLogInfo(char *filename) {
    strcpy(m_Filename, filename);
    bool ret = OpenLog();
    ASSERT(ret);
}

ExpLogInfo::~ExpLogInfo() {
    Flush();
    fclose(m_Out);
}

void ExpLogInfo::Flush(ExpLogTodo& todo) {
    for (uint32 i=0; i<todo.len; i++) {
	if (m_BytesWritten >= g_MeasurementParams.maxLogSize) {
	    if (! RotateLog() )
		WARN << "log rotation of " << m_Filename 
		     << " failed!" << endl;
	}

	// HACK: Index into array without knowing the runtime type...
	ExpLogEntry *entry = (ExpLogEntry *)(((byte *)todo.log)+i*todo.size);
	m_BytesWritten += entry->Dump(m_Out);
	// destruct in place (don't free memory)
	entry->~ExpLogEntry();
    }
    fflush(m_Out);

    // HACK: Originally this was just allocated as a byte array, therefore
    // to free it, just cast it back to a byte array...
    // xxx: we should recycle this buffer back to the logger
    delete[] (byte *)todo.log;
}

bool ExpLogInfo::HasTodo() {
    bool hasTodo = false;
    Lock();
    hasTodo = m_Todo.size() > 0;
    Unlock();
    return hasTodo;
}

void ExpLogInfo::Flush() {
    while (m_Todo.size() > 0) {
	Lock();
	ExpLogTodo todo = m_Todo.front();
	m_Todo.pop_front();
	Unlock();
	Flush(todo);
    }
}

bool ExpLogInfo::OpenLog() {
    OS::GetCurrentTime (&m_StartTime);

    m_Out = fopen(m_Filename, "w" /*"a"*/);
    if ( ! m_Out ) {
	WARN << "error opening log file " << m_Filename 
	     << ": " << strerror(errno) << endl;
	return false;
    }
    m_BytesWritten = 0;
    return true;
}

bool ExpLogInfo::RotateLog() {
    ASSERT(m_Out);
    if ( fclose(m_Out) < 0 )
	return false;

    char oldLog[255];
    TimeVal endTime;

    OS::GetCurrentTime (&endTime);

    sprintf(oldLog, "%s.%d-%d", m_Filename, 
	    (int)m_StartTime.tv_sec, (int)endTime.tv_sec);

    if ( rename(m_Filename, oldLog) < -1 )
	return false;

    if ( g_MeasurementParams.gzipLog ) {
	char cmd[300];
	sprintf(cmd, GZIP_COMMAND " \"%s\"", oldLog);
	if ( system(cmd) )
	    WARN << "'cmd' failed on log rotation of " 
		 << m_Filename << "!" << endl;
	else
	    remove(oldLog);
    }

    return OpenLog();
}

void ExpLogInfo::Add(ExpLogTodo& todo) 
{
    Lock();
    m_Todo.push_back(todo);
    Unlock();
}

///////////////////////////////////////////////////////////////////////////////

ExpLogDumperThread::ExpLogDumperThread() {}

ExpLogDumperThread::~ExpLogDumperThread() {
    for (ExpLogInfoMapIter it = m_LogInfo.begin();
	 it != m_LogInfo.end(); it++) {
	delete it->second;
    }
}

void ExpLogDumperThread::Flush() {
    while (true) {
	// Careful with locks here
	Lock();
	// One could argue that we should start the iterator outside
	// the while loop and then iterate over each element within it
	// however that would require us to acquire the lock outside
	// the loop which would block additions to the queue in the main
	// thread. this is undesirable from the standpoint of responsiveness 
	//
	// however, one problem with this approach is that it is vulnerable
	// to starvation of the logs later in the info list if the logs
	// in front always add stuff very fast...
	ExpLogInfoMapIter it = m_LogInfo.begin();
	for ( ; it != m_LogInfo.end(); it++) {
	    if (it->second->HasTodo())
		break;
	}
	if (it == m_LogInfo.end()) {
	    Unlock();
	    return;
	} else {
	    Unlock();
	    it->second->Flush();
	}
    }
}

bool ExpLogDumperThread::Rotate() {
    Lock();
    bool ret = true;
    ExpLogInfoMapIter it = m_LogInfo.begin();
    for ( ; it != m_LogInfo.end(); it++) {
	bool ok = it->second->RotateLog();

	// force a rotation after flush
	if (!ok)
	    WARN << "log rotation of " << it->second->m_Filename 
		 << " failed!" << endl;
	
	ret = ret && ok;
    }
    Unlock();
    return ret;
}

void ExpLogDumperThread::Run() {
    while (true) {
	OS::SleepMillis( (int) ((1.0 + drand48()) * g_MeasurementParams.flushInterval));
	Flush();
    }
}

void ExpLogDumperThread::BeginLog(char *filename) {
    Lock();
    ASSERT( m_LogInfo.find(filename) == m_LogInfo.end() );
    m_LogInfo[filename] = new ExpLogInfo(filename);
    Unlock();
}

void ExpLogDumperThread::Add(char *filename, ExpLogEntry *log, 
			     uint32 size, uint32 len) {
    Lock();
    ExpLogInfoMapIter it = m_LogInfo.find(filename);
    ASSERT( it != m_LogInfo.end() );
    ExpLogInfo *info = it->second;
    Unlock();

    ExpLogTodo todo;
    todo.log  = log;
    todo.size = size;
    todo.len  = len;
    info->Add(todo);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
