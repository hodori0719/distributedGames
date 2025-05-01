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
#ifndef __METRICINFO__H
#define __METRICINFO__H   

// $Id: MetricInfo.h 2382 2005-11-03 22:54:59Z ashu $

#include <map>
#include <mercury/IPEndPoint.h>
#include <mercury/ID.h>
#include <mercury/Timer.h>
#include <util/TimeVal.h>

struct less_timeval {
    // keep the most recent version first.
    bool operator () (const TimeVal& a, const TimeVal& b) const {
	return a > b;
    }
};

class Sample;

typedef multimap<TimeVal, Sample *, less_timeval> TimedSampleMap;
typedef TimedSampleMap::iterator TimedSampleMapIter;

typedef map<SID, TimedSampleMapIter, less_SID> SIDSampleMap;
typedef SIDSampleMap::iterator SIDSampleMapIter;

typedef map<int, Sample *, less<int> > EstimateMap;
typedef EstimateMap::iterator EstimateMapIter;

class MemberHub;
class MercuryNode;
class Sampler;
struct MsgSampleResponse;

/////////////////////////////////////////////////////////////////////
/// Sampling related functionality. 

class LocalSamplingTimer : public Timer {
    MemberHub *m_Hub;
    uint32 m_Handle;
    u_long m_Timeout;

 public:
    LocalSamplingTimer (MemberHub *hub, uint32 handle, u_long timeout) : 
	Timer (timeout), m_Hub (hub), m_Handle (handle), m_Timeout (timeout) {}

    void OnTimeout ();
};

class RandomWalkTimer : public Timer {
    MemberHub *m_Hub;
    uint32 m_Handle;
    u_long m_Timeout;

 public:
    RandomWalkTimer (MemberHub *hub, uint32 handle, u_long timeout) : 
	Timer (timeout), m_Hub (hub), m_Handle (handle), m_Timeout (timeout) {}

    void OnTimeout ();
};

class MetricInfo {
    MemberHub              *m_Hub;
    uint32                  m_Handle;
    Sampler                *m_Sampler;
    ref<RandomWalkTimer>    m_RandomWalkTimer;
    ref<LocalSamplingTimer> m_LocalSamplingTimer;

    TimedSampleMap          m_ReceivedSamples;      // index by expiry time so we give most recent ones
    SIDSampleMap            m_SIDMap;               // index by SID so we dont keep multiple samples from same guy
    EstimateMap             m_Estimates;            // estimates from our local neighborhood

    MercuryNode            *m_MercuryNode;

 public:
    MetricInfo (MemberHub *hub, uint32 handle, Sampler *s, ref<RandomWalkTimer> rwt, ref<LocalSamplingTimer> lst);
    ~MetricInfo ();

    // add to the sample collection available to the application
    void AddSample (Sample *s, TimeVal& timenow);

    // remove sample from this sender
    void RemoveSample (const IPEndPoint& sender);	

    // return received samples which haven't expired
    void FillSamples (vector<Sample *> *ret);

    // return a pointer to the application Sampler object
    Sampler *GetSampler () const { return m_Sampler; }

    // register a point estimate from my neighbors (used to make a local estimate)
    void AddPointEstimate (int distance, Sample *est);

    // expire old samples
    void ExpireSamples (TimeVal& timenow);

    MsgSampleResponse *MakeSampleResponse ();

    // utility: convert a pointestimate (metric) to a sample
    Sample *GetPointEstimate ();

    // indexes into m_Estimates. the name of the method may not be optimal!
    Sample *GetNeighborHoodEstimate (int distance);
};

#endif /* __METRICINFO__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
