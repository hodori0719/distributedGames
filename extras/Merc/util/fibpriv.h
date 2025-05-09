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
 * Copyright 1997, 1999-2003 John-Mark Gurney.
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
 *	$Id: fibpriv.h 2382 2005-11-03 22:54:59Z ashu $
 *
 */

#ifndef _FIBPRIV_H_
#define _FIBPRIV_H_

struct fibheap_el;

/*
 * global heap operations
 */
struct fibheap {
    int	(*fh_cmp_fnct)(void *, void *);
    int	fh_n;
    int	fh_Dl;
    struct	fibheap_el **fh_cons;
    struct	fibheap_el *fh_min;
    struct	fibheap_el *fh_root;
    void	*fh_neginf;
    int	fh_keys		: 1;
#ifdef FH_STATS
    int	fh_maxn;
    int	fh_ninserts;
    int	fh_nextracts;
#endif
};

static void fh_initheap(struct fibheap *);
static void fh_insertrootlist(struct fibheap *, struct fibheap_el *);
static void fh_removerootlist(struct fibheap *, struct fibheap_el *);
static void fh_consolidate(struct fibheap *);
static void fh_heaplink(struct fibheap *h, struct fibheap_el *y,
			struct fibheap_el *x);
static void fh_cut(struct fibheap *, struct fibheap_el *, struct fibheap_el *);
static void fh_cascading_cut(struct fibheap *, struct fibheap_el *);
static struct fibheap_el *fh_extractminel(struct fibheap *);
static void fh_checkcons(struct fibheap *h);
static void fh_destroyheap(struct fibheap *h);
static int fh_compare(struct fibheap *h, struct fibheap_el *a,
		      struct fibheap_el *b);
static int fh_comparedata(struct fibheap *h, int key, void *data,
			  struct fibheap_el *b);
static void fh_insertel(struct fibheap *h, struct fibheap_el *x);
static void fh_deleteel(struct fibheap *h, struct fibheap_el *x);

/*
 * specific node operations
 */
struct fibheap_el {
    int	fhe_degree;
    int	fhe_mark;
    struct	fibheap_el *fhe_p;
    struct	fibheap_el *fhe_child;
    struct	fibheap_el *fhe_left;
    struct	fibheap_el *fhe_right;
    int	fhe_key;
    void	*fhe_data;
};

static struct fibheap_el *fhe_newelem(void);
static void fhe_initelem(struct fibheap_el *);
static void fhe_insertafter(struct fibheap_el *a, struct fibheap_el *b);
static inline void fhe_insertbefore(struct fibheap_el *a, struct fibheap_el *b);
static struct fibheap_el *fhe_remove(struct fibheap_el *a);
#define	fhe_destroy(x)	free((x))

/*
 * general functions
 */
static inline int ceillog2(unsigned int a);

#endif /* _FIBPRIV_H_ */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
