%module mercsim
%{
#include <Mercury.h>
#include <mercury/BootstrapNode.h>
#include <mercury/MercuryNode.h>
#include <sim-env/Simulator.h>
#include <om/OMEvent.h>
#include <om/OMInterest.h>
#include <om/OMMessage.h>
#include <mercury/ObjectLogs.h>
#include <util/Options.h>
#include <mercury/Parameters.h>
#include <mercury/HubManager.h>
%}

struct IPEndPoint {
    uint32   m_IP;     // network byte order
    uint16   m_Port;   // host byte order 

    IPEndPoint() : m_IP(0), m_Port(0) {}
    IPEndPoint(uint32 address, int port);
    IPEndPoint(int port); // figure out my own IP address...

    // WARNING: these constructors block on a DNS lookup
    // They construct SID_NONE if the name fails to resolve
    IPEndPoint(char *addrport); // name:port string 
    IPEndPoint(char *addr, int port);
    void ConstructFromString(char *addr, int port);

    uint32 NetworkToHostOrder() { return ntohl(m_IP); }
    bool IsLoopBack() { return m_IP == LOOPBACK_ADDRBITS; }

    inline uint32 GetIP() const { return m_IP; }
    inline uint16 GetPort() const { return m_Port; }
    
    static uint32 Parse(char *ip);
 
    void ToString(char *bufOut) const;
    char *ToString() const;

    IPEndPoint(Packet *pkt);
    void Serialize(Packet *pkt);
    uint32 GetLength() {
	return sizeof(m_IP) + sizeof(m_Port);
    }
    void Print(FILE *stream) {
	fprintf(stream, "%s", ToString());
    }

    %extend { 
	const char *__str__() {
	    return (const char *) self->ToString ();
	}
    }
};

/*
struct _bootstrap_prefs_t {
    bool haveHistograms;
    char schemaFile[1024];
    int  nBuckets;
    int  nServers;
};

extern struct _bootstrap_prefs_t g_BootstrapPreferences;
*/

%include "../mercury/ID_common.h"
%include "../mercury/Parameters.h"

class Node {
protected:
    NetworkLayer  *m_Network;
    Scheduler     *m_Scheduler;
    IPEndPoint     m_Address;
public:
    Node (NetworkLayer *network, Scheduler *scheduler, IPEndPoint& addr) :
	m_Network (network), m_Scheduler (scheduler), m_Address (addr)  
	{}
    
    NetworkLayer *GetNetwork () { return m_Network; }
    Scheduler *GetScheduler () { return m_Scheduler; }
    IPEndPoint& GetAddress () { return m_Address; }

    virtual void ReceiveMessage (IPEndPoint *from, Message *msg) = 0;
};

%include "../mercury/common.h"
%include "../mercury/BootstrapNode.h"
typedef unsigned long u_long;

// Grab a Python function object as a Python object.
%typemap(python,in) PyObject *pyfunc {
    if (!PyCallable_Check($input)) {
	PyErr_SetString(PyExc_TypeError, "Need a callable object!");
	ASSERT (false);
	return NULL;
    }
    $1 = $input;
}

// This tells SWIG to treat char ** as a special case
%typemap(in) (int argc, char **argv) {
    /* Check if is a list */
    if (PyList_Check($input)) {
	int i;
	$1 = PyList_Size($input);
	$2 = (char **) malloc(($1+1)*sizeof(char *));
	for (i = 0; i < $1; i++) {
	    PyObject *o = PyList_GetItem($input,i);
	    if (PyString_Check(o))
		$2[i] = PyString_AsString(PyList_GetItem($input,i));
	    else {
		PyErr_SetString(PyExc_TypeError,"list must contain strings");
		free($2);
		return NULL;
	    }
	}
	$2[i] = 0;
    } else {
	PyErr_SetString(PyExc_TypeError,"not a list");
	return NULL;
    }
}

%typemap(freearg) (int argc, char **argv) {
    free((char *) $2);
}

/*
%typemap(python,in) Node& addingnode {
    Py_INCREF ($input);
    $1 = $input;
}

%typemap(python,in) Node& removingnode {
    Py_DECREF ($input);
    $1 = $input;
}
*/

