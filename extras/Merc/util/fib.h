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
/*-
 * Copyright 1997, 1998-2003 John-Mark Gurney.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: fib.h 2382 2005-11-03 22:54:59Z ashu $
 *
 */

#ifndef _FIB_H_
#define _FIB_H_

struct fibheap;
struct fibheap_el;
typedef int (*voidcmp)(void *, void *);

/* functions for key heaps */
struct fibheap *fh_makekeyheap(void);
struct fibheap_el *fh_insertkey(struct fibheap *, int, void *);
int fh_minkey(struct fibheap *);
int fh_replacekey(struct fibheap *, struct fibheap_el *, int);
void *fh_replacekeydata(struct fibheap *, struct fibheap_el *, int, void *);

/* functions for void * heaps */
struct fibheap *fh_makeheap(void);
voidcmp fh_setcmp(struct fibheap *, voidcmp);
void *fh_setneginf(struct fibheap *, void *);
struct fibheap_el *fh_insert(struct fibheap *, void *);

/* shared functions */
void *fh_extractmin(struct fibheap *);
void *fh_min(struct fibheap *);
void *fh_replacedata(struct fibheap *, struct fibheap_el *, void *);
void *fh_delete(struct fibheap *, struct fibheap_el *);
void fh_deleteheap(struct fibheap *);
struct fibheap *fh_union(struct fibheap *, struct fibheap *);

#ifdef FH_STATS
int fh_maxn(struct fibheap *);
int fh_ninserts(struct fibheap *);
int fh_nextracts(struct fibheap *);
#endif

#endif /* _FIB_H_ */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
