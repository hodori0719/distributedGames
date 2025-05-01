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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

/**************************************************************************
  FibHeap.h

begin           : Nov 8, 2003
version         : $Id: FibHeap.h 2382 2005-11-03 22:54:59Z ashu $
copyright       : (C) 2003      Jeff Pang        ( jeffpang@cs.cmu.edu 

***************************************************************************/

#ifndef __FIB_HEAP_H__
#define __FIB_HEAP_H__

#include "fib.h"
// C++ wrapper around fib.c

template<class T>
int __fib_heap_cmp(void *a, void *b)
{
    T *at = (T *)a;
    T *bt = (T *)b;

    if (at == NULL || bt == NULL) {
	return at == NULL && bt != NULL ? -1 : at != NULL ? 1 : 0;
    }

    return *at < *bt ? -1 : *bt < *at ? 1 : 0;
}

typedef struct fibheap_el FibHeapElt;

/**
 * A Fibonacci Heap templated by class T. It must hold pointers to class T
 * and class T must have the operator< defined on it. It is also assumed that
 * you will never insert NULL into the heap (it is used as the negative
 * infinity value).
 *
 * Note: Do NOT delete values that are in the heap until *after* you delete
 * the heap. Otherwise it will bork.
 */
template<class T>
class FibHeap
{
 private:
    struct fibheap *h;
 public:
    FibHeap() {
	h = fh_makeheap();
	fh_setcmp(h, __fib_heap_cmp<T>);
	fh_setneginf(h, NULL);
    }
    virtual ~FibHeap() {
	if (h) {
	    fh_deleteheap(h);
	}
    }

    /**
     * Insert a new data value into the heap and return a pointer
     * to the tree element containing it (used for Key operations).
     */
    FibHeapElt *Insert(T *v) {
	return fh_insert(h, (void *)v);
    }

    /**
     * Extract the min data value from the heap.
     */
    T *ExtractMin() {
	return (T *)fh_extractmin(h);
    }

    /**
     * Peek at the min data value in the heap.
     */
    T *GetMin() {
	return (T *)fh_min(h);
    }

    /**
     * Change the data value associated with an element.
     *
     * XXX: Only works to decrease key! Increasing a key will abort()!
     */
    T *ChangeKey(FibHeapElt *elt, T *v) {
	return (T *)fh_replacedata(h, elt, (void *)v);
    }

    /**
     * Delete a key from the heap and return its data value.
     */
    T *DeleteKey(FibHeapElt *elt) {
	return (T *)fh_delete(h, elt);
    }

    /** 
     * Merge the other heap with this one. Invalidates other (do not use
     * anymore).
     */
    void Merge(FibHeap *other) {
	h = fh_union(h, other->h);
	other->h = NULL;
    }
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
