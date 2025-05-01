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
#include <mercury/Message.h>
#include <mercury/Sampling.h>
#include <mercury/Parameters.h>
#include <mercury/NetworkLayer.h>
#include <mercury/MercuryNode.h>
#include <mercury/PubsubRouter.h>
#include <mercury/Histogram.h>
#include <mercury/LinkMaintainer.h>
#include <mercury/Scheduler.h>
#include <mercury/Peer.h>
#include <mercury/Hub.h>
#include <mercury/Utils.h>

byte METRIC_NC, METRIC_LOAD;

ostream& operator<< (ostream& out, const Metric *m)
{
    m->Print (out);
    return out;
}

ostream& operator<< (ostream& out, const Metric& m) 
{
    return out << &m;
}

void Metric::Print (ostream& os) const  {
    // os << "type=" << TypeString ();
}

void Metric::Print (FILE *stream) {
    fprintf (stream, "type=%s", TypeString ());
}

void RegisterMetricTypes ()
{
    METRIC_NC = REGISTER_TYPE (Metric, NCMetric);
    METRIC_LOAD = REGISTER_TYPE (Metric, LoadMetric);
}

ostream& operator<< (ostream& out, const Sample *s)
{
    if (s == NULL)
	return out << "(null sample)";

    out << "sender=" << s->GetSender () << " range=" << s->GetRange () 
	<< " metric=" << s->GetMetric ();
    return out;
}

ostream& operator<< (ostream& out, const Sample& s) 
{
    return out << &s;
}

int NodeCountSampler::GetLocalRadius () const {
    return 2;
}

u_long NodeCountSampler::GetSampleLifeTime () const {
    return 3 * 60 * 1000;
}

int NodeCountSampler::GetNumReportSamples () const {
    return 10;
}

int NodeCountSampler::GetRandomWalkInterval () const {
    return 5000;
}

Metric *NodeCountSampler::GetPointEstimate ()
{
    if (m_Hub->GetStatus () != ST_JOINED)
	return NULL;
    if (m_Hub->GetRange () == NULL)
	return NULL;

    Value span = m_Hub->GetRangeSpan ();
    {
	MercuryNode *m_MercuryNode = m_Hub->GetMercuryNode ();
	MDB (10) << " point-estimate for node-count-span=" << span << endl;
    }
    return new NCMetric (span);
}

Metric *NodeCountSampler::MakeLocalEstimate (vector<Metric *>& samples) 
{
    if (m_Hub->GetStatus () != ST_JOINED)
	return NULL;
    if (m_Hub->GetRange () == NULL)
	return NULL;
    if (samples.size () == 0)
	return NULL;

    Value avg = 0U;
    for (vector<Metric *>::iterator it = samples.begin (); it != samples.end (); ++it) {
	NCMetric *ncs = dynamic_cast<NCMetric *> (*it);
	avg += ncs->GetRangeSpan ();
    }

    avg /= samples.size ();

    {
	MercuryNode *m_MercuryNode = m_Hub->GetMercuryNode ();
	MDB (10) << " making node count sample with average span=" << avg << endl;
    }
    return new NCMetric (avg);
}

int LoadSampler::GetLocalRadius () const {
    return 2;
}

u_long LoadSampler::GetSampleLifeTime () const {
    return 5000;
}

int LoadSampler::GetNumReportSamples () const {
    return 10;
}

int LoadSampler::GetRandomWalkInterval () const {
    return 5000;
}

Metric *LoadSampler::GetPointEstimate () 
{
    if (m_Hub->GetStatus () != ST_JOINED)
	return NULL;
    if (m_Hub->GetRange () == NULL)
	return NULL;

    {
	MercuryNode *m_MercuryNode = m_Hub->GetMercuryNode ();
	MDB (10) << " point-estimate for load=" << m_Hub->GetPubsubRouter ()->GetRoutingLoad () << endl;
    }
    return new LoadMetric (m_Hub->GetPubsubRouter ()->GetRoutingLoad ());
}

Metric *LoadSampler::MakeLocalEstimate (vector<Metric *>& samples) 
{
    if (m_Hub->GetStatus () != ST_JOINED)
	return NULL;
    if (m_Hub->GetRange () == NULL)
	return NULL;
    if (samples.size () == 0)
	return NULL;

    float avg = 0.0;
    for (vector<Metric *>::iterator it = samples.begin (); it != samples.end (); ++it) {
	LoadMetric *ls = dynamic_cast<LoadMetric *> (*it);
	avg += ls->GetLoad (); 
    }

    avg /= samples.size ();
    {
	MercuryNode *m_MercuryNode = m_Hub->GetMercuryNode ();
	MDB (0) << " making load sample with average load=" << avg << endl;
    }
    return new LoadMetric (avg);
}

