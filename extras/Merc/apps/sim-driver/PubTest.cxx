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
typedef vector<SimMercuryNode *> MNVec;
typedef MNVec::iterator MNVecIter;

ofstream fout;
MNVec nlist;

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

class PrintRangesEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    PrintRangesEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow) {
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

	    /*
	      cerr << " succs={";
	      vector<Neighbor> succs = self->GetSuccessors (0);
	      for (vector<Neighbor>::iterator ooit = succs.begin(); ooit != succs.end (); ooit++) {
	      Neighbor n = *ooit;
	      cerr << n.addr << " ";
	      }
	      cerr << "}" << endl;
	    */
	    cerr << endl;

#if 0
	    vector<Neighbor> lnbrs = self->GetLongNeighbors (0);
	    fout << "node " << self->GetAddress () << " range " << vc[0] << " nlinks " << (int) lnbrs.size ()
		 << " repaircount=" << HM(self)->GetNodeCountWhenLastRepaired () << endl;
	    for (vector<Neighbor>::iterator ooit = lnbrs.begin(); ooit != lnbrs.end (); ooit++) {
		Neighbor n = *ooit;
		fout << "\t" << n.addr << " " << n.range << endl;
	    }
#endif
	    // PrintSamples (self);
	}

    }
};

class PrintTimeEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    PrintTimeEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow) {
	cout << " =====>>> Current Time=" << timenow << " <<<=====" << endl;
	g_Simulator->RaiseEvent (mkref (this), SID_NONE, 1000);
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

#if 0
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
#endif
	for (MNVecIter it = m_Nodes->begin (); it != m_Nodes->end (); it++) {
	    SimMercuryNode *self = *it;

	    vector<Constraint> vc = self->GetHubRanges ();
	    if (vc.size () == 0) 
		continue;

	    vector<Neighbor> lnbrs = self->GetLongNeighbors (0);

	    Value v = vc[0].GetMin ();
	    for (vector<Neighbor>::iterator oit = lnbrs.begin (); oit != lnbrs.end (); oit++) {
		Neighbor n = *oit;
		os << v << " " << n.range.GetMin () << " " << n.range.GetMax () << endl;
	    }
	}

	os.flush ();
	os.close ();

	// g_Simulator->RaiseEvent (mkref (this), SID_NONE, PRINT_GRAPH_DELAY);
    }
};

void print_range (SID addr, ostream& os)
{
    int id = addr.GetPort ();
    SimMercuryNode *n = nlist[(id - 1)];
    os << "(" << id << ") -- " << GetHub (n)->GetRange () << endl;
}

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
	if (pmsg->hopCount <= 50) {
	    cout << "hops " << (int) pmsg->hopCount << endl;
	    return false;
	}

#ifdef DEBUG
	fout << "event val=" << m_Constraints[0].GetMin () << " route=" << endl;
	print_range (pmsg->GetCreator (), fout);

	for (list<Neighbor>::iterator it = pmsg->routeTaken.begin (); it != pmsg->routeTaken.end (); it++)
	    {
		print_range (it->addr, fout);
	    }
	fout << endl;
#endif
	cout << "hops " << (int) pmsg->hopCount << endl;
	return false;
    }
};