%{

class SpEvent : public SchedulerEvent {
    PyObject     *m_PyFunc;
public:
    SpEvent (PyObject *f) : m_PyFunc (f) {
	Py_INCREF (m_PyFunc);
    }
    
    ~SpEvent () { 
	Py_DECREF (m_PyFunc);
    }
    
    virtual void Execute (Node& node, TimeVal& timenow) {
	PyObject *arglist;
	
	// how do you convert this thing back to python objects??
	PyObject *arg1 = SWIG_NewPointerObj ((void *) &node, SWIGTYPE_p_Node, 0);
	PyObject *arg2 = SWIG_NewPointerObj ((void *) &timenow, SWIGTYPE_p_TimeVal, 0);

	arglist = PyTuple_New (2);
	PyTuple_SetItem (arglist, 0, arg1);
	PyTuple_SetItem (arglist, 1, arg2);
	PyEval_CallObject (m_PyFunc, arglist);

	// free arglist??
    }
};

%}

%include "../util/Options.h"
extern OptionType *g_MercuryOptions;

%inline %{

struct _driver_prefs_t 
{
    int nodes;
    int inter_arrival_time;
};

struct _driver_prefs_t g_DriverPrefs;

OptionType g_DriverOptions [] = 
{
    { '#', "nodes", OPT_INT, "number of mercury nodes in the simulation",
	&g_DriverPrefs.nodes, "4" , NULL },
    { '#', "arrive-int", OPT_INT, "inter-arrival interval (milliseconds)",
	&g_DriverPrefs.inter_arrival_time, "100" , NULL },
    { 0, 0, 0, 0, 0, 0, 0 }
};

OptionType *g_TrulyAllOptions;

void init_environment (int argc, char **argv)
{
    SID foo ("gs203.sp.cs.cmu.edu:65535");
    
    strcpy (g_ProgramName, *argv);
    DBG_INIT (&SID_NONE);

    // merge options
    OptionType **all_options = new OptionType *[2];
    all_options[0] = g_DriverOptions;
    all_options[1] = g_BootstrapOptions;
    
    g_TrulyAllOptions = MergeOptions (all_options, 2);
    delete[] all_options;

    // initialize mercury
    InitializeMercury (&argc, argv, g_TrulyAllOptions, false);

    OM_RegisterEventTypes ();
    OM_RegisterInterestTypes ();
    OM_RegisterMessageTypes ();

    InitObjectLogs (foo);
}

#if 0
OptionType *add_option (char shortword, char *longword, int flags, char *comment,
	char *varname, char *default_val, void *val_to_set)
{

    if (flags & OPT_STR) {
    }
    else if (flags & OPT_INT) {
    }
}
#endif

PyObject *S_Py_ModuleLatencyFunc;

static u_long S_ModuleLatencyFunc (IPEndPoint& start, IPEndPoint& end)
{
    return 0;
}
%}

struct TimeVal {

};

%extend TimeVal {
    const char *__str__ () {
	static char buf [32];
	sprintf(buf, "%.3f", (float) self->tv_sec + ((float) self->tv_usec/USEC_IN_SEC));
	return buf;
    }
};

typedef u_long (*LatencyFunc) (IPEndPoint& start, IPEndPoint& end);

class Simulator : public NetworkLayer, public Scheduler {
public:
    Simulator ();
    virtual ~Simulator ();
    static const int NODE_TO_NODE_LATENCY = 50;     // 50 milliseconds

    void AddNode (Node& addingnode);
    void RemoveNode (Node& removingnode);

    void SetLatencyFunc (LatencyFunc func) { m_LatencyFunc = func; }

    //============================================================================
    /////// Networklayer 
    virtual int SendMessage(Message *msg, IPEndPoint *toWhom, TransportType proto);
    
    //============================================================================
    //////// Scheduler functionality
    
    virtual void ProcessFor (u_long millis);
    virtual TimeVal& TimeNow () { return m_CurrentTime; }

    %extend {
	void add_task (PyObject *pyfunc, IPEndPoint& address, u_long millis) {
	    self->RaiseEvent (new refcounted<SpEvent> (pyfunc), address, millis);
	}
	void add_sim_task (PyObject *pyfunc, u_long millis) {
	    self->RaiseEvent (new refcounted<SpEvent> (pyfunc), SID_NONE, millis);
	}

	void set_latency_func (PyObject *pyfunc) {
	    S_Py_ModuleLatencyFunc = pyfunc;
	    self->SetLatencyFunc (S_ModuleLatencyFunc);
	}
    }
};