void LoadSampler::SetLoad (Metric *nl)
{
}

/////////////////////////////   BootstrapSamplingTimer  ////////////////////////
//
class BootstrapSamplingTimer : public Timer {
    HistogramMaintainer *m_HistogramMaintainer;
    MemberHub *m_Hub;
public:
    BootstrapSamplingTimer (HistogramMaintainer *hm, MemberHub *hub, int timeout) : Timer(timeout)
	{
	    m_Hub = hub;
	    m_HistogramMaintainer = hm;
	}

    // version 1.0 of the sampling algorithm, instead of querying samples from the
    // hub, it just asks the bootstrap server about it. the bootstrap server also periodically
    // queries these nodes for getting samples.

    void OnTimeout()
	{
	    DB(10) << "sending an estimate request..." << endl;

	    if (!g_Preferences.self_histos) {
		// send a message to the bootstrap node asking for a histogram estimate
		MsgCB_EstimateReq *req = new MsgCB_EstimateReq(m_Hub->GetID(), m_Hub->GetAddress ());
		m_Hub->GetNetwork()->SendMessage(req, &m_Hub->GetBootstrapIP (), Parameters::TransportProto);
		delete req;
	    }

	    m_HistogramMaintainer->DoPeriodic();
	    DB(20) << "rescheduling timer after " << Parameters::BootstrapSamplingInterval << " milliseconds " << endl;
	    _RescheduleTimer(Parameters::BootstrapSamplingInterval);
	}
};

//////////////////////////////////////////////////////////////////////////
// HistogramMaintainer
HistogramMaintainer::HistogramMaintainer(MemberHub *hub)
    : m_SamplingTimer (new refcounted<BootstrapSamplingTimer>(this, hub, 0))
{
    m_Hub = hub;
    m_Network = m_Hub->GetNetwork ();
    m_Address = m_Hub->GetAddress ();
    m_Scheduler = m_Hub->GetScheduler ();
    m_MercuryNode = m_Hub->GetMercuryNode ();

    m_NodeCountHistogram = NULL;
    m_NodeCount = -1;
    m_NodeCountWhenLastRepaired = -1;

    m_MercuryNode->RegisterMessageHandler (MSG_CB_ESTIMATE_RESP, this);
}

void HistogramMaintainer::Start()
{
    m_NodeCountHistogram = NULL;
    m_NodeCount = -1;
    m_NodeCountWhenLastRepaired = -1;

    m_SamplingTimer = new refcounted<BootstrapSamplingTimer> (this, m_Hub, 0);
    m_Scheduler->RaiseEvent (m_SamplingTimer, m_Address, 0);
}

void HistogramMaintainer::Pause ()
{
    m_SamplingTimer->Cancel ();
}

void HistogramMaintainer::ProcessMessage(IPEndPoint *from, Message *msg)
{
    if (msg->hubID != m_Hub->GetID())
	return;

    if (msg->GetType () == MSG_CB_ESTIMATE_RESP)
	HandleEstimateResponse(from, (MsgCB_EstimateResp *) msg);
    else 
	WARN << merc_va("HistogramMaintainer:: received some idiotic message [%s]", msg->TypeString()) << endl;
}

// FIXME: These histograms will actually be computed by a
// random-walk based sampling procedure. However, in the
// current implementation, the centralized bootstrap server
// contains statistics and sends us a full histogram
// instead.

void HistogramMaintainer::HandleEstimateResponse(IPEndPoint *from, MsgCB_EstimateResp *msg)
{
    if (g_Preferences.self_histos) {
	MWARN << " received histogram from bootstrap but self_histos is enabled!" << endl;
	delete msg->hist;
	return;
    }

    if (m_NodeCountHistogram) {
	delete m_NodeCountHistogram;
    }
    m_NodeCountHistogram = msg->hist;
    MDB (20) << " received estimate response ... " << m_NodeCountHistogram << endl;
}

int HistogramMaintainer::EstimateNodeCount() {
    if (!m_NodeCountHistogram)
	return -1;

    // Given that the server sends us a histogram, we can
    // just sum up the bucket counts and we'll be done!

    float count = 0;
    for (int i = 0, len = m_NodeCountHistogram->GetNumBuckets(); i < len; i++) {
	count += m_NodeCountHistogram->GetBucket(i)->GetValue ();
    }

    return (int) (count + 0.5);
}

