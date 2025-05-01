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

#ifndef __OS__H
#define __OS__H

#include <sys/socket.h>
#include <util/types.h>
#include <util/TimeVal.h>

#ifdef _WIN32
typedef SOCKET         Socket;
#else
typedef int            Socket;
typedef long int       DWORD;
#endif  // ifdef _WIN32

//
// This file contains the OS dependent stuff. 
//  Try to keep most "#ifdef"s in this and the 
//  corresponding .cpp files only.
//
class OS {
 public:
    static void  SleepMillis(int msec);
    static int   CreateSocket(Socket *pSock, int protocol);
    static int   Accept(Socket *pNewSock, Socket sock, struct sockaddr *pAddr, socklen_t *pAddrLen);
    static int   Connect(Socket sock, struct sockaddr *pAddr, socklen_t addrLen);
    static int   GetSockOpt(Socket sock, int level, int optname, char *optval, socklen_t *optlen);
    static int   SetSockOpt(Socket sock, int level, int optname, char *optval, socklen_t optlen);
    static TimeVal GetSockTimeStamp(Socket sock);
    static int   CloseSocket(Socket sock);
    static unsigned int GetMaxDatagramSize();

    static int   Write(Socket fd, const void *buffer, int length);
    static int   Read(Socket fd, const void *buffer, int length);

    inline static void  GetCurrentTime(TimeVal *tv) {
#ifdef _WIN32
	// Jeff: Correct function (for sub-msec accuracy) is:
	FILETIME hsec; // time in 100ns units
	GetSystemTimeAsFileTime(&hsec);
	uint64 longtime = 
	    (((uint64)hsec.dwHighDateTime)<<32) | ((uint64)hsec.dwLowDateTime);
	longtime /= 10; // convert to usec
	tv->tv_sec  = (uint32)(longtime/USEC_IN_SEC);
	tv->tv_usec = (uint32)(longtime%USEC_IN_SEC);
#else
	gettimeofday(tv, NULL);
#endif
	if (g_Slowdown > 1.0f) {
	    ApplySlowdown(*tv);

	    /* is there any way this can be made more efficient? cause this 
	     * can be in the critical path */
	    /*
	      sint64 dif =  ((sint64) tv->tv_sec - (sint64) g_StartTime.tv_sec) * USEC_IN_SEC + 
	      ((sint64) tv->tv_usec - (sint64) g_StartTime.tv_usec);
	      dif = (sint64) ((double) dif / g_Slowdown);

	      tv->tv_sec = g_StartTime.tv_sec + dif / USEC_IN_SEC;
	      tv->tv_usec = g_StartTime.tv_usec + dif % USEC_IN_SEC;
	      if (tv->tv_usec > USEC_IN_SEC) { 
	      tv->tv_sec++;
	      tv->tv_usec %= USEC_IN_SEC;
	      }
	    */
	}
    }
};

#endif // __OS__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