class EApp : public DummyApp {
public:
    int EventRoute (Event *ev, const IPEndPoint& lastHop) {
	// cout << "ev=" << hex << ev->GetNonce () << " -> routing" << endl;
	return 1;
    }
    int EventLinear (Event *ev, const IPEndPoint& lastHop) {
	// cout << "ev=" << hex << ev->GetNonce () << " -> linear" << endl;
	return 1;
    }
    EventProcessType EventAtRendezvous (Event *ev, const IPEndPoint& lastHop, int nhops) {
	cout << "ev=" << merc_va ("%0x", ev->GetNonce ()) << " -> hops " << nhops << endl;
	return EV_MATCH;
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

    virtual bool OnRendezvousReceive (MemberHub *h, MsgPublication *pmsg) { 
	cout << "hops " << (int) pmsg->hopCount << endl;
	/*
	  SimMercuryNode *m_MercuryNode = h->GetMercuryNode ();
	  MDB (0) << "::: event " << this << " routed in <<<" << pmsg->hopCount  << ">>> hops " << endl;
	  MDB (0) << pmsg << endl;
	*/
	return false;
    }
};

class SendPubsEvent : public SchedulerEvent {
    MNVec *m_Nodes;
public:
    SendPubsEvent (MNVec *n) : m_Nodes (n) {}
    virtual void Execute (Node& node, TimeVal& timenow) {
#if 0
	SimMercuryNode *self = (*m_Nodes)[2];

	TMEvent *ev = new TMEvent ();
	Constraint c (0, 400, 5000);
	ev->AddConstraint (c);
	self->SendEvent (ev);
#endif
	// everybody's joined by now! :)
	Parameters::SuccessorMaintenanceTimeout = 1000;

	int nnodes = m_Nodes->size ();
	vector<Constraint> vc = (*m_Nodes)[0]->GetHubConstraints ();
	unsigned int max = vc[0].GetMax ().getui ();

#if 0
	for (MNVecIter it = m_Nodes->begin (); it != m_Nodes->end (); it++) {
	    SimMercuryNode *self = *it;

	    vector<Constraint> vc = self->GetHubRanges ();
	    if (vc.size () == 0)
		continue;

	    cerr << self->GetAddress () << "[" << vc[0].GetMin () << "]";

	    vector<Neighbor> lnbrs = self->GetLongNeighbors (0);

	    for (vector<Neighbor>::iterator oit = lnbrs.begin (); oit != lnbrs.end (); oit++) {
		Neighbor n = *oit;
		cerr << " " << n.range.GetMin () << " " << n.range.GetMax () << endl;
	    }
	}
#endif

	for (int npubs = 0; npubs < 1000;  npubs++) {
	    SimMercuryNode *self = (*m_Nodes)[(int) (drand48 () * nnodes)];
#ifdef POINT
	    TPEvent *tev = new TPEvent ();
	    Value v = (uint32) ( drand48 () * max );		
	    Tuple t (0, v);
	    tev->AddTuple (t);

	    self->SendEvent (tev);
	    delete tev;
#else
	    TMEvent *tmv = new TMEvent ();

#if 0
	    Value m = (uint32) (drand48 () * 0.2 * max);
	    Value M = (uint32) (0.4 * max + drand48 () * 0.2 * max);
	    M += m;
#else
	    Value m = (uint32) (drand48 () * 0.95 * max);
	    Value M = m;
#endif

	    Constraint c (0, m, M);
	    tmv->AddConstraint (c);

	    self->SendEvent (tmv);
	    delete tmv;
#endif
	}
#if 0	    
	for (MNVecIter it = m_Nodes->begin (); it != m_Nodes->end (); it++) 
	    {	    
		for (int i = 0; i < 4; i++) {
		    TPEvent *tev = new TPEvent ();
		    Value v = (uint32) ( drand48 () * 10000 );		
		    Tuple t (0, v);
		    tev->AddTuple (t);

		    self->SendEvent (tev);
		    delete tev;
		}

		for (int i = 0; i < 4; i++) {
		    Interest *in = new Interest ();

		    Value v = (uint32) ( drand48 () * 10000 );		
		    Value w = (uint32) ( drand48 () * 10000 );		

		    Constraint c = RANGE_NONE;
		    if (v < w) 
			c = Constraint (0, v, w);
		    else 
			c = Constraint (0, w, v);

		    in->AddConstraint (c);
		    self->RegisterInterest (in);
		    delete in;
		}

	    }
	g_Simulator->RaiseEvent (mkref (this), SID_NONE, 50);
#endif
    }
};

void create_nodes (MNVec *p_nlist)
{
    EApp *app = new EApp ();     // dont care about leak!

    for (int i = 0; i < g_DriverPrefs.nodes; i++) {
	IPEndPoint ip ("gs203.sp.cs.cmu.edu", i + 1);
	SimMercuryNode *mn = new SimMercuryNode (g_Simulator, g_Simulator, ip);

	mn->RegisterApplication (app);
	g_Simulator->AddNode (*mn);
	p_nlist->push_back (mn);

	g_Simulator->RaiseEvent (new refcounted<CreateNodeEvent> (mn), SID_NONE, 100 + i * g_DriverPrefs.inter_arrival_time);
    }
}

void run_script () {
    if (g_DriverPrefs.lfile[0]) 
	fout.open (g_DriverPrefs.lfile, ios::out);

    REGISTER_TYPE (Event, TPEvent);
    REGISTER_TYPE (Event, TMEvent);

    int tjoin = g_DriverPrefs.nodes * g_DriverPrefs.inter_arrival_time + 5000;
    g_Simulator->RaiseEvent (new refcounted<PrintTimeEvent> (&nlist), SID_NONE, 100);
    g_Simulator->RaiseEvent (new refcounted<PrintRangesEvent> (&nlist), SID_NONE, tjoin - 1000);
    g_Simulator->RaiseEvent (new refcounted<PrintGraphEvent> (&nlist), SID_NONE, tjoin - 1000);
    // g_Simulator->RaiseEvent (new refcounted<SendPubsEvent> (&nlist), SID_NONE, tjoin);

    create_nodes (&nlist);
}

void finish_script () 
{
    if (g_DriverPrefs.lfile[0])
	fout.close ();
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
