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

// $Id$

#include <mercury/Hub.h>
#include <mercury/Sampling.h>

// #include <boost/random.hpp>
typedef vector<SimMercuryNode *> MNVec;
typedef MNVec::iterator MNVecIter;

class TestSampler;
vector<TestSampler *> g_Samplers;
MNVec nlist;
double g_Mean = 100.0;
double g_Std = 10.0;

float avg_load = 0.0;
/*
  float GenerateDistribution (IPEndPoint& addr, NodeRange& range)
  {
  Value v = range->GetMin ();
  v += range->GetMax ();
  v /= 2;


  }
*/

class TestSampler : public LoadSampler {
    float m_Value;

public:	    
    TestSampler (MemberHub *hub, float val) : LoadSampler (hub), m_Value (val) {
    }
    virtual ~TestSampler () {}

    const float GetValue () const { return m_Value; }
    virtual Metric *GetPointEstimate () { 
	ASSERT (m_Hub != NULL);
	if (m_Hub->GetRange () == NULL)
	    return NULL;

	// cerr << " returning value " << m_Value << " for load sampling " << endl;
	return new LoadMetric (m_Value);
    }

    void SetLoad (Metric *l) {
	MercuryNode *m_MercuryNode = m_Hub->GetMercuryNode ();
	m_Value = (float) (dynamic_cast<LoadMetric *> (l)->GetLoad ());
	MDB (-5) << "new load=" << m_Value << endl;
    }
};

class CreateNodeEvent : public SchedulerEvent {
    SimMercuryNode *m_Node;
public:
    CreateNodeEvent (SimMercuryNode *n) : m_Node (n) {}

    void Execute (Node& node, TimeVal& timenow) {
	m_Node->StartUp ();
    }
};

void PrintSamples (SimMercuryNode *self)
{
    MemberHub *mh = dynamic_cast<MemberHub *> (self->GetHubManager ()->GetHubByIndex (0));
    NodeCountSampler *nc = mh->GetNodeCountSampler ();

    vector<Sample *> samples;
    mh->GetSamples (nc, &samples);
    cerr << "--[[";
    for (vector<Sample *>::iterator it = samples.begin (); it != samples.end (); ++it) {
	if (it != samples.begin ()) cerr << "; ";
	cerr << *it;
    }
    cerr << "]]--" << endl;
}

#define PRINT_DELAY 10000

class PrintEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    PrintEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow) {
	cerr << " =====>>> Current Time=" << timenow << " <<<=====" << endl;

	for (MNVecIter it = m_Nodes->begin(); it != m_Nodes->end(); it++)  {
	    SimMercuryNode *self = *it;

	    cerr << " " << self->GetAddress () << ": ";
	    vector<Constraint> vc = self->GetHubRanges ();
	    vector<Constraint> vcm = self->GetHubConstraints ();

	    if (vc.size () == 0) {
		cerr << " no range information yet " << endl;
		continue;
	    }
	    for (vector<Constraint>::iterator ooit = vc.begin(); ooit != vc.end(); ooit++) {
		Constraint c = *ooit;
		cerr << g_MercuryAttrRegistry[c.GetAttrIndex()].name << 
		    " [" << c.GetMin () << "," << c.GetMax () << "]" ;
		cerr << "\tspan=" << c.GetSpan (vcm[0].GetMin (), vcm[0].GetMax ());
	    }

	    cerr << " succs={";
	    vector<Neighbor> succs = self->GetSuccessors (0);
	    for (vector<Neighbor>::iterator ooit = succs.begin(); ooit != succs.end (); ooit++) {
		Neighbor n = *ooit;
		cerr << n.addr.GetPort () << "; " << n.range;
	    }
	    cerr << "}" << endl;

	    cerr << " preds={";
	    vector<Neighbor> preds = self->GetPredecessors (0);
	    for (vector<Neighbor>::iterator ooit = preds.begin(); ooit != preds.end (); ooit++) {
		Neighbor n = *ooit;
		cerr << n.addr.GetPort () << "; " << n.range;
	    }
	    cerr << "}" << endl;

	    // PrintSamples (self);
	}

	g_Simulator->RaiseEvent (mkref (this), SID_NONE, PRINT_DELAY);
    }
};

class GenericPrintEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    GenericPrintEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow)
    {
	cerr << " =====>>> Current Time=" << timenow << " <<<=====" << endl;

	MNVec dupe = *m_Nodes;
	Sort (dupe);

	for (MNVecIter it = dupe.begin(); it != dupe.end(); ++it)  {
	    DoPrint (*it);
	}	
	g_Simulator->RaiseEvent (mkref (this), SID_NONE, GetDelay ());
    }
    virtual void DoPrint (SimMercuryNode *self) {}
    virtual void Sort (MNVec& v) {}
    virtual int GetDelay () { return 2000; }
};

class PrintStuffEvent : public GenericPrintEvent {
public:
    PrintStuffEvent (MNVec *n) : GenericPrintEvent (n) {}

    struct sort_by_ranges {
	bool operator() (const SimMercuryNode *sa, const SimMercuryNode *sb) const {
	    SimMercuryNode *csa = (SimMercuryNode *) sa;
	    SimMercuryNode *csb = (SimMercuryNode *) sb; 
	    NodeRange *ra = GetHub (csa)->GetRange ();
	    NodeRange *rb = GetHub (csb)->GetRange ();

	    if (ra && rb) 
		return ra->GetMin () < rb->GetMin ();
	    if (!ra && rb)
		return true;
	    if (ra && !rb)
		return false;

	    return csa->GetAddress ().GetPort () < csb->GetAddress ().GetPort ();
	}
    };

    virtual void Sort (MNVec& dupe)
    {
	sort (dupe.begin (), dupe.end (), sort_by_ranges ());
    }
    virtual void DoPrint (SimMercuryNode *self) 
    {
	LoadMetric *mine = (LoadMetric *) GetHub (self)->GetLoadSampler ()->GetPointEstimate ();
	double d = -1.0;
	if (mine)    
	    d = (double) mine->GetLoad ();
	delete mine;

	MemberHub *h = GetHub (self);
	Peer *succ = h->GetSuccessor ();

	cerr << self->GetAddress () << " range=" << h->GetRange () << " load=" << d;
#if 0
	cerr << " succs={";
	vector<Neighbor> succs = self->GetSuccessors (0);
	for (vector<Neighbor>::iterator ooit = succs.begin(); ooit != succs.end (); ooit++) {
	    Neighbor n = *ooit;
	    cerr << n.addr.GetPort () << "; " << n.range << " ";
	}
	cerr << "} ";

	cerr << " preds={";
	vector<Neighbor> preds = self->GetPredecessors (0);
	for (vector<Neighbor>::iterator ooit = preds.begin(); ooit != preds.end (); ooit++) {
	    Neighbor n = *ooit;
	    cerr << n.addr.GetPort () << "; " << n.range << " ";
	}
	cerr << "}" << endl;
#else
	if (succ)
	    cerr << " \tsucc=:" << succ->GetAddress ().GetPort () << " (" << succ->GetRange () << ")";
	Peer *pred = h->GetPredecessor ();
	if (pred)
	    cerr << " pred=:" << pred->GetAddress ().GetPort () << " (" << pred->GetRange () << ")";
	cerr << endl;
#endif
    }
};

class PrintLoadStats : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    PrintLoadStats (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow)
    {
	float max = -99, ratio = 1.0;
	float avg = 0;

	for (MNVecIter it = m_Nodes->begin(); it != m_Nodes->end(); it++)  {
	    SimMercuryNode *self = *it;

	    float load = GetLoad (self);
	    if (load > 0) { 
		avg += load;
		if (load > max) max = load;
	    }
	}

	avg /= m_Nodes->size ();

	if (avg_load > 1.0e-5)
	    ratio = max / avg_load; 

	cerr << timenow << " orig.avg=" << avg_load << " cur.avg=" << avg << " max=" << max << " ratio=" << ratio << endl;
	g_Simulator->RaiseEvent (mkref (this), SID_NONE, GetDelay ());
    }

    int GetDelay () { return 2000; }

    float GetLoad (SimMercuryNode *self) {
	LoadMetric *mine = (LoadMetric *) GetHub (self)->GetLoadSampler ()->GetPointEstimate ();
	double d = -1.0;
	if (mine)
	    d = (double) mine->GetLoad ();
	delete mine;

	return d;
    }
};