class MessageHandler {
 public:
    virtual void ProcessMessage(IPEndPoint *from, Message *msg) = 0;
};

class Value {
 public: 
    Value (const char *s, int base = 10);
    %extend {
	char *__str__ () {
	    static char t[256];
	    gmp_sprintf (t, "%Zd", (MP_INT *) self);
	    return t;
	}
    }   
};

class Constraint {
public:
    Constraint (int attr, Value min, Value: max);
    %extend {
	char *__str__ () {
	    static char t[256];
	    sprintf (t, "%s:[", G_GetTypeName (self->GetAttrIndex ()));
	    gmp_sprintf (t + strlen (t), "%Zd,%Zd]", (const MP_INT *) &self->GetMin (), (const MP_INT *) &self->GetMax ());
	    return t;
	}
    }
};

class Tuple {
public:
    Tuple (int attr, Value v);
    %extend {
	char *__str__ () {
	    static char t[256];
	    sprintf (t, "%s=[", G_GetTypeName (self->GetAttrIndex ()));
	    gmp_sprintf (t + strlen (t), "%Zd", (const MP_INT *) &self->GetValue ());
	    return t;
	}
    }
};

/*
struct GUID {
    uint32 m_IP;
    uint16 m_Port;
    uint32 m_LocalOID;
    
    GUID();
    GUID(uint32 ip, uint16 port, uint32 localOID);
    GUID (IPEndPoint ipport, uint32 localOID);

    static GUID CreateRandom();
};

typedef GUID guid_t;
typedef IPEndPoint SID;
typedef SID sid_t;
*/

class OMEvent {
public: 
    OMEvent (guid_t g, sid_t loc);
    void AddTuple(const Tuple& t);
};

class PointEvent {
public:
    PointEvent ();
    void AddTuple (Tuple& t);
};


class OMInterest {
public:
    OMInterest (guid_t g);
    void AddConstraint (Constraint& c);
};

class MercuryNode : public Node, public MessageHandler {
    friend class MercuryNodePeriodicTimer;

private:
    BufferManager         *m_BufferManager;
    HubManager            *m_HubManager;
    bool                   m_AllJoined;        // bootstrap tells us when everyone is joined
    
    int                    m_Epoch;            // repair epoch
    HandlerMap             m_HandlerMap;

    
    MercuryNode           *m_MercuryNode;      // hack for debug
public:
    MercuryNode(NetworkLayer *network, Scheduler *scheduler, IPEndPoint& addr);
    ~MercuryNode();

    void Start ();
    void Stop ();

    bool IsJoined();
    bool AllJoined() { return m_AllJoined; }
    uint32 GetIP() { return m_Address.GetIP(); }
    uint16 GetPort() { return m_Address.GetPort(); }
    
    void SendEvent (Event *pub);
    void RegisterInterest (Interest *sub);
    Event* ReadEvent ();

    void LockBuffer() { ASSERT(0); }
    void UnlockBuffer() { ASSERT(0); }
    
    void Print(FILE *stream);

    virtual void DoWork() {}      

    void PrintSubscriptionList (ostream& stream);
    void PrintPublicationList (ostream& stream);

    
    void RegisterMessageHandler(MsgType type, MessageHandler *handler);
	
    virtual void ReceiveMessage (IPEndPoint *from, Message *msg);
    void ProcessMessage (IPEndPoint *from, Message *msg);

    %extend {
	void PrintRanges () {
	    vector<Constraint> vc =  self->GetHubRanges ();

	    if (vc.size () == 0) {
		cerr << " no range information yet " << endl;
		return;
	    }
	    for (vector<Constraint>::iterator it = vc.begin(); it != vc.end(); it++) {
		Constraint c = *it;
		cerr << g_MercuryAttrRegistry[c.GetAttrIndex()].name << 
		    " [" << c.GetMin () << "," << c.GetMax () << "]" << endl;
	    }
	}
    }
    ostream& croak (int mode, int lvl = 0, const char *file = NULL, const char *func = NULL, int line = 0);
private:
    
    void DoPeriodic ();
    void HandleAllJoined(IPEndPoint *from, MsgCB_AllJoined *msg);
    void SendApplicationPackets( void );
    void PrintPeerList(FILE *stream);
};

