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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:nil -*- */

#include <unistd.h>
#include <sys/ioctl.h>
#include <util/OS.h>
#include <util/debug.h>
#include <mercury/NetworkLayer.h>

//
// Create a socket and check for error... Error-checking is OS-dependent.
//
int  OS::CreateSocket(Socket *pSock, int protocol) {
    if (protocol == (int) PROTO_UDP) 
	{
	    *pSock = socket(AF_INET, SOCK_DGRAM, 0);
	}
    else if (protocol == (int) PROTO_TCP) 
	{
	    *pSock = socket(AF_INET, SOCK_STREAM, 0);
	}
    else {
	Debug::warn ("Unknown protocol specified for socket creation!\n");
	return -1;
    }
#ifdef _WIN32 // Socket is unsigned in this case... GetLastError() is a safer indicator
    if (*pSock == INVALID_SOCKET)  
	return -1;
#else
    if (*pSock < 0)
	return -1;
#endif
    return 0;
}

int  OS::Accept(Socket *pNewSock, Socket sock, struct sockaddr *pAddr, socklen_t *pAddrLen) {
    *pNewSock = accept(sock, pAddr, pAddrLen);
#ifdef _WIN32
    if (*pNewSock == INVALID_SOCKET)
	return -1;
#else
    if (*pNewSock < 0)
	return -1;
#endif
    return 0;
}

int  OS::GetSockOpt(Socket sock, int level, int optname, char *optval, socklen_t *optlen) {
    return getsockopt(sock, level, optname, optval, optlen);
}

int  OS::SetSockOpt(Socket sock, int level, int optname, char *optval, socklen_t optlen) {
    return setsockopt(sock, level, optname, optval, optlen);
}

TimeVal OS::GetSockTimeStamp(Socket sock) {
#if defined(_WIN32) || defined(_DARWIN)
    // XXX Don't know what to do on Windows or Mac OS X ???
    TimeVal tv;
    GetCurrentTime(&tv);
    return tv;
#else
    TimeVal tv;
    if ( ioctl(sock, SIOCGSTAMP, &tv) < 0 ) {
	WARN << "SIOCGSTAMP TimeStamp failed on socket " << sock << "!" 
	     << endl;
    }

    // xxx: ick! but we have todo this here since kernel does not
    // know about slowdown!
    if (g_Slowdown > 1.0f) {
	ApplySlowdown(tv);
    }

    return tv;
#endif
}

//
// The connect error code is funky on WIN32.
//
int  OS::Connect(Socket sock, struct sockaddr *pAddr, socklen_t addrLen) {
    int err = connect(sock, (const sockaddr *) pAddr, addrLen);
#ifdef _WIN32
    if (err == SOCKET_ERROR) {
	return -1;
    } else
	return 0;
#else
    return err;
#endif
}

int  OS::CloseSocket(Socket sock) {
#ifdef _WIN32
    return closesocket(sock);
#else
    return close(sock);
#endif
}

unsigned int OS::GetMaxDatagramSize() {
#ifndef _WIN32
    return 1 << 16;  // On linux, there isn't really any max size, afaik.
#else
    Socket sock = 0;
    static unsigned int          MaxDatagramSize; 

    int err = OS::CreateSocket(&sock, (int) PROTO_UDP); 
    if (err < 0) return err;

    int val_size = sizeof(MaxDatagramSize);

    err =  OS::GetSockOpt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *) &MaxDatagramSize, &val_size);
    if (err == SOCKET_ERROR) {
	if (WSAGetLastError() == WSAENOPROTOOPT)
	    return 1 << 16;	// XXX option not supported on Xbox; assume that 1 << 16 will work
	else
	    return 0;
    } else {
	MercMessage("Max datagram size: %u\n", MaxDatagramSize);
	return MaxDatagramSize;
    }

#endif
}

int  OS::Write(Socket fd, const void *buffer, int length) {
#ifdef _WIN32
    int retval = send(fd, (const char *) buffer, length, 0);
    if (retval == SOCKET_ERROR)
	return -1;
    else
	return retval;
#else
    return write(fd, buffer, length);
#endif
}

int  OS::Read(Socket fd, const void *buffer, int length) {
#ifdef _WIN32
    int retval = recv(fd, (char *) buffer, length, 0);
    if (retval == SOCKET_ERROR)
	return -1;
    else
	return retval;
#else
    return read(fd, (void *) buffer, length);
#endif
}

void OS::SleepMillis(int msec) {
#ifdef _WIN32
    Sleep(msec); 
#else
    usleep(msec * USEC_IN_MSEC);
#endif
}

/*
 * This is just there for debug purposes. select(0,0,0,0, &tv) does not 
 * work on Windows. This code was there to test the error code it 
 * returns. This is all because I couldn't find the "perror" equivalent
 * on Windows (or does there not exist one?)

 if (err == SOCKET_ERROR) {
 switch (WSAGetLastError()) {
 case WSANOTINITIALISED:
 printf("not initialized\n");
 break;
 case WSAEFAULT:
 printf("efault\n");
 break;
 case WSAENETDOWN:
 printf("net down\n");
 break;
 case WSAEINVAL:
 printf("invalid\n");
 break;
 case WSAEINTR:
 printf("interrupted\n");
 break;
 case WSAEINPROGRESS:
 printf("in progress\n");
 break;
 case WSAENOTSOCK:
 printf("not socket\n");
 break;
 }

 }
*/
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