#define EPSILON 0.0000001

//
// Estimate a value ...
//
bool HistogramMaintainer::GetValueAtDistance(int distance, Value &val)
{
    if (!m_NodeCountHistogram)
	return false;

    NodeRange *range = m_Hub->GetRange();
    if (!range)
	return false;

    float dist = (float) distance;
    int   nbkts = m_NodeCountHistogram->GetNumBuckets();

    MDB(10) << merc_va("we asked dist: %d", distance) << endl;

    // first; locate ourselves in the histogram;
    int start_bkt = m_NodeCountHistogram->GetBucketForValue (range->GetMax());
    if (start_bkt < 0) { 
	DB(-5) << "error looking up " << range->GetMax() << endl;
    }
    ASSERTDO(start_bkt >= 0, INFO << "** my_range=" << range 
	    << " start_bkt=" << start_bkt << endl 
	    << " histogram=" << m_NodeCountHistogram << endl);

    int bkt_index = -1;

    // second; loop starting from the right in the histogram and keep decrementing dist;
    int i;
    for (int j = 0; j < nbkts; j++) 
    {
	i = (start_bkt + j + 1) % nbkts;
	dist -= m_NodeCountHistogram->GetBucket (i)->GetValue ();

	MDB(10) << merc_va ("dist...%f", dist) << endl;
	if (dist <= 0) {
	    bkt_index = i;
	    break;
	}
    }

    ASSERTDO(bkt_index != -1, INFO << " distance=" << distance << " bkt_index=" << bkt_index << endl);

    dist += m_NodeCountHistogram->GetBucket (bkt_index)->GetValue ();
    MDB(10) << merc_va("finally. dist=%f bkt_index=%d", dist, bkt_index) << endl;

    // third; get a value and store it in val; 
    Value to_ret;
    const HistElem *he = m_NodeCountHistogram->GetBucket (bkt_index);
    const NodeRange &r = he->GetRange ();

    /* account for the fact that we are in between a bucket */
    if (dist > EPSILON && he->GetValue () > EPSILON) {
	int ratio = (int)  (he->GetValue() / dist);
	MDB(10) << " hev=" << he->GetValue() << " bkt=" << bkt_index << " dist=" << dist << " RATIO=" << ratio << endl;
	to_ret = r.GetSpan (m_Hub->GetAbsMin(), m_Hub->GetAbsMax());
	to_ret /= ratio;
    }
    else 
	to_ret = 0;

    to_ret += r.GetMin ();

    if (to_ret > m_Hub->GetAbsMax ())
	to_ret -= m_Hub->GetAbsMax ();

    MDB(10) << "val to ret: " << to_ret << endl;
    val = to_ret;

    ASSERTDO (to_ret <= m_Hub->GetAbsMax (), cerr << "to_ret=" << to_ret << " max=" << m_Hub->GetAbsMax() 
	      << " dist=" << distance << " nodecount=" << EstimateNodeCount() << "r=" << r << endl << "histo=" << *m_NodeCountHistogram << endl);
    ASSERTDO (to_ret >= m_Hub->GetAbsMin (), cerr << "to_ret=" << to_ret << " max=" << m_Hub->GetAbsMax() 
	      << " dist=" << distance << " nodecount=" << EstimateNodeCount());
    return true;
}

void HistogramMaintainer::DoPeriodic()
{
    // if we have a histogram, compute the number of nodes;
    int m_NodeCount = EstimateNodeCount();
    MTDB(20) << "now:prev = " << m_NodeCount << ":" << m_NodeCountWhenLastRepaired << endl;

    // do some stuff if we had a histogram
    if (m_NodeCount > 0) {
	int prevNumNodes = m_NodeCountWhenLastRepaired;

	// xxx: Jeff: don't we also have to repair long pointers due to
	// changes in the existing ptrs' locations in the distribution?
	// in addition to changes in the distribution itself?
	if (prevNumNodes < 8                // first time
	    || m_NodeCount > 2 * prevNumNodes   // increase by a factor of 2
	    || 2 * m_NodeCount < prevNumNodes   // decrease by a factor of 2
	    )
	{
	    MTDB(20) << "repairing long pointers, yo!" << endl;
	    m_Hub->m_LinkMaintainer->RepairLongPointers();
	    m_NodeCountWhenLastRepaired = m_NodeCount;
	}
    } else {
	DB(1) << "not repairing: prev=" << m_NodeCountWhenLastRepaired
	      << " now=" << m_NodeCount << endl;
    }
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
