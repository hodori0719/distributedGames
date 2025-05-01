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
#include <Mercury.h>
#include <util/OS.h>
#include <mercury/Message.h>
#include <wan-env/RealNet.h>
#include <wan-env/WANScheduler.h>
#include <util/types.h>
#include <util/TimeVal.h>
#include <util/debug.h>
#include <util/Benchmark.h>

Message *ReadMessage(RealNet *net, int timeout) {
    IPEndPoint from((uint32)0, 0);
    Message *msg = 0;
    ConnStatusType status;

    TimeVal now, endtime;
    gettimeofday(&endtime, NULL);
    endtime = endtime + timeout;

    do {
	bool dead = false;
	do {
	    RealNet::DoWork();
	    status = net->GetNextMessage(&from, &msg);

	    switch (status) {
	    case CONN_NEWINCOMING:
	    case CONN_OK:
		break;
	    case CONN_CLOSED:
		DBG << "connection closed from: " << from.ToString() << endl;
		dead = true;
		break;
	    case CONN_ERROR:
		DBG << "connection error from: " << from.ToString() << endl;
		dead = true;
		break;
	    case CONN_NOMSG:
		break;
	    default:
		Debug::die("Hmm... got weird connection status.");
		break;
	    }

	} while (msg == NULL && status != CONN_NOMSG);
	if (dead) break;
	gettimeofday(&now, NULL);
    } while (msg == NULL && now < endtime);

    return msg;
}

static int int_cmp(const void *a, const void *b) {
    sint32 ia = *(sint32 *)a;
    sint32 ib = *(sint32 *)b;

    return ia - ib;
}

/**
 * Obtain the time offset from our clock to the master clock.
 *
 * @param offset   will be output as the number of ms our clock is off by
 *                 (this will be the median of all the samples)
 * @param self     our address (of RealNet instance)
 * @param net      instance of realnet to send through
 * @param remote   address of master clock
 * @param num      number of iterations of ping-pong
 * @param interval time between iterations
 * @param maxRTT   timeout waiting for master.
 */
bool Synchronize(sint32 *offset, IPEndPoint& self,
		 RealNet *net, IPEndPoint& remote, 
		 int num, int interval, int maxRTT)
{
    ASSERT(num > 0);

    int warmup = 3;
    int maxretries = 3*num;

    sint32 *samples = new sint32[num];

    TimeVal v;
    *offset = 0xFFFFFFFF;

    for (int i=0; i<num; i++) {
	TimeVal start, end;
	MsgPing ping;
	ping.sender = self;

	OS::SleepMillis(interval);

	gettimeofday(&start, NULL);
	net->SendMessage(&ping, &remote, PROTO_UDP);

	Message *msg = NULL;

	msg = ReadMessage(net, maxRTT);

	gettimeofday(&end, NULL);

	if (msg && msg->GetType () == MSG_PING && 
	    ((MsgPing *)msg)->pingNonce == ping.pingNonce) {
	    MsgPing *pong = (MsgPing *)msg;

	    // estimate the offset as: time - (start + RTT/2)
	    // (assumes symmetric latency)
	    uint32 rtt = end - start;
	    samples[i] = pong->time - (start + rtt/2);

	    net->FreeMessage(pong);
	} else {
	    DB(0) << "ping msg timeout or error!" << endl;
	    if (maxretries-- <= 0) {
		// dropped too many!
		return false;
	    } else {
		// try again
		i--;
		continue;
	    }
	}

	// do a couple for warmup
	if (warmup-- > 0) {
	    i--;
	    continue;
	}
    }

    qsort(samples, num, sizeof(samples[0]), int_cmp);
    int median = samples[num/2];
    double mean = 0;    
    for (int i=0; i<num; i++) {
	mean += samples[i];
    }
    mean /= num;

    *offset = median;

    {
	double stddev = 0;
	sint32 min = 1<<30, max = -(1<<30);
	for (int i=0; i<num; i++) {
	    stddev += (mean - samples[i])*(mean - samples[i]);
	    min = MIN(min, samples[i]);
	    max = MAX(max, samples[i]);
	}
	stddev = sqrt((double)stddev/num);

	cout 
	    << "avg=" << mean << "\tmed=" << median 
	    << "\tstd=" << stddev << "\tmin=" << min
	    << "\tmax=" << max << endl;
    }

    delete[] samples;

    return true;
}

int main(int argc, char **argv)
{
    InitializeMercury(&argc, argv, NULL, false);
    if (argc != 3) {
	cerr << "usage: client <self_ip:port> <remote_ip:port>" << endl;
	exit(1);
    }

    IPEndPoint self(argv[1]);
    IPEndPoint remote(argv[2]);

    /*
      char  *saddr  = argv[1];
      uint32 addr   = inet_addr(saddr);
      uint16 port   = atoi(argv[2]);

      char  *sraddr = argv[3];
      uint32 raddr  = inet_addr(sraddr);
      uint32 rport  = atoi(argv[4]);

      IPEndPoint self(addr, port);
      IPEndPoint remote(raddr, rport);
    */

    WANScheduler *sched = new WANScheduler ();
    RealNet *net = new RealNet(sched, self);
    net->StartListening (PROTO_UDP);

    ///////////////////////////////////////////////////////////////////////////

    sint32 offset;

    if (! Synchronize(&offset, self, net, remote, 30, 100, 5000) ) {
	cout << "ERROR: too many ping failures!" << endl;
    }

    return 0;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