#define PRINT_GRAPH_DELAY 2000

#define ID(node)   (node)->GetAddress ().GetPort ()
#define ID2(node)  (node).addr.GetPort ()

class PrintGraphEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    PrintGraphEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow) {
	char fname [128];
	sprintf (fname, "graph.%.3f", timeval_to_float (timenow));
	ofstream os (fname);
	// stringstream buf;

	os << "digraph time" << (int) (timeval_to_float (timenow)) << " {\n";
	for (MNVecIter it = m_Nodes->begin (); it != m_Nodes->end (); it++) {
	    SimMercuryNode *self = *it;

	    vector<Neighbor> succs = self->GetSuccessors (0);
	    vector<Neighbor> lnbrs = self->GetLongNeighbors (0);

	    if (succs.size () > 0)
		os << "n" << ID(self) << " -> " << "n" << ID2(succs[0]) << "[color=red];" << endl;
	    else
		os << "n" << ID(self) << endl;

	    for (vector<Neighbor>::iterator oit = lnbrs.begin (); oit != lnbrs.end (); oit++)
		os << "n" << ID(self) << " -> " << "n" << ID2(*oit) << "[color=green];" << endl;
	}

	os << "}\n";
	os.flush ();
	os.close ();

	g_Simulator->RaiseEvent (mkref (this), SID_NONE, PRINT_GRAPH_DELAY);
    }
};

class TPEvent : public PointEvent {
protected:
    DECLARE_TYPE (Event, TPEvent);
public:
    TPEvent () : PointEvent () {}
    TPEvent (Packet *pkt) : PointEvent (pkt) {}

    virtual ostream& Print (ostream& out) const { return PointEvent::Print (out); }

    virtual bool OnRendezvousReceive (MemberHub *h, MsgPublication *pmsg) { 
#if 0
	cerr << "::: event " << this << " routed in <<<" << pmsg->hopCount  << ">>> hops " << endl;
	cerr << pmsg << endl;
#endif
	cout << "hops " << (int) pmsg->hopCount << endl;
	return false;
    }
};

class TMEvent : public MercuryEvent {
protected:
    DECLARE_TYPE (Event, TMEvent);
public:
    TMEvent () : MercuryEvent () {}
    TMEvent (Packet *pkt) : MercuryEvent (pkt) {}

    virtual ostream& Print (ostream& out) const { 
	out << " constraints=[";
	for (MercuryEvent::iterator it = MercuryEvent::begin (); it != MercuryEvent::end (); it++) {
	    if (it != MercuryEvent::begin ()) out << ",";
	    out << *it;
	}
	out << "]";
	return out;
    }

    /*
      virtual bool OnRendezvousReceive (MemberHub *h, MsgPublication *pmsg) { 
      SimMercuryNode *m_MercuryNode = h->GetMercuryNode ();
      MDB (0) << "::: event " << this << " routed in <<<" << pmsg->hopCount  << ">>> hops " << endl;
      MDB (0) << pmsg << endl;
      return false;
      }
    */
};

class SendPubsEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    SendPubsEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow) {
	int nnodes = m_Nodes->size ();
	for (int npubs = 0; npubs < 1000;  npubs++) {
	    SimMercuryNode *self = (*m_Nodes)[(int) (drand48 () * nnodes)];
	    TPEvent *tev = new TPEvent ();
	    Value v = (uint32) ( drand48 () * 10000 );		
	    Tuple t (0, v);
	    tev->AddTuple (t);

	    self->SendEvent (tev);
	    delete tev;
	}
    }
};

// create nodes 
// once everybody's joined, set everybody's "value" using the "loads" vector
// call RegisterSampler ()
// run for a few seconds. set timeouts properly. 
// select a few random nodes. query samples. compute mean.

void create_nodes (MNVec *p_nlist)
{
    for (int i = 0; i < g_DriverPrefs.nodes; i++) {
	IPEndPoint ip ("gs203.sp.cs.cmu.edu", i + 1);
	SimMercuryNode *mn = new SimMercuryNode (g_Simulator, g_Simulator, ip);
	g_Simulator->AddNode (*mn);
	p_nlist->push_back (mn);

	g_Simulator->RaiseEvent (new refcounted<CreateNodeEvent> (mn), SID_NONE, 100 + i * g_DriverPrefs.inter_arrival_time);
    }
}

