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
#include <mercury/IPEndPoint.h>
#include <mercury/Node.h>
#include <mercury/ID.h>
#include <mercury/EventQueue.h>
#include <sim-env/Simulator.h>
#include <map>

DummyNode s_DummyNode (NULL, NULL, SID_NONE);

typedef map <IPEndPoint, Node* , less_SID> NodeMap;
NodeMap s_NodeMap;

Simulator::Simulator() : m_CurrentTime (TIME_NONE)
{
    m_Queue = new EventQueue ();
    m_LatencyFunc = NULL;
    // srand (42);
}

Simulator::~Simulator()
{
    delete m_Queue;
}

void Simulator::AddNode (Node& node) {
    s_NodeMap.insert (NodeMap::value_type (node.GetAddress (), &node));
}

void Simulator::RemoveNode (Node& node) {
    s_NodeMap.erase (node.GetAddress ());
}

void Simulator::RaiseEvent (ref<SchedulerEvent> event, IPEndPoint& address, u_long millis)
{
    Node *node = NULL;

    if (address == SID_NONE) {
	node = &s_DummyNode;
    }
    else {	
	NodeMap::iterator it = s_NodeMap.find (address);
	if (it == s_NodeMap.end ())
	    return;

	node = it->second;
    }

    m_Queue->Insert (event, *node, TimeNow () + millis);
}

// dont use this!
void Simulator::CancelEvent (ref<SchedulerEvent> event)
{
    ASSERT (false);   
}

void Simulator::ProcessTill (TimeVal& limit)
{
    while (!m_Queue->Empty ())
	{
	    const SEvent *ev = m_Queue->Top ();
	    if (ev->firetime > limit) 
		break;

	    m_CurrentTime = ev->firetime;
	    ev->ev->Execute (ev->node, m_CurrentTime);
	    m_Queue->Pop ();
	}
}

static Packet* _MakePacket (Message *msg)
{
    int len = msg->GetLength ();
    Packet *pkt = new Packet (len);

    msg->Serialize (pkt);

    // if this isn't true, some component of the msg
    // allocated too much or too little space!
    ASSERT (pkt->GetBufPosition () == len);
    return pkt;
}

class MessageEvent : public SchedulerEvent {
    Packet *pkt;
public:
    MessageEvent (Message *m) {
	pkt = _MakePacket (m);
    }
    ~MessageEvent () { delete pkt; }

    void Execute (Node& node, TimeVal& timenow) {
	pkt->ResetBufPosition ();
	Message *msg = CreateObject <Message> (pkt);
	msg->hopCount += 1;
	msg->recvTime = timenow;
	// INFO << "invoking on=" << node.GetAddress () << " recvmsg=" << msg << endl;
	node.ReceiveMessage (&(msg->sender), msg);
    }
};

int Simulator::SendMessage (Message *msg, IPEndPoint *toWhom, TransportType proto)
{
    ASSERT (toWhom != NULL);

    // INFO << "sending message to " << *toWhom << endl;	
    // INFO << "msg=" << msg << endl;

    u_long latency = 0;
    if (m_LatencyFunc)
	latency = (*m_LatencyFunc) (msg->sender, *toWhom);
    else 
	latency = (u_long) (1 + 0.5 * drand48()) * NODE_TO_NODE_LATENCY;

    if (g_Slowdown > 1.0f)
	latency = (u_long) (latency * g_Slowdown);

    RaiseEvent (new refcounted<MessageEvent>(msg), *toWhom, latency);
    return 0;
}

void Simulator::FreeMessage (Message *msg) {
    delete msg;
}

// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
