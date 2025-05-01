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
#ifndef SAMPLING__H
#define SAMPLING__H

#include <mercury/MercuryID.h>
#include <mercury/Timer.h>
#include <mercury/IPEndPoint.h>
#include <mercury/AutoSerializer.h>
#include <mercury/Constraint.h>

//////////////////////////////////////////////////////////////////////////
// Forward declarations
class MemberHub;
class Histogram;

struct MsgCB_EstimateResp;
class HistogramMaintainer;
class Scheduler;
class NetworkLayer;

// Nothing in here. The app can put in what it wants!
class Metric : public virtual Serializable {
 protected:
    DECLARE_BASE_TYPE (Metric);
 public:
    Metric () {}
    Metric (Packet *pkt) {
	(void) pkt->ReadByte ();
    }
    virtual ~Metric () {}

    void Serialize (Packet *pkt) {
	pkt->WriteByte (GetType ());
    }
    uint32 GetLength () { 
	return 1;
    }
    void Print (FILE *stream);

    virtual const char *TypeString () const = 0;
    virtual void Print (ostream& os) const;
};

ostream& operator<< (ostream& out, const Metric *m);
ostream& operator<< (ostream& out, const Metric& m);

class Sample : public virtual Serializable {
    friend class MetricInfo;

    IPEndPoint m_SenderAddr;
    NodeRange  m_Range;
    Metric    *m_Metric;

 public:
    Sample (IPEndPoint& addr, NodeRange& range, Metric *metric) : 
	m_SenderAddr (addr), m_Range (range), m_Metric (metric->Clone ()) {}

    virtual ~Sample () {
	delete m_Metric;
    }
    Sample (const Sample& other) : 
	m_SenderAddr (other.GetSender ()), m_Range (other.GetRange ()), m_Metric (other.GetMetric ()->Clone ()) {}

    Sample& operator=(const Sample& other) {
	if (this == &other) return *this;

	m_SenderAddr = other.GetSender ();
	m_Range = other.GetRange ();
	delete m_Metric;
	m_Metric = other.GetMetric ()->Clone ();
    }

    Sample* Clone () const { return new Sample (*this); }

    const IPEndPoint& GetSender () const { return m_SenderAddr; }
    const NodeRange& GetRange () const { return m_Range; }
    const Metric* GetMetric () const { return m_Metric; }

    void SetRange (const NodeRange& r) { m_Range = r; }

    Sample (Packet *pkt) : m_SenderAddr (pkt), m_Range (pkt) {
	m_Metric = CreateObject<Metric> (pkt);
    }

    virtual void Serialize (Packet *pkt) {
	m_SenderAddr.Serialize (pkt);
	m_Range.Serialize (pkt);
	m_Metric->Serialize (pkt);
    }

    virtual uint32 GetLength () {
	return m_SenderAddr.GetLength () + m_Range.GetLength () + m_Metric->GetLength (); 
    }

    virtual void Print (FILE *stream) {
	fprintf(stream, "sender=");
	m_SenderAddr.Print (stream);
	fprintf (stream, " range=");
	m_Range.Print (stream);
	fprintf (stream, " metric=");
	m_Metric->Print (stream);
    }

};

ostream& operator<< (ostream& out, const Sample *s);
ostream& operator<< (ostream& out, const Sample& s);

class Sampler {
 public:
    virtual ~Sampler () {}

    /**
     * Return a string identifying this sampling function. 
     * This is used by Mercury to generate a handle internally
     * for identifying across nodes.
     **/ 
    virtual const char *GetName () const = 0;

    /**
     * Radius of the local neighborhood for performing point
     * sampling. e.g., specifying a radius = 1 implies mercury
     * point samples (pred, me, and succ) to generate a 
     * local sample at my node.
     **/
    virtual int GetLocalRadius () const = 0;

    /**
     * Returns the lifetime of a sample in milliseconds. i.e., 
     * the time for which a sample does not become stale.
     **/
    virtual u_long GetSampleLifeTime () const = 0;

    /**
     * Number of samples to report from our cache of obtained
     * samples when a random-walk-based sample request arrives.
     **/
    virtual int GetNumReportSamples () const = 0;

    /**
     * The period at which a new random walk will be started 
     * for collecting samples. Together with GetNumReportSamples()
     * and GetSampleLifeTime() it will roughly decide the number
     * of samples available at any given time.
     **/
    virtual int GetRandomWalkInterval () const = 0;

    /**
     * Perform a measurement of the metric at THIS node only. 
     * Return NULL if measurement cannot be performed. Mercury
     * takes ownership of the returned sample and 'delete's it.
     **/
    virtual Metric* GetPointEstimate () = 0;

    /** 
     * Combine the point estimates to produce a sample to be
     * sent to requesting nodes. Mercury takes ownership of 
     * the returned sample and 'delete's it.
     **/
    virtual Metric* MakeLocalEstimate (vector<Metric* >& samples) = 0;
};

