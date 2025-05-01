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
// Implementation of the Socket class.


#include "SocketStream.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <util/debug.h>


SocketStream::SocketStream() :
    m_sock ( -1 )
{

    memset ( &m_addr,
	     0,
	     sizeof ( m_addr ) );

}

SocketStream::~SocketStream()
{
    // XXX This is dangerous since everything is pass by reference!
    // must call explicitly to be safe
    //if ( IsValid() )
    //::close ( m_sock );
}

bool SocketStream::Close()
{
    return close(m_sock);
}

bool SocketStream::Create()
{
    m_sock = socket ( AF_INET,
		      SOCK_STREAM,
		      0 );

    if ( ! IsValid() )
	return false;


    // TIME_WAIT - argh
    int on = 1;
    if ( SetSockOpt(SOL_SOCKET, SO_REUSEADDR, 
		    (char*) &on, sizeof ( on ) ) == -1 )
	return false;


    return true;

}



bool SocketStream::Bind ( const int port )
{

    if ( ! IsValid() )
	{
	    return false;
	}



    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = INADDR_ANY;
    m_addr.sin_port = htons ( port );

    int bind_return = ::bind ( m_sock,
			       ( struct sockaddr * ) &m_addr,
			       sizeof ( m_addr ) );


    if ( bind_return == -1 )
	{
	    return false;
	}

    return true;
}


bool SocketStream::Listen() const
{
    if ( ! IsValid() )
	{
	    return false;
	}

    int listen_return = ::listen ( m_sock, MAXCONNECTIONS );

    if ( listen_return == -1 )
	{
	    return false;
	}

    return true;
}


bool SocketStream::Accept ( SocketStream& new_socket ) const
{

    int addr_length = sizeof ( new_socket.m_addr );
    new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &new_socket.m_addr, ( socklen_t * ) &addr_length );

    if ( new_socket.m_sock < 0 )
	return false;
    else
	return true;
}


bool SocketStream::Send ( const std::string s ) const
{
    int status = -1;
    while (status < 0) {
#if defined(_DARWIN) || defined(_WIN32)
	status = ::send ( m_sock, s.c_str(), s.size(), 0 );
#else
	status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
#endif
	if (status < 0) {
	    if (errno == EINTR || errno == EAGAIN)
		continue;
	    break;
	}
    }

    if ( status == -1 ) {
	return false;
    }
    else {
	return true;
    }
}


int SocketStream::Recv ( std::string& s ) const
{
    char buf [ MAXRECV + 1 ];

    s = "";

    memset ( buf, 0, MAXRECV + 1 );

    int status = -1;

    while (status < 0) {
	status = ::recv ( m_sock, buf, MAXRECV, 0 );
	if (status < 0) {
	    if (errno == EINTR || errno == EAGAIN)
		continue;
	    break;
	}
    }

    if ( status == -1 ) {
	DBG << "status == -1   errno == " 
	    << errno << "  in SocketStream::recv\n";
	return 0;
    }
    else if ( status == 0 ) {
	return 0;
    }
    else {
	s = buf;
	return status;
    }
}



bool SocketStream::Connect ( const std::string host, const int port )
{
    if ( ! IsValid() ) return false;

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons ( port );

    int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

    if ( errno == EAFNOSUPPORT ) return false;

    status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

    if ( status == 0 )
	return true;
    else
	return false;
}

void SocketStream::SetNonBlocking ( const bool b )
{

    int opts;

    opts = fcntl ( m_sock,
		   F_GETFL );

    if ( opts < 0 ) {
	WARN << "Set non-blocking socket failed: " << strerror(errno) << endl;
	return;
    }

    if ( b )
	opts = ( opts | O_NONBLOCK );
    else
	opts = ( opts & ~O_NONBLOCK );

    fcntl ( m_sock,
	    F_SETFL,opts );

}

int SocketStream::SetSockOpt(int level, int optname, void *optval, socklen_t optlen)
{
    return setsockopt(m_sock, level, optname, optval, optlen);
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
