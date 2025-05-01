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

// $Id: MetricInfo.cpp 2473 2005-11-13 01:55:30Z ashu $

#include <mercury/Hub.h>
#include <mercury/MercuryNode.h>
#include <mercury/Message.h>
#include <mercury/Sampling.h>
#include <mercury/MetricInfo.h>

void LocalSamplingTimer::OnTimeout () 
{
    m_Hub->DoLocalSampling (m_Handle);
    _RescheduleTimer (m_Timeout);
}

void RandomWalkTimer::OnTimeout ()
{
    m_Hub->StartRandomWalk (m_Handle);
    _RescheduleTimer (m_Timeout);
}

MetricInfo::MetricInfo (MemberHub *hub, uint32 handle, Sampler *s, ref<RandomWalkTimer> rwt, ref<LocalSamplingTimer> lst)
    : m_Hub (hub), m_Handle (handle), m_Sampler (s), m_RandomWalkTimer (rwt), m_LocalSamplingTimer (lst) 
{
    m_MercuryNode = m_Hub->GetMercuryNode ();
}

MetricInfo::~MetricInfo ()
{
    m_RandomWalkTimer->Cancel ();
    m_LocalSamplingTimer->Cancel ();

    for (TimedSampleMapIter it = m_ReceivedSamples.begin (); it != m_ReceivedSamples.end (); ++it) 
	delete it->second;
    for (EstimateMapIter it = m_Estimates.begin (); it != m_Estimates.end (); ++it) 
	delete it->second;
}

MsgSampleResponse* MetricInfo::MakeSampleResponse () 
{
    vector<Metric *> ps;

    for (EstimateMapIter it = m_Estimates.begin (); it != m_Estimates.end (); ++it) 
	ps.push_back ((Metric *) it->second->GetMetric ());

    Metric *local_estimate = m_Sampler->MakeLocalEstimate (ps);
    if (local_estimate == NULL)
	return NULL;

    Sample *local_sample = new Sample (m_Hub->GetAddress (), *m_Hub->GetRange (), local_estimate);
    MsgSampleResponse *resp = new MsgSampleResponse (m_Hub->GetID (), m_Hub->GetAddress (), m_Handle);
    resp->AddSample (local_sample);

    // add the most recent 'max' samples to the response...
    int i = 0, max = m_Sampler->GetNumReportSamples ();
    for (TimedSampleMapIter it = m_ReceivedSamples.begin (); it != m_ReceivedSamples.end (); ++it) {
	if (i++ >= max)
	    break;

	resp->AddSample (it->second);
    }

    delete local_sample;
    delete local_estimate;
    return resp;
}

Sample* MetricInfo::GetPointEstimate () 
{
    Metric *estimate = m_Sampler->GetPointEstimate ();
    if (estimate == NULL)
	return NULL;

    Sample *sample = new Sample (m_Hub->GetAddress (), *m_Hub->GetRange (), estimate);
    delete estimate;
    return sample;
}

void MetricInfo::ExpireSamples (TimeVal& timenow) 
{
    for (TimedSampleMapIter it = m_ReceivedSamples.begin (); it != m_ReceivedSamples.end (); /* it++ */) {
	if (it->first <= timenow) {
	    TimedSampleMapIter oit (it);
	    oit++;

	    SIDSampleMapIter ssmit = m_SIDMap.find (it->second->GetSender ());
	    ASSERT (ssmit != m_SIDMap.end ());
	    m_SIDMap.erase (ssmit);

	    delete it->second;
	    m_ReceivedSamples.erase (it);

	    it = oit;
	}
	else {
	    it++;
	}
    }
}

void MetricInfo::AddPointEstimate (int distance, Sample *est) 
{
    EstimateMapIter it = m_Estimates.find (distance);
    if (it != m_Estimates.end ()) {
	delete it->second;
	m_Estimates.erase (it);
    }

    m_Estimates.insert (EstimateMap::value_type (distance, est->Clone ()));
}

void MetricInfo::FillSamples (vector<Sample *> *ret) 
{
    for (TimedSampleMapIter it = m_ReceivedSamples.begin (); it != m_ReceivedSamples.end (); ++it) {
	ret->push_back (it->second);
    }
}

void MetricInfo::RemoveSample (const IPEndPoint& sender)
{
    // check if we have a sample from this node
    SIDSampleMapIter ssmit = m_SIDMap.find (sender);
    if (ssmit != m_SIDMap.end ()) {  
	TimedSampleMapIter tsmit = ssmit->second;
	delete tsmit->second;
	m_ReceivedSamples.erase (tsmit);
	m_SIDMap.erase (ssmit);
    }
}

void MetricInfo::AddSample (Sample *s, TimeVal& timenow) 
{
    MDB (10) << "adding sample " << s << " from " << s->GetSender () << " at " << timenow << endl;

    // reject our own sample! :)
    if (s->GetSender () == m_Hub->GetAddress ()) 
	return;

    // check if we have a sample from this node
    SIDSampleMapIter ssmit = m_SIDMap.find (s->GetSender ());
    if (ssmit != m_SIDMap.end ()) {  
	// XXX: we dont have time-stamped samples! so it 
	// could be that we overwrite a new sample with a old one!

	TimedSampleMapIter tsmit = ssmit->second;
	delete tsmit->second;
	m_ReceivedSamples.erase (tsmit);
	m_SIDMap.erase (ssmit);
    }

    u_long ttl = m_Sampler->GetSampleLifeTime ();
    TimedSampleMapIter it = m_ReceivedSamples.insert (TimedSampleMap::value_type (timenow + ttl, s->Clone ()));		
    m_SIDMap.insert (SIDSampleMap::value_type (s->GetSender (), it));
}


Sample *MetricInfo::GetNeighborHoodEstimate (int distance)
{
    EstimateMapIter it = m_Estimates.find (distance);
    if (it == m_Estimates.end ())
	return NULL;
    else 
	return it->second;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
