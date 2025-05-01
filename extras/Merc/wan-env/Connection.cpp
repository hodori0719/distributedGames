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

#include <mercury/common.h>
#include <mercury/IPEndPoint.h>
#include <mercury/Message.h>
#include <mercury/Packet.h>
#include <wan-env/Transport.h>
#include <wan-env/Connection.h>
#include <wan-env/RealNet.h>

Connection::Connection(Transport *t, Socket sock, IPEndPoint *otherEnd) :
    m_Transport(t), m_Socket(sock), m_Status(CONN_OK),
    m_AppPeerAddress((uint32)0, 0), m_SocketPeerAddress((uint32)0, 0)
{
    ASSERT(m_Transport);
    //ASSERT(sock > 0);
    ASSERT(otherEnd);
    m_AppPeerAddress = *otherEnd;
}

Connection::~Connection() {}

void Connection::SetSocketPeerAddress() {
    struct sockaddr_in addr;
    int addrlen = sizeof(sockaddr_in);

    if (getpeername(m_Socket, (struct sockaddr *) &addr, 
		    (socklen_t *) &addrlen) < 0) {
	WARN << "Connection::SetOtherEndAddress(): "
	     << "error in getting peer name for socket." << endl;
    } else {
	m_SocketPeerAddress = IPEndPoint(addr.sin_addr.s_addr, addr.sin_port);
    }
}

int Connection::GetProtocol() {
    return m_Transport->GetProtocol();
}

int Connection::SendMessage(Packet *pkt)
{
    return Send(pkt); // delegate to subclass
}

//
// Deserialize the latest complete packet we read and return the message.
// This routine will be called only when we tell the RealNet class that
// we read a complete message. (i.e., when we get a ready connection
// that has data on it.)
//
Message *Connection::GetLatestMessage() {
    PacketAuxInfo info;
    Packet *pkt = GetNextPacket(&info); // delegate to subclass
    if (pkt) {
	pkt->ResetBufPosition();
	Message *msg = CreateObject<Message>(pkt);
	msg->recvTime = info.timestamp;

	FreePacket(pkt); // delegate to subclass

	return msg;
    } else {
	return NULL;
    }
}

void Connection::Print(FILE * stream)
{
    fprintf(stream, "[%s] app:%s status:%s\n", 
	    g_TransportProtoStrings[GetProtocol()],
	    m_AppPeerAddress.ToString(), g_ConnStatusStrings[m_Status]);
}

ostream& operator<<(ostream& out, Connection *con)
{
    out << "[" << g_TransportProtoStrings[con->GetProtocol()] 
	<< "] app:" << con->m_AppPeerAddress.ToString() 
	<< " status:" << g_ConnStatusStrings[con->m_Status];
    return out;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
