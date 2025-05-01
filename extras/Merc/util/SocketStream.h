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
// Definition of the Socket class
//
// Derived from: http://www.pcs.cnu.edu/~dgame/sockets/socketsC++/sockets.html
//

#ifndef Socket_class
#define Socket_class


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include "SocketException.h"

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;
//const int MSG_NOSIGNAL = 0; // defined by dgame

class SocketStream
{
 public:
    SocketStream();
    virtual ~SocketStream();

    // Server initialization
    bool Create();
    bool Bind( const int port );
    bool Listen() const;
    bool Accept( SocketStream & ) const;
    bool Close();

    // Client initialization
    bool Connect( const std::string host, const int port );

    // Data Transimission
    bool Send( const std::string ) const;
    int  Recv( std::string& ) const;

    void SetNonBlocking( const bool );
    int  SetSockOpt(int , int , void *, socklen_t);
    int  GetSocket() { return m_sock; }
    bool IsValid() const { return m_sock != -1; }

 private:

    int m_sock;
    sockaddr_in m_addr;


};


#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
