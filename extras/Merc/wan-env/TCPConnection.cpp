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

#include <wan-env/TCPConnection.h>
#include <mercury/Message.h>
#include <mercury/Packet.h>
#include <wan-env/RealNet.h>

TCPConnection::TCPConnection(Transport *t, Socket sock, IPEndPoint *otherEnd) :
    UnbufferedConnection(t, sock, otherEnd)
{
    SetSocketPeerAddress();
}

TCPConnection::~TCPConnection() {}

int TCPConnection::Send(Packet *pkt) {
    if (pkt->GetUsed () > 65 * 1000) {
	WARN << " humonguous packet length=" << pkt->GetUsed () << endl;
	ASSERT (pkt->GetUsed () <= 65 * 1000);
    }
    int ret = ((TCPTransport *)GetTransport())->_Send_TCP(GetSocket(), 
							  pkt->GetBuffer (),
							  pkt->GetUsed ());
    delete pkt;
    return ret;
}

Packet *TCPConnection::GetNextPacket(PacketAuxInfo* aux)
{
    GetTransport()->IncrReadPackets();
    return UnbufferedConnection::GetNextPacket(aux);
}

int TCPConnection::_InitNewMessage() {
    uint32 pktLength;
    uint32 tBuf;

    // block on this read. it's a small read anyways, and if select returned 
    // yes, it's most likely we'll end up getting this block of data...
    int retcode = RealNet::ReadBlock(GetSocket(),(byte *)&tBuf,sizeof(uint32));
    pktLength = ntohl(tBuf);

    if (retcode < (int)sizeof(uint32))
	return retcode;

    if (pktLength > (uint32)TCPTransport::MAX_TCP_MSGSIZE) {
	WARN << "Got a packet length that is too big: " << pktLength << endl;

	// Should close connection here... someone got desync'd

	return -1;
    }

    if (retcode > 0) {
	if (pktLength > 65 * 1000) { 
	    WARN << " pktLength=" << pktLength << endl;
	}
	_InitPacket(pktLength);
    }
    return retcode;
}

//
// Read a message from a TCP connection
//
int TCPConnection::_ReadMessage_TCP() {
    ASSERT(m_Packet != 0);

    int  rBufPos = m_Packet->GetBufPosition (), rSize = m_Packet->GetMaxSize ();
    byte *buffer = m_Packet->GetBuffer ();

    // Read as much as is possible  NEVER BLOCK
    ASSERT(rBufPos < rSize);

    // have read up to BufPosition; have to read m_Size in all...
    int retcode = RealNet::ReadNoBlock(GetSocket(), buffer + rBufPos, 
				       rSize - rBufPos);
    if (retcode <= 0) {
	DB_DO (1) {
	    perror ("tcp:readnoblock");
	}
	return NetworkLayer::ReportReadError(retcode);
    }

    m_Packet->IncrBufPosition (retcode);
    rBufPos = m_Packet->GetBufPosition ();

    if (rBufPos == rSize) {
	// XXX -- can we get the kernel level timestamp? this is delayed...
	m_TimeStamp = GetTransport ()->TimeNow ();
	// doesn't work!
	//m_TimeStamp = OS::GetSockTimeStamp(GetSocket());

	DB(5) << "Read is complete now..." << endl;
	return NetworkLayer::READ_COMPLETE;
    } 
    else { 
	DB(5) << "Read continuing...: read " << retcode << "bytes; " 
	      << " rBufPos = " << rBufPos << " rSize=" << rSize << endl;
	return NetworkLayer::READ_INCOMPLETE;
    }
}

//
// Read a message from the connection. If the complete message cannot be read,
// return "READ_INCOMPLETE" and the RealNet class will call us again when we 
// will proceed to complete the read.
//
int TCPConnection::PerformRead() {
    if ( !m_Packet ) {
	int error = _InitNewMessage();
	if (error <= 0) {
	    int report = NetworkLayer::ReportReadError(error);

	    if (report == NetworkLayer::READ_CLOSE) {
		DB(9) << "  case #1 - connection CLOSED!" << endl;
		SetStatus(CONN_CLOSED);
	    } else if (report == NetworkLayer::READ_ERROR) {
		DB(9) << "  - connection error..." << endl;
		SetStatus(CONN_ERROR);
	    }
	    return report;
	}

	if (!RealNet::IsDataWaiting(GetSocket()))
	    return NetworkLayer::READ_INCOMPLETE;
    }

    int report = _ReadMessage_TCP();

    if (report == NetworkLayer::READ_CLOSE) {
	DB(4) << "  case #3 - connection CLOSED!" << endl;
	SetStatus( CONN_CLOSED );
	_ResetPacket();
    } else if (report == NetworkLayer::READ_ERROR) {
	DB(4) << "  - connection error..." << endl;
	SetStatus( CONN_ERROR );
	_ResetPacket();
    }
    return report;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
