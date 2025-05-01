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
/*************************************************************************
 *
 * $Id: stacktrace.h 2382 2005-11-03 22:54:59Z ashu $
 *
 * Copyright (c) 1998 by Bjorn Reese <breese@mail1.stofanet.dk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 ************************************************************************
 *
 * Purpose:
 *  To generate a stacktrace of a program.
 *
 * Description:
 *  If a debugger is located on the machine we will try this first,
 *  as it usually give better information than built-in methods.
 *  The debugger needs the name of the program to be able to attach
 *  to it, so this must be passed to StackTraceInit(). The second
 *  argument is a filedescriptor that StackTrace() writes to. If this
 *  argument is -1 then standard output will be used.
 *
 * Compilation:
 *  Some platforms have a built-in method to generate stacktraces, but
 *  they are usually located in some non-standard library. The following
 *  are supported:
 *
 *   IRIX    Compile with -DUSE_BUILTIN and -lexc
 *   HP/UX   Compile with -DUSE_BUILTIN and -lcl
 *   GCC     Compile with -DUSE_BUILTIN
 *
 ************************************************************************/

#ifndef H_DEBUG_STACKTRACE
#define H_DEBUG_STACKTRACE

void StackTrace(void);
void StackTraceInit(const char *progname, int handle);

#endif /* H_DEBUG_STACKTRACE */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