class NodeCountSampler : public Sampler {
    MemberHub *m_Hub;
 public:
    NodeCountSampler (MemberHub *hub) : m_Hub (hub) {}

    virtual const char *GetName () const { return "NodeCountSampler"; }
    virtual int GetLocalRadius () const;

    virtual u_long GetSampleLifeTime () const;
    virtual int GetNumReportSamples () const;
    virtual int GetRandomWalkInterval () const;

    virtual Metric* GetPointEstimate ();
    virtual Metric* MakeLocalEstimate (vector<Metric* >& samples);
};

class LoadSampler : public Sampler {
 protected:
    MemberHub *m_Hub;
 public:
    LoadSampler (MemberHub *hub) : m_Hub (hub) {}

    virtual const char *GetName () const { return "LoadSampler"; }
    virtual int GetLocalRadius () const;

    virtual u_long GetSampleLifeTime () const;
    virtual int GetNumReportSamples () const;
    virtual int GetRandomWalkInterval () const;

    virtual Metric* GetPointEstimate ();
    virtual Metric* MakeLocalEstimate (vector<Metric* >& samples);

    // ONLY FOR DEBUG and TESTING
    virtual void SetLoad (Metric *currentload);
};

class NCMetric : public Metric {
    Value m_RangeSpan;
 public:    
    DECLARE_TYPE (Metric, NCMetric);

    NCMetric (Value& span) : m_RangeSpan (span) {}
    NCMetric (Packet *pkt) : Metric (pkt), m_RangeSpan (pkt) {}
    virtual ~NCMetric () {}

    const Value& GetRangeSpan () const { return m_RangeSpan; }
    virtual void Print (ostream& out) const { 
	Metric::Print (out);
	out << "(span=" << m_RangeSpan << ")";
    }

    const char *TypeString () const { return "NODE_COUNT_METRIC"; }

    virtual void Serialize (Packet *pkt) { 
	Metric::Serialize (pkt);
	m_RangeSpan.Serialize (pkt); 
    }
    virtual uint32 GetLength () { 
	return Metric::GetLength () + m_RangeSpan.GetLength (); 
    }
    virtual void Print (FILE *stream) { 
	Metric::Print (stream);
	fprintf (stream, "(span="); 
	m_RangeSpan.Print (stream); 
	fprintf (stream, ")");
    }
};

class LoadMetric : public Metric {
    float m_Load;
 public:
    DECLARE_TYPE (Metric, LoadMetric);

    LoadMetric (float load) : m_Load (load) {}
    LoadMetric (Packet *pkt) : Metric (pkt), m_Load (pkt->ReadFloat ()) {}
    virtual ~LoadMetric () {}

    float GetLoad () const { return m_Load; }

    virtual void Print (ostream& out) const {
	Metric::Print (out);
	out << "(load=" << merc_va ("%.3f", m_Load) << ")";
    }

    const char *TypeString () const { return "LOAD_METRIC"; }
    virtual void Serialize (Packet *pkt) { 
	Metric::Serialize (pkt);
	pkt->WriteFloat (m_Load); 
    }
    virtual uint32 GetLength () { 
	return Metric::GetLength () + 4; 
    }
    virtual void Print (FILE *stream) { 
	Metric::Print (stream);
	fprintf (stream, "(load=%.3f)", m_Load); 
    }
};

class DummyLoadSampler : public LoadSampler {
 public:
    DummyLoadSampler (MemberHub *hub) : LoadSampler (hub) {}
    virtual ~DummyLoadSampler () {}

    virtual Metric *GetPointEstimate () { 
	return new LoadMetric (0.0);
    }

    void SetLoad (Metric *l) {}
};

class MercuryNode;
class BootstrapSamplingTimer;

class HistogramMaintainer : public MessageHandler
{
    MemberHub      *m_Hub;
    MercuryNode    *m_MercuryNode;
    int             m_NodeCount;
    int             m_NodeCountWhenLastRepaired;
    Histogram      *m_NodeCountHistogram;
    ref<BootstrapSamplingTimer>  m_SamplingTimer;

    IPEndPoint      m_Address;
    Scheduler      *m_Scheduler;
    NetworkLayer   *m_Network;	
 public:
    HistogramMaintainer(MemberHub *hub);
    virtual ~HistogramMaintainer() {}

    void ProcessMessage(IPEndPoint *from, Message *msg);
    void Start();
    void Pause ();
    void DoPeriodic();

    bool GetValueAtDistance(int distance, Value &val);
    int  EstimateNodeCount();
    int  GetNodeCountWhenLastRepaired () { return m_NodeCountWhenLastRepaired; }
    void SetHistogram (Histogram *h) { m_NodeCountHistogram = h; }

 private:
    void HandleEstimateResponse(IPEndPoint *from, MsgCB_EstimateResp *msg);
};

void RegisterMetricTypes ();
#endif // SAMPLING__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