int rand_between (int i, int j)
{
    return i + (int) (drand48 () * (j - i));
}

int rand_between_incl (int i, int j)
{
    return i + (int) (drand48 () * (j - i + 1));
}

void get_rand_permutation (vector<int>& array, int n)
{
    for (int i = 0; i < n; i++) 
	array.push_back (i);

    int tmp, k;
    for (int i = 0; i < n - 1; i++) {
	k = rand_between (i + 1, n);

	tmp = array[k];
	array[k] = array[i];
	array[i] = tmp;
    }
}


class LoadSetupEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    LoadSetupEvent (MNVec *nodes) : m_Nodes (nodes) {}

    void Execute (Node& node, TimeVal& timenow) {

#if 0
	boost::mt19937 rng;
	boost::normal_distribution<> dist (g_Mean, g_Std);
	boost::variate_generator<boost::mt19937, boost::normal_distribution<> > generator (rng, dist);

	float val = 40000, drop = 20000, dec;
	dec = drop / m_Nodes->size ();
#endif

	vector<int> choices;
	get_rand_permutation (choices, m_Nodes->size ());

	int i = 0;
	float d;

	avg_load = 0.0;

	for (MNVecIter it = m_Nodes->begin (); it != m_Nodes->end (); it++, i++ ) {	    
	    SimMercuryNode *self = *it;

	    bool chosen;

	    chosen = false;	    
	    for (int j = 0; j < g_DriverPrefs.spikes; j++) {
		if (choices[j] == i) {
		    chosen = true;
		    break;
		}
	    }

	    if (chosen) 
		d = (float) g_DriverPrefs.spike_height;
	    else
		d = 100;

	    avg_load += d;

	    cout << self->GetAddress () << " range=" << GetHub (self)->GetRange () << " load=" << d << endl;

	    TestSampler *s = new TestSampler (GetHub (self), d);
	    g_Samplers.push_back (s);
	    self->RegisterLoadSampler (0, s); 
	}
	avg_load /= m_Nodes->size ();
	cout << "=============================================" << endl;
    }
};

float GetOverallMean ()
{
    float m = 0;
    for (int i = 0; i < g_DriverPrefs.nodes; i++) {
	m += g_Samplers[i]->GetValue ();
    }
    m /= g_DriverPrefs.nodes;
    return m;
}

float GetMeanSampleLoad (SimMercuryNode *self, Sampler *sampler)
{
    vector<Sample *> vec;
    if (self->GetSamples (0, sampler, &vec) < 0)
	Debug::die ("error in getting samples; should never happen\n");

    ASSERT (vec.size () > 0);
    float ret = 0;
    for (vector<Sample *>::iterator it = vec.begin (); it != vec.end (); ++it)
	{
	    const LoadMetric *lm = dynamic_cast<const LoadMetric *> ((*it)->GetMetric ());
	    ret += lm->GetLoad ();
	}    

    cerr << " ------ number of samples " << vec.size () << endl;
    return ret / vec.size ();
}

void run_script () 
{
    REGISTER_TYPE (Event, TPEvent);
    REGISTER_TYPE (Event, TMEvent);

    // g_Simulator->RaiseEvent (new refcounted<PrintEvent> (&nlist), SID_NONE, PRINT_DELAY);
    // g_Simulator->RaiseEvent (new refcounted<PrintGraphEvent> (&nlist), SID_NONE, 5000);
    //
    int tjoin = g_DriverPrefs.nodes * g_DriverPrefs.inter_arrival_time + 1000;
    // g_Simulator->RaiseEvent (new refcounted<SendPubsEvent> (&nlist), SID_NONE, tjoin);
    g_Simulator->RaiseEvent (new refcounted<LoadSetupEvent> (&nlist), SID_NONE, tjoin);
    g_Simulator->RaiseEvent (new refcounted<PrintLoadStats> (&nlist), SID_NONE, tjoin + 10);
    g_Simulator->RaiseEvent (new refcounted<PrintStuffEvent> (&nlist), SID_NONE, tjoin + 10);
    create_nodes (&nlist);
}

void finish_script () 
{
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
